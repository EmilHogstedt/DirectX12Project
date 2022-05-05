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
}

void Engine::Run() noexcept
{
	static Window& s_Window{ Window::Get() };
	while (s_Window.IsRunning())
	{
		s_Window.OnUpdate();
	}

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
