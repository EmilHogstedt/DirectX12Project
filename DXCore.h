#pragma once
class DXCore
{
public:
	DXCore() noexcept = default;
	~DXCore() noexcept = default;
	static void Initialize() noexcept;
	[[nodiscard]] static Microsoft::WRL::ComPtr<IDXGIFactory6> CreateFactory() noexcept;
	[[nodiscard]] static constexpr Microsoft::WRL::ComPtr<ID3D12Device5>& GetDevice() noexcept { return m_pDevice; }
	[[nodiscard]] static constexpr Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& GetCommandAllocator() noexcept { return m_pCommandAllocator; }
	[[nodiscard]] static constexpr Microsoft::WRL::ComPtr<ID3D12CommandQueue>& GetCommandQueue() noexcept { return m_pCommandQueue; }
private:
	static void CreateDebugAndGPUValidationLayer() noexcept;
	static void InitializeDevice() noexcept;
	static void InitializeCommandInterfaces() noexcept;
	static Microsoft::WRL::ComPtr<IDXGIAdapter1> CreateAdapter(const Microsoft::WRL::ComPtr<IDXGIFactory6> pFactory) noexcept;
	static void CheckSupportForDXR(Microsoft::WRL::ComPtr<ID3D12Device> pDevice) noexcept;
private:
	static Microsoft::WRL::ComPtr<ID3D12Device5> m_pDevice;
	static Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	static Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
	static Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_pCommandList;
	static Microsoft::WRL::ComPtr<IDXGIAdapter> m_pAdapter;
};

