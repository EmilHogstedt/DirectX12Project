#pragma once
class DescriptorHeap
{
public:
	DescriptorHeap(uint32_t capacity, D3D12_DESCRIPTOR_HEAP_TYPE heapType, bool shaderVisible) noexcept;
	DescriptorHeap() noexcept = default;
	~DescriptorHeap() noexcept = default;
	[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE GetCPUStartHandle() noexcept { return m_CPUAdressStart; }
	[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentCPUOffsetHandle() noexcept { return m_CPUCurrentAdressOffset; }
	void OffsetAddressPointerBy(uint32_t offset) noexcept;
private:
	uint32_t m_Capacity;
	uint32_t m_Size;
	D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;
	bool m_IsShaderVisible;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap;
	uint32_t m_IncrementSize;
	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUAdressStart;
	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUCurrentAdressOffset;
};