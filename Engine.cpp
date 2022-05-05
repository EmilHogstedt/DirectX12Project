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
	m_pRenderer = std::make_unique<Renderer>();
	m_pRenderer->Initialize();
}

void Engine::Run() noexcept
{
	static Window& s_Window{ Window::Get() };
	while (s_Window.IsRunning())
	{
		//Rendering: We *COULD* have a scene that sets everything up and just feeds to Submit-function...
		m_pRenderer->Begin();
		m_pRenderer->Submit(/*Objects...*/);
		m_pRenderer->End();

		s_Window.OnUpdate();
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
