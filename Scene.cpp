#include "pch.h"
#include "Scene.h"

void Scene::Initialize()
{
	m_pRayTracingManager = std::make_unique<RayTracingManager>();


	//Create objects here.
	AddVertexObject();

	auto pCommandAllocator = DXCore::GetCommandAllocators()[0];
	auto pCommandList = DXCore::GetCommandList();
	auto pDevice = DXCore::GetDevice();

	HR(pCommandList->Close());
	ID3D12CommandList* commandLists[] = { pCommandList.Get() };
	STDCALL(DXCore::GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists));
	RenderCommand::Flush();
	
	HR(pCommandAllocator->Reset());
	HR(pCommandList->Reset(pCommandAllocator.Get(), nullptr));

	//m_pRayTracingManager->Initialize(m_UniqueModels, m_Objects, m_TotalObjects);

	HR(pCommandList->Close());
	STDCALL(DXCore::GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists));
	RenderCommand::Flush();

	HR(pCommandAllocator->Reset());
	HR(pCommandList->Reset(pCommandAllocator.Get(), nullptr));
}

void Scene::AddVertexObject()
{
	m_TotalObjects++;
	
	std::shared_ptr<Model> tempModel = std::make_shared<Model>();
	tempModel->Initialize(L"Test");
	m_UniqueModels.insert(std::pair(L"Test", tempModel));

	DirectX::XMVECTOR pos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR rot = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR scale = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	std::shared_ptr<VertexObject> tempObject = std::make_shared<VertexObject>();
	tempObject->Initialize(
		std::move(tempModel),
		pos,
		rot,
		scale
	);
	std::vector<std::shared_ptr<VertexObject>> objects = {};
	objects.push_back(std::move(tempObject));
	m_Objects.insert(std::pair(L"Test", objects));
}

void Scene::LoadModel()
{
	//Transition the vertex & index buffer resources after creation.
}