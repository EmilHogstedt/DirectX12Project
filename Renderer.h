#pragma once
class Renderer
{
public:
	Renderer() noexcept = default;
	~Renderer() noexcept = default;
	void Initialize() noexcept;
	void Begin() noexcept;
	void Submit(/*Objects...*/) noexcept;
	void End() noexcept;
};