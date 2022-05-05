#pragma once
class RenderCommand
{
public:
	static void TransitionResource(const Microsoft::WRL::ComPtr<ID3D12Resource> pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) noexcept;
	static void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorhandle, FLOAT* clearColor) noexcept;
	[[nodiscard]] static uint64_t SignalFenceFromGPU() noexcept;
	static void WaitForFenceValue(uint64_t fenceValue) noexcept;
	static void Flush() noexcept;
private:
	RenderCommand() noexcept = default;
	~RenderCommand() noexcept = default;

private:
	static uint64_t s_FenceValue;
};