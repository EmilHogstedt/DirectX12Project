#include "pch.h"
#include "Engine.h"
#include "DXCore.h"
#include "Window.h"

void Engine::Initialize(const std::wstring& applicationName) noexcept
{
	CreateConsole();
	DXCore::Initialize();
	Window::Get().Initialize(applicationName);

	auto& memoryManager = MemoryManager::Get();
	memoryManager.CreateShaderVisibleDescriptorHeap("ShaderBindables", 200'000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	memoryManager.AddRangeForDescriptor("ShaderBindables", "TransformsRange", 25'000);
	memoryManager.CreateNonShaderVisibleDescriptorHeap("Transforms", 25'000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_pScene = std::make_unique<Scene>();
	m_pScene->Initialize();
	m_pRenderer = std::make_unique<Renderer>();
	m_pRenderer->Initialize();

	DirectX::XMFLOAT3 cameraStartPosition = DirectX::XMFLOAT3(0.0f, 0.0f, -5.0f);
	auto [width, height] = Window::Get().GetDimensions();
	m_pCamera = std::make_unique<Camera>(cameraStartPosition, width, height);
}

void Engine::Run() noexcept
{
	static Window& s_Window{ Window::Get() };

	float deltaTime = 0.0f;
	auto lastFrameEnd = std::chrono::system_clock::now();
	uint64_t frameCount = 0u;
	bool startProfiling = false;
	while (s_Window.IsRunning())
	{
		{
			if (startProfiling)
				Profiler profiler(__FUNCTION__, [&](ProfilerData profilerData) {ProfilerManager::ProfilerDatas.emplace_back(profilerData); });

			m_pRenderer->Begin(m_pCamera.get(), m_pScene->GetAccelerationStructureGPUAddress());
			m_pRenderer->Submit(m_pScene->GetCulledVertexObjects(), deltaTime);
			m_pRenderer->End();
		}
		m_pCamera->Update(deltaTime);
		s_Window.OnUpdate();

		auto currentFrameEnd = std::chrono::system_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(currentFrameEnd - lastFrameEnd).count();
		deltaTime = elapsed / 1'000'000.0f;
		lastFrameEnd = currentFrameEnd;

		frameCount++;
		if (frameCount == 2000 && startProfiling == false)
		{
			startProfiling = true;
			frameCount = 0u;
		}
		else if (frameCount == 2'000 && startProfiling == true)
		{
			ProfilerManager::Clear();
			frameCount = 0u;
		}
	}
	m_pRenderer->OnShutDown();
}

void Engine::CreateConsole() noexcept
{
	AllocConsole() && "Unable to allocate console";
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
}
