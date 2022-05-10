#include "pch.h"
#include "RayTracingManager.h"

void RayTracingManager::Initialize(
	Microsoft::WRL::ComPtr<ID3D12Device5> pDevice,
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> pCommandList,
	std::vector<std::unique_ptr<ModelInfo>> models,
	uint32_t totalNrObjects
) noexcept
{
	m_pDevice = pDevice;
	m_pCommandList = pCommandList;

	//Make sure to reset command memory before this. Vertex buffer needs to have a GPU address.
	//Transition the vertexbuffer resource to non pixel shader resource before this aswell.

	//Create the bottom level acceleration structure, send in the vertex data which consists of the local vertex data for all different models used.
	//Only one instance of each model in this vertex buffer.
	BuildBottomAcceleration(models);

	//Make sure we are finished building the bottom level acceleration structure before using it.
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	for (auto resultBuffer : m_ResultBuffersBottom)
	{
		uavBarrier.UAV.pResource = resultBuffer.second.Get();
		STDCALL(m_pCommandList->ResourceBarrier(1, &uavBarrier));
	}
	
	BuildTopAcceleration(models, totalNrObjects);

	uavBarrier.UAV.pResource = m_pResultBufferTop.Get();
	STDCALL(m_pCommandList->ResourceBarrier(1, &uavBarrier));
}

void RayTracingManager::Refit() noexcept
{
}

void RayTracingManager::Rebuild() noexcept
{
}

void RayTracingManager::BuildBottomAcceleration(
	const std::vector<std::unique_ptr<ModelInfo>>& models
) noexcept
{
	
	//Create the descriptions of the different geometries.
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDescs[1] = {};
	{
		geometryDescs[0].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometryDescs[0].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		geometryDescs[0].Triangles.Transform3x4 = NULL;
		geometryDescs[0].Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN; //Maybe change later?
		geometryDescs[0].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		geometryDescs[0].Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	}

	//Create the bottom level acceleration structure input desc.
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomInputs = {};
	{
		bottomInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		bottomInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; //Maybe swap this for better performance in a dynamic scene.
		bottomInputs.NumDescs = 1;
		bottomInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	}
	
	//Go through each unique model and create a result- & scratchbuffer for each.
	//This is so that they later
	for (uint32_t i{0u}; i < models.size(); i++)
	{
		std::wstring currentModelName = models[i]->m_model->GetName();
		geometryDescs[0].Triangles.IndexCount = models[i]->m_model->GetIndexCount();
		geometryDescs[0].Triangles.VertexCount = models[i]->m_model->GetIndexCount();
		geometryDescs[0].Triangles.IndexBuffer = models[i]->m_model->GetIndexBufferGPUAddress();
		geometryDescs[0].Triangles.VertexBuffer.StartAddress = models[i]->m_model->GetVertexBufferGPUAddress();
		
		bottomInputs.pGeometryDescs = geometryDescs;

		//Get prebuild info that is used for creating the acceleration structure.
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
		STDCALL(m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomInputs, &prebuildInfo));

		//Create temporary local Result & Scratchbuffer which we then put into the unordered maps which can be accessed with the model's name.
		Microsoft::WRL::ComPtr<ID3D12Resource> tempResult = nullptr;
		//Create the result buffer.
		CreateCommitedBuffer(
			L"Bottom Level Acceleration Structure - Resultbuffer: " + currentModelName,
			tempResult,
			D3D12_HEAP_TYPE_DEFAULT,
			prebuildInfo.ResultDataMaxSizeInBytes,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE
		);

		Microsoft::WRL::ComPtr<ID3D12Resource> tempScratch = nullptr;
		//Create the scratch buffer.
		CreateCommitedBuffer(
			L"Bottom Level Acceleration Structure - Scratchbuffer: " + currentModelName,
			tempScratch,
			D3D12_HEAP_TYPE_DEFAULT,
			prebuildInfo.ScratchDataSizeInBytes,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);

		//Finally create the acceleration structure for this model.
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelerationDesc = {};
		{
			accelerationDesc.DestAccelerationStructureData = tempResult->GetGPUVirtualAddress();
			accelerationDesc.Inputs = bottomInputs;
			accelerationDesc.SourceAccelerationStructureData = NULL; //Change this when dynamic scene?
			accelerationDesc.ScratchAccelerationStructureData = tempScratch->GetGPUVirtualAddress();
		}
		//Insert the created buffers into the unordered maps.
		m_ResultBuffersBottom.insert(std::pair(currentModelName, tempResult));
		m_ScratchBuffersBottom.insert(std::pair(currentModelName, tempScratch));

		STDCALL(m_pCommandList->BuildRaytracingAccelerationStructure(&accelerationDesc, 0, nullptr)); //Maybe catch postbuild info here when rebuilding/refitting is needed?
	}
	
	//Add 1 geometryDesc element per other arbritary geometry.
}

void RayTracingManager::BuildTopAcceleration(
	const std::vector<std::unique_ptr<ModelInfo>>& models,
	uint32_t totalNrObjects
) noexcept
{
	//Create the top level instance buffer resource.
	CreateCommitedBuffer(
		L"Top Level Acceleration Structure - InstanceBuffer",
		m_pInstanceBufferTop,
		D3D12_HEAP_TYPE_UPLOAD,
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * totalNrObjects,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ
	);

	//Define the desc for the top level instance buffer resource.
	uint32_t index = 0u;
	std::unique_ptr<D3D12_RAYTRACING_INSTANCE_DESC[]> instancingDesc(DBG_NEW D3D12_RAYTRACING_INSTANCE_DESC[totalNrObjects]);
	for (uint32_t i{0u}; i < models.size(); i++)
	{
		std::wstring currentModelName = models[i]->m_model->GetName();
		for (uint32_t j{0u}; j < models[i]->m_transforms.size(); j++)
		{
			//Change the transform to use the object's transform
			instancingDesc[index].Transform[0][0] = instancingDesc[index].Transform[1][1] = instancingDesc[index].Transform[2][2] = 1;
			instancingDesc[index].InstanceID = index;
			instancingDesc[index].InstanceMask = 0xFF;
			instancingDesc[index].InstanceContributionToHitGroupIndex = 0;
			instancingDesc[index].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			instancingDesc[index].AccelerationStructure = m_ResultBuffersBottom[currentModelName]->GetGPUVirtualAddress();
			index++;
		}
	}
	
	//Copy the desc to the created buffer resource.
	D3D12_RANGE zero = { 0, 0 };
	unsigned char* mappedPtr = nullptr;
	HR(m_pInstanceBufferTop->Map(0, &zero, reinterpret_cast<void**>(&mappedPtr)));
	std::memcpy(mappedPtr, &instancingDesc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * totalNrObjects);
	STDCALL(m_pInstanceBufferTop->Unmap(0, nullptr));

	//Create the top level acceleration structure description.
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topInputs = {};
	{
		topInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		topInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; //Maybe change this to faster build and make it a member variable.
		topInputs.NumDescs = totalNrObjects;
		topInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		topInputs.InstanceDescs = m_pInstanceBufferTop->GetGPUVirtualAddress();
	}

	//Get prebuild info that is used for creating the acceleration structure.
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topInputs, &prebuildInfo);

	//Create the buffers and acceleration structure.
	BuildStructure(
		L"Top Level Acceleration Structure - ResultBuffer",
		m_pResultBufferTop,
		prebuildInfo.ResultDataMaxSizeInBytes,
		L"Top Level Acceleration Structure - ScratchBuffer",
		m_pScratchBufferTop,
		prebuildInfo.ScratchDataSizeInBytes,
		topInputs
	);
}

void RayTracingManager::BuildStructure(
	std::wstring resultName,
	Microsoft::WRL::ComPtr<ID3D12Resource> resultBuffer,
	uint64_t resultSize,
	std::wstring scratchName,
	Microsoft::WRL::ComPtr<ID3D12Resource> scratchBuffer,
	uint64_t scratchSize,
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs
	) noexcept
{
	//Create the result buffer.
	CreateCommitedBuffer(
		resultName,
		resultBuffer,
		D3D12_HEAP_TYPE_DEFAULT,
		resultSize,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE
	);

	//Create the scratch buffer.
	CreateCommitedBuffer(
		scratchName,
		scratchBuffer,
		D3D12_HEAP_TYPE_DEFAULT,
		scratchSize,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);

	//Finally create the acceleration structure.
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelerationDesc = {};
	{
		accelerationDesc.DestAccelerationStructureData = resultBuffer->GetGPUVirtualAddress();
		accelerationDesc.Inputs = inputs;
		accelerationDesc.SourceAccelerationStructureData = NULL; //Change this when dynamic scene?
		accelerationDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
	}

	STDCALL(m_pCommandList->BuildRaytracingAccelerationStructure(&accelerationDesc, 0, nullptr)); //Maybe catch postbuild info here when rebuilding/refitting is needed?
}

void RayTracingManager::CreateCommitedBuffer(
	std::wstring bufferName,
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer,
	D3D12_HEAP_TYPE heapType,
	uint64_t bufferSize,
	D3D12_RESOURCE_FLAGS flags,
	D3D12_RESOURCE_STATES initialState
) noexcept
{
	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = heapType;

	D3D12_RESOURCE_DESC bufferDesc = {};
	{
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		bufferDesc.Width = bufferSize;
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = flags;
	}

	HR(m_pDevice->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		initialState,
		nullptr,
		IID_PPV_ARGS(buffer.GetAddressOf()) //?
	));
	HR(buffer->SetName(bufferName.c_str()));
}
