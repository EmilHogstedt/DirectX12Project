#include "pch.h"
#include "Renderer.h"
#include "DXCore.h"
#include "Window.h"
#include "RenderCommand.h"

void Renderer::Initialize() noexcept
{
	DXCore::GetCommandList()->Close();
}

void Renderer::Begin() noexcept
{
	auto pCommandAllocator = DXCore::GetCommandAllocators()[m_CurrentBackBufferIndex];
	auto pBackBuffer = Window::Get().GetBackBuffers()[m_CurrentBackBufferIndex];
	DescriptorHeap backBufferRTVDescHeap = Window::Get().GetBackBufferRTVHeap();
	auto pCommandList = DXCore::GetCommandList();

	pCommandAllocator->Reset();
	pCommandList->Reset(pCommandAllocator.Get(), nullptr);

	//Clear current back buffer:
	{
		RenderCommand::TransitionResource(pBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		static FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		auto backBufferDescriptorHandle = backBufferRTVDescHeap.GetCPUStartHandle();
		backBufferDescriptorHandle.ptr += m_CurrentBackBufferIndex * backBufferRTVDescHeap.GetDescriptorTypeSize();
		RenderCommand::ClearRenderTarget(backBufferDescriptorHandle, clearColor);
	}
}

void Renderer::Submit() noexcept
{

}

void Renderer::End() noexcept
{
	auto pCommandList = DXCore::GetCommandList();
	auto pBackBuffer = Window::Get().GetBackBuffers()[m_CurrentBackBufferIndex];

	//Present:
	{
		RenderCommand::TransitionResource(pBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		HR(pCommandList->Close());

		ID3D12CommandList* const commandLists[] = { pCommandList.Get() };
		STDCALL(DXCore::GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists));

		m_FrameFenceValues[m_CurrentBackBufferIndex] =  RenderCommand::SignalFenceFromGPU();

		Window::Get().Present();

		m_CurrentBackBufferIndex = Window::Get().GetCurrentBackbufferIndex();

		RenderCommand::WaitForFenceValue(m_FrameFenceValues[m_CurrentBackBufferIndex]);
	}
}

void Renderer::OnShutDown() noexcept
{
	RenderCommand::Flush();
}
