#include "pch.h"
#include "Scene.h"

void Scene::Initialize()
{
	m_pRayTracingManager = std::make_unique<RayTracingManager>();


	//Create objects here.
	AddVertexObject("Models/deer.obj", DirectX::XMVectorSet(0.0f, 0.0f, 50.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(0.2f, 0.2f, 0.2f, 1.0f));
	AddVertexObject("Models/Shark.obj", DirectX::XMVectorSet(50.0f, 0.0f, 50.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f));

	auto pCommandAllocator = DXCore::GetCommandAllocators()[0];
	auto pCommandList = DXCore::GetCommandList();
	auto pDevice = DXCore::GetDevice();

	HR(pCommandList->Close());
	ID3D12CommandList* commandLists[] = { pCommandList.Get() };
	STDCALL(DXCore::GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists));
	RenderCommand::Flush();
	
	HR(pCommandAllocator->Reset());
	HR(pCommandList->Reset(pCommandAllocator.Get(), nullptr));

	m_pRayTracingManager->Initialize(m_UniqueModels, m_Objects, m_TotalMeshes);

	HR(pCommandList->Close());
	STDCALL(DXCore::GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists));
	RenderCommand::Flush();

	HR(pCommandAllocator->Reset());
	HR(pCommandList->Reset(pCommandAllocator.Get(), nullptr));
}

void Scene::AddVertexObject(const std::string path, DirectX::XMVECTOR pos, DirectX::XMVECTOR rot, DirectX::XMVECTOR scale)
{	
	std::shared_ptr<Model> tempModel = nullptr;
	bool newModel = false;

	//Check if the requested model is new or not.
	if (m_UniqueModels.find(path) == m_UniqueModels.end())
	{
		//If it is a new model we have to create it and initialize (load it using Assimp) the model.
		//It is then inserted into our unique models map.
		tempModel = std::make_shared<Model>();
		tempModel->Initialize(path);
		m_UniqueModels.insert(std::pair(path, tempModel));

		newModel = true;
	}
	else
	{
		//Otherwise we simply fetch the shared_ptr to the unique model.
		tempModel = m_UniqueModels[path];
	}
	m_TotalMeshes += (uint32_t)(tempModel->GetMeshes().size());

	//No matter the case we want to create a new object using the fetched model.
	std::shared_ptr<VertexObject> tempObject = std::make_shared<VertexObject>();
	tempObject->Initialize(
		std::move(tempModel),
		pos,
		rot,
		scale
	);
	
	if (newModel)
	{
		//If the model was new we have to create a new vector that we add the object to. We then insert the vector into our objects map.
		std::vector<std::shared_ptr<VertexObject>> objects = {};
		objects.push_back(std::move(tempObject));
		m_Objects.insert(std::pair(path, objects));
	}
	else
	{
		//If the model already existed we simply push back the new object to the already existing vector.
		m_Objects[path].push_back(std::move(tempObject));
	}
	
	//Increment the total number of objects.
	m_TotalObjects++;
	
}