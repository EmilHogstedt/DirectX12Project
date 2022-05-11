#pragma once
#include "Renderer.h"
#include "Camera.h"
class Engine
{
public:
	Engine() noexcept = default;
	~Engine() noexcept = default;
	void Initialize(const std::wstring& applicationName) noexcept;
	void Run() noexcept;
#if defined(_DEBUG)
private:
	void CreateConsole() noexcept;
#endif
private:
	std::wstring m_AppName;
	std::unique_ptr<Renderer> m_pRenderer;
	std::unique_ptr<Camera> m_pCamera;
};