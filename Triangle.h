#pragma once
class Triangle
{
public:
	Triangle() noexcept;
	~Triangle() noexcept = default;
	[[nodiscard]] constexpr uint32_t GetNrOfIndices() noexcept { return m_NrOfIndices; }
	[[nodiscard]] constexpr Microsoft::WRL::ComPtr<ID3D12Resource>& GetVertexBuffer() noexcept { return m_pVertexBuffer; }
	[[nodiscard]] constexpr Microsoft::WRL::ComPtr<ID3D12Resource>& GetIndexBuffer() noexcept { return m_pIndexBuffer; }
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBuffer;
	uint32_t m_NrOfIndices;
};