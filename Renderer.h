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
	void OnShutDown() noexcept;
private:
	uint32_t m_CurrentBackBufferIndex{0u};
	uint64_t m_FrameFenceValues[NR_OF_FRAMES];
};