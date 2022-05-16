#pragma once
#include "RayTracingManager.h"
#include "DXCore.h"
#include "RenderCommand.h"

class Scene
{
public:
	Scene() noexcept = default;
	~Scene() noexcept = default;

	//Initializes all objects in the scene.
	void Initialize();

	const std::unordered_map<std::wstring, std::vector<std::shared_ptr<VertexObject>>>& GetCulledVertexObjects() const { return m_Objects; }
private:
	void AddVertexObject();
	void LoadModel();
private:
	std::unique_ptr<RayTracingManager> m_pRayTracingManager = nullptr;

	uint32_t m_TotalObjects = 0u;
	//Key is the path to the model.
	//First unordered map holds all unique models.
	//Second unordered map has the same key, but holds a vector with all the objects that use that model.
	std::unordered_map<std::wstring, std::shared_ptr<Model>> m_UniqueModels = {};
	std::unordered_map<std::wstring, std::vector<std::shared_ptr<VertexObject>>> m_Objects = {};

	//Add corresponding unordered maps for arbitrary geometry.
};