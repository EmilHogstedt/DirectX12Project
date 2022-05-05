#pragma once
#include "DXHelper.h"

#include "Model.h"

class RayTracingManager
{
public:
	RayTracingManager() noexcept = default;
	~RayTracingManager() noexcept = default;

	void Initialize(
		Microsoft::WRL::ComPtr<ID3D12Device5> pDevice,
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> pCommandList,
		std::vector<std::unique_ptr<ModelInfo>> models,
		uint32_t totalNrObjects
	) noexcept;

	void Refit() noexcept;
	void Rebuild() noexcept;

private:
	void BuildBottomAcceleration(
		const std::vector<std::unique_ptr<ModelInfo>>& models
	) noexcept;
	void BuildTopAcceleration(
		const std::vector<std::unique_ptr<ModelInfo>>& models,
		uint32_t totalNrObjects
	) noexcept;

	void BuildStructure(
		std::wstring resultName,
		Microsoft::WRL::ComPtr<ID3D12Resource> resultBuffer,
		uint64_t resultSize,
		std::wstring scratchName,
		Microsoft::WRL::ComPtr<ID3D12Resource> scratchBuffer,
		uint64_t scratchSize,
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs
	) noexcept;
	void CreateCommitedBuffer(
		std::wstring bufferName,
		Microsoft::WRL::ComPtr<ID3D12Resource> buffer,
		D3D12_HEAP_TYPE heapType,
		uint64_t bufferSize,
		D3D12_RESOURCE_FLAGS flags,
		D3D12_RESOURCE_STATES initialState
	) noexcept;

private:
	Microsoft::WRL::ComPtr<ID3D12Device5> m_pDevice = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_pCommandList = nullptr;

	std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D12Resource>> m_ResultBuffersBottom;
	std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D12Resource>> m_ScratchBuffersBottom;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pInstanceBufferTop = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pResultBufferTop = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pScratchBufferTop = nullptr;
};