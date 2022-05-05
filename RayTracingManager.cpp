#include "pch.h"
#include "RayTracingManager.h"

void RayTracingManager::Initialize(
	Microsoft::WRL::ComPtr<ID3D12Device5> pDevice,
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> pCommandList,
	Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBuffer
) noexcept
{
	m_pDevice = pDevice;
	m_pCommandList = pCommandList;

	//Reset command memory
	//Transition the vertexbuffer resource to non pixel shader resource.

	//Create the bottom level acceleration structure.
	BuildBottomAcceleration(pVertexBuffer->GetGPUVirtualAddress());

	//Make sure we are finished building the bottom level acceleration structure before using it.
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = m_pResultBufferBottom.Get();
	STDCALL(m_pCommandList->ResourceBarrier(1, &uavBarrier));


}

void RayTracingManager::BuildBottomAcceleration(D3D12_GPU_VIRTUAL_ADDRESS vBufferFirstPos) noexcept
{
	//Create the descriptions of the different geometries.
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDescs[1] = {};
	{
		geometryDescs[0].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometryDescs[0].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		geometryDescs[0].Triangles.Transform3x4 = NULL;
		geometryDescs[0].Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN; //Maybe change later?
		geometryDescs[0].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		geometryDescs[0].Triangles.IndexCount = 0; //Change later.
		geometryDescs[0].Triangles.VertexCount = 3; //Change later.
		geometryDescs[0].Triangles.IndexBuffer = NULL;
		geometryDescs[0].Triangles.VertexBuffer.StartAddress = vBufferFirstPos;
		geometryDescs[0].Triangles.VertexBuffer.StrideInBytes = sizeof(/*Vertex*/float) * 4;

		//Add 1 geometryDesc element per other arbritary geometry.
	}

	//Create the bottom level acceleration structure based on the created geometries.
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomInputs = {};
	{
		bottomInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		bottomInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; //Maybe swap this for better performance in a dynamic scene.
		bottomInputs.NumDescs = 1;
		bottomInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		bottomInputs.pGeometryDescs = geometryDescs;
	}

	//Get prebuild info
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	STDCALL(m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomInputs, &prebuildInfo));

	//Create the result buffer.
	CreateCommitedBuffer(
		L"Bottom Level Acceleration Structure - ResultBuffer",
		m_pResultBufferBottom,
		D3D12_HEAP_TYPE_DEFAULT,
		prebuildInfo.ResultDataMaxSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE
	);
	
	//Create the scratch buffer.
	CreateCommitedBuffer(
		L"Bottom Level Acceleration Structure - ScratchBuffer",
		m_pScratchBufferBottom,
		D3D12_HEAP_TYPE_DEFAULT,
		prebuildInfo.ScratchDataSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);

	//Finally create the acceleration structure.
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelerationDesc = {};
	{
		accelerationDesc.DestAccelerationStructureData = m_pResultBufferBottom->GetGPUVirtualAddress();
		accelerationDesc.Inputs = bottomInputs;
		accelerationDesc.SourceAccelerationStructureData = NULL; //Change this when dynamic scene?
		accelerationDesc.ScratchAccelerationStructureData = m_pScratchBufferBottom->GetGPUVirtualAddress();
	}
	
	STDCALL(m_pCommandList->BuildRaytracingAccelerationStructure(&accelerationDesc, 0, nullptr)); //Maybe catch postbuild info here when rebuilding/refitting is needed?
}

void RayTracingManager::BuildTopAcceleration() noexcept
{
	CreateCommitedBuffer(
		L"Top Level Acceleration Structure - InstanceBuffer",
		m_pInstanceBufferTop,
		D3D12_HEAP_TYPE_UPLOAD,
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ
	);

	D3D12_RAYTRACING_INSTANCE_DESC instancingDesc = {};
	{
		instancingDesc.Transform[0][0] = instancingDesc.Transform[1][1] = instancingDesc.Transform[2][2] = 1;
		instancingDesc.InstanceID = 0;
		instancingDesc.InstanceMask = 0xFF;
		instancingDesc.InstanceContributionToHitGroupIndex = 0;
		instancingDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		instancingDesc.AccelerationStructure = m_pResultBufferBottom->GetGPUVirtualAddress();
	}
	

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
