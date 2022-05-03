#include "pch.h"
#include "Engine.h"
#include "DXCore.h"

void Engine::Initialize(const std::wstring& applicationName) noexcept
{
#if defined(_DEBUG)
	CreateConsole();
#endif
	DXCore::Initialize();
}

void Engine::Run() noexcept
{
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
