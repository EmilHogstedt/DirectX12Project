#pragma once
#include "DescriptorHeap.h"
#include "Triangle.h"

class Camera;
struct ColorData
{
	DirectX::XMVECTORF32 Color;
};

struct WVP
{
	DirectX::XMFLOAT4X4 WVPMatrix;
};

class Renderer
{
public:
	Renderer() noexcept = default;
	~Renderer() noexcept = default;
	void Initialize() noexcept;
	void Begin(Camera* const pCamera) noexcept;
	void Submit(/*Objects...*/) noexcept;
	void End() noexcept;
	void OnShutDown() noexcept;
private:
	void CreateDepthBuffer() noexcept;
	void CreateRootSignature() noexcept;
	void CreatePipelineStateObject() noexcept;
	void CreateViewportAndScissorRect() noexcept;
private:
	uint32_t m_CurrentBackBufferIndex{0u};
	uint64_t m_FrameFenceValues[NR_OF_FRAMES];
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPSO;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pDepthBuffer;
	DescriptorHeap m_DSVDescriptorHeap;

	D3D12_VIEWPORT m_ViewPort;
	RECT m_ScissorRect;
	std::unique_ptr<Triangle> m_pTriangle;
};