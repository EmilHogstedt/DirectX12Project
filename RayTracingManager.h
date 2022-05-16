#pragma once
#include "DXHelper.h"
#include "DXCore.h"
#include "VertexObject.h"

class RayTracingManager
{
public:
	RayTracingManager() noexcept = default;
	~RayTracingManager() noexcept = default;

	void Initialize(
		const std::unordered_map<std::wstring, std::shared_ptr<Model>>& models,
		const std::unordered_map<std::wstring, std::vector<std::shared_ptr<VertexObject>>>& objects,
		uint32_t totalNrObjects
	) noexcept;

	void Refit() noexcept;
	void Rebuild() noexcept;

private:
	void BuildBottomAcceleration(
		const std::unordered_map<std::wstring, std::shared_ptr<Model>>& models
	) noexcept;
	void BuildTopAcceleration(
		const std::unordered_map<std::wstring, std::shared_ptr<Model>>& models,
		const std::unordered_map<std::wstring, std::vector<std::shared_ptr<VertexObject>>>& objects,
		uint32_t totalNrObjects
	) noexcept;

	void CreateCommitedBuffer(
		std::wstring bufferName,
		Microsoft::WRL::ComPtr<ID3D12Resource>& buffer,
		D3D12_HEAP_TYPE heapType,
		uint64_t bufferSize,
		D3D12_RESOURCE_FLAGS flags,
		D3D12_RESOURCE_STATES initialState
	) noexcept;

private:
	std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D12Resource>> m_ResultBuffersBottom = {};
	std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D12Resource>> m_ScratchBuffersBottom = {};
	std::unordered_map<std::wstring, std::shared_ptr<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>> m_AccelerationDescsBottom = {};

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pInstanceBufferTop = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pResultBufferTop = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pScratchBufferTop = nullptr;
	std::unique_ptr<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC> m_AccelerationDescTop = nullptr;
};