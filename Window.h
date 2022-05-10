#pragma once
#include "DescriptorHeap.h"
class Window
{
public:
	[[nodiscard]] static constexpr Window& Get() noexcept { return s_Instance; }
	void Initialize(const std::wstring& applicationName, uint8_t framesInFlight = 3u) noexcept;
	void OnUpdate() noexcept;
	void Present() noexcept;
	[[nodiscard]] constexpr bool IsRunning() noexcept { return m_IsRunning; }
	[[nodiscard]] constexpr Microsoft::WRL::ComPtr<ID3D12Resource>* GetBackBuffers() noexcept { return m_pBackBuffers; }
	[[nodiscard]] constexpr DescriptorHeap& GetBackBufferRTVHeap() noexcept { return m_BackBufferRTVHeap; }
	[[nodiscard]] constexpr uint8_t GetCurrentBackbufferIndex() noexcept { return static_cast<uint8_t>(m_pSwapChain->GetCurrentBackBufferIndex()); }
	[[nodiscard]] constexpr std::pair<uint32_t, uint32_t> GetDimensions() noexcept { return std::make_pair(m_Width, m_Height); }
private:
	Window() noexcept;
	~Window() noexcept = default;
	void CreateWindow(const std::wstring& applicationName) noexcept;
	void CreateSwapChain() noexcept;
	void CreateBackBufferRTVs();
private:
	static Window s_Instance;
	HWND m_WindowHandle;
	bool m_IsRunning;
	MSG m_Message;
	uint8_t m_NrOfFramesInFlight;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_pSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pBackBuffers[NR_OF_FRAMES];
	DescriptorHeap m_BackBufferRTVHeap;
	uint32_t m_Width;
	uint32_t m_Height;
};