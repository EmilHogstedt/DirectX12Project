#pragma once
#include "DescriptorHeap.h"
#include "Triangle.h"
#include "Scene.h"

class Camera;

struct VP
{
	DirectX::XMFLOAT4X4 VPMatrix;
};

struct InverseVP
{
	DirectX::XMFLOAT4X4 InverseVPMatrix;
	DirectX::XMFLOAT2 elementsP;
};

struct World
{
	DirectX::XMFLOAT4X4 WorldMatrix;
};

class Renderer
{
public:
	Renderer() noexcept = default;
	~Renderer() noexcept = default;
	void Initialize() noexcept;
	void Begin(Camera* const pCamera, D3D12_GPU_VIRTUAL_ADDRESS accelerationStructure) noexcept;
	void Submit(const std::unordered_map<std::string, std::vector<std::shared_ptr<VertexObject>>>& vertexObjects, float deltaTime) noexcept;
	void End() noexcept;
	void OnShutDown() noexcept;
private:
	void CreateDepthBuffer() noexcept;
	void CreateRootSignature() noexcept;
	void CreatePipelineStateObject() noexcept;
	void CreateViewportAndScissorRect() noexcept;

	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filepath, const std::wstring& entryPoint, const std::wstring& target) noexcept;
	Microsoft::WRL::ComPtr<ID3DBlob> LoadCSO(const std::wstring& filepath) noexcept;
private:
	uint32_t m_CurrentBackBufferIndex{0u};
	uint64_t m_FrameFenceValues[NR_OF_FRAMES];
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPSO;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pDepthBuffer;
	std::unique_ptr<DescriptorHeap> m_pDSVDescriptorHeap;

	D3D12_VIEWPORT m_ViewPort;
	RECT m_ScissorRect;
};