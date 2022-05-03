#include "pch.h"
#include "DXCore.h"

Microsoft::WRL::ComPtr<ID3D12Device5> DXCore::m_pDevice{ nullptr };
Microsoft::WRL::ComPtr<ID3D12CommandQueue> DXCore::m_pCommandQueue{ nullptr };
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> DXCore::m_pCommandAllocator{ nullptr };
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> DXCore::m_pCommandList{ nullptr };
Microsoft::WRL::ComPtr<IDXGIAdapter> DXCore::m_pAdapter{ nullptr };

void DXCore::Initialize() noexcept
{
#if defined(_DEBUG)
	CreateDebugAndGPUValidationLayer();
#endif

	InitializeDevice();

#if defined(_DEBUG)
	InitializeInfoQueue(m_pDevice);
#endif
	//Can use HRI-Macro from now on
	InitializeCommandInterfaces();

}

void DXCore::CreateDebugAndGPUValidationLayer() noexcept
{
	Microsoft::WRL::ComPtr<ID3D12Debug1> pDebugController{ nullptr };
	HR(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController)));
	pDebugController->EnableDebugLayer();
	pDebugController->SetEnableGPUBasedValidation(TRUE);
}

void DXCore::InitializeDevice() noexcept
{
	Microsoft::WRL::ComPtr<IDXGIFactory6> pFactory = std::move(CreateFactory());
	m_pAdapter = std::move(CreateAdapter(pFactory));

	Microsoft::WRL::ComPtr<ID3D12Device> pTempDevice{ nullptr };
	D3D12CreateDevice(m_pAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&pTempDevice));

	CheckSupportForDXR(pTempDevice);

	HR(pTempDevice->QueryInterface(__uuidof(ID3D12Device5), reinterpret_cast<void**>(m_pDevice.GetAddressOf())));
	HR(m_pDevice->SetName(L"Main Device"));
}

void DXCore::InitializeCommandInterfaces() noexcept
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = 0;
	commandQueueDesc.NodeMask = 0;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	HRI(m_pDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_pCommandQueue)));
	HRI(m_pCommandQueue->SetName(L"Main Command Queue"));

	HRI(m_pDevice->CreateCommandAllocator(commandQueueDesc.Type, IID_PPV_ARGS(&m_pCommandAllocator)));
	HRI(m_pCommandAllocator->SetName(L"Main Command Allocator"));

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pTempCommandList{ nullptr };

	HRI(m_pDevice->CreateCommandList(0u, commandQueueDesc.Type, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&pTempCommandList)));
	HRI(pTempCommandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList4), reinterpret_cast<void**>(m_pCommandList.GetAddressOf())));
	HRI(m_pCommandList->SetName(L"Main Command List"));
}

Microsoft::WRL::ComPtr<IDXGIFactory6> DXCore::CreateFactory() noexcept
{
	Microsoft::WRL::ComPtr<IDXGIFactory6> pFactory{ nullptr };
	uint32_t factoryFlag = 0u;
#if defined(_DEBUG)
	factoryFlag |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	HR(CreateDXGIFactory2(factoryFlag, IID_PPV_ARGS(&pFactory)));

	return pFactory;
}

Microsoft::WRL::ComPtr<IDXGIAdapter1> DXCore::CreateAdapter(const Microsoft::WRL::ComPtr<IDXGIFactory6> pFactory) noexcept
{
	Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter{ nullptr };
	bool adapterFound{ false };
	for (uint32_t i{ 0u };
		DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&pAdapter));
		++i)
	{
		DXGI_ADAPTER_DESC1 adapterDesc = {};
		pAdapter->GetDesc1(&adapterDesc);
		if (adapterDesc.Flags == DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		Microsoft::WRL::ComPtr<ID3D12Device5> pTempDevice{ nullptr };
		if (S_OK == D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device5), reinterpret_cast<void**>(pTempDevice.GetAddressOf())))
		{
			adapterFound = true;
			break;
		}
	}
	DBG_ASSERT(adapterFound, "Unable to create adapter.");

	return pAdapter;
}

void DXCore::CheckSupportForDXR(Microsoft::WRL::ComPtr<ID3D12Device> pDevice) noexcept
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureData = {};
	HR(pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5)));
	//Change to 1.1 to enable inline Ray tracing: 
	DBG_ASSERT(featureData.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0, "Ray Tracing 1.1 is not supported on current device");
}