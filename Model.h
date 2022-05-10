#pragma once

//Vertex struct for colors. We do not handle textures.
struct Vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT4 color; //w-value is opacity
	DirectX::XMFLOAT3 normal;
};



class Model
{
public:
	Model() noexcept = default;
	~Model() noexcept = default;

	void Initialize(std::wstring path);

public:
	const D3D12_GPU_VIRTUAL_ADDRESS GetVertexBufferGPUAddress() const noexcept {
		DBG_ASSERT(m_pVertexBuffer, "Error! Trying to get a vertex buffer's GPU address while it has not been set.");
		return m_pVertexBuffer->GetGPUVirtualAddress();
	}
	const D3D12_GPU_VIRTUAL_ADDRESS GetIndexBufferGPUAddress() const noexcept {
		DBG_ASSERT(m_pVertexBuffer, "Error! Trying to get an index buffer's GPU address while it has not been set.");
		return m_pIndexBuffer->GetGPUVirtualAddress();
	}
	const std::wstring& GetName() const noexcept {
		return m_name;
	}
	const uint64_t GetVertexCount() const noexcept { return m_vertexCount; }
	const uint64_t GetIndexCount() const noexcept { return m_indexCount; }
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBuffer = nullptr;

	uint64_t m_vertexCount = 0u;
	uint64_t m_indexCount = 0u;

	std::wstring m_name = L"";
};

//MOVE THIS TO OBJECTHANDLER!!!
struct ModelInfo
{
	std::unique_ptr<Model> m_model;
	std::vector<DirectX::XMFLOAT4X4> m_transforms; //REPLACE THIS WITH A VECTOR OF OBJECTS INSTEAD!!!
};