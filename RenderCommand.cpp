#include "pch.h"
#include "RenderCommand.h"
#include "DXCore.h"

uint64_t RenderCommand::s_FenceValue{ 0u };

void RenderCommand::TransitionResource(const Microsoft::WRL::ComPtr<ID3D12Resource> pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) noexcept
{
	D3D12_RESOURCE_BARRIER resourceTransitionBarrier = {};
	resourceTransitionBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resourceTransitionBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	resourceTransitionBarrier.Transition.pResource = pResource.Get();
	resourceTransitionBarrier.Transition.StateBefore = stateBefore;
	resourceTransitionBarrier.Transition.StateAfter = stateAfter;
	resourceTransitionBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	STDCALL(DXCore::GetCommandList()->ResourceBarrier(1u, &resourceTransitionBarrier));
}

void RenderCommand::ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorhandle, FLOAT* clearColor) noexcept
{
	STDCALL(DXCore::GetCommandList()->ClearRenderTargetView(cpuDescriptorhandle, clearColor, 0u, nullptr));
}

uint64_t RenderCommand::SignalFenceFromGPU() noexcept
{
	s_FenceValue++;
	HR(DXCore::GetCommandQueue()->Signal(DXCore::GetFence().Get(), s_FenceValue));
	return s_FenceValue;
}

void RenderCommand::WaitForFenceValue(uint64_t fenceValue) noexcept
{
	if (DXCore::GetFence()->GetCompletedValue() < fenceValue)
	{
		HR(DXCore::GetFence()->SetEventOnCompletion(fenceValue, DXCore::GetFenceEvent()));
		DWORD retVal{ ::WaitForSingleObject(DXCore::GetFenceEvent(), INFINITE) };
		DBG_ASSERT(retVal != WAIT_FAILED, "Failed to wait for fence event completion.");
	}
}

void RenderCommand::Flush() noexcept
{
	uint64_t fenceValue = SignalFenceFromGPU();
	WaitForFenceValue(fenceValue);
}
