#include "pch.h"
#include "Engine.h"
#include "DXCore.h"
#include "Window.h"

void Engine::Initialize(const std::wstring& applicationName) noexcept
{
#if defined(_DEBUG)
	CreateConsole();
#endif
	DXCore::Initialize();
	Window::Get().Initialize(applicationName);

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

	while (s_Window.IsRunning())
	{
		m_pRenderer->Begin(m_pCamera.get());
		m_pRenderer->Submit(m_pScene->GetCulledVertexObjects(), m_pCamera.get());
		m_pRenderer->End();
		
		m_pCamera->Update(deltaTime);
		s_Window.OnUpdate();

		auto currentFrameEnd = std::chrono::system_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(currentFrameEnd - lastFrameEnd).count();
		deltaTime = elapsed / 1'000'000.0f;
		lastFrameEnd = currentFrameEnd;
	}
	m_pRenderer->OnShutDown();
}

#if defined(_DEBUG)
void Engine::CreateConsole() noexcept
{
	assert(AllocConsole() && "Unable to allocate console");
	assert(freopen("CONIN$", "r", stdin));
	assert(freopen("CONOUT$", "w", stdout));
	assert(freopen("CONOUT$", "w", stderr));
}
#endif
