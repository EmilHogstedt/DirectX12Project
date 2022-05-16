#pragma once
#include "Model.h"
#include "MemoryManager.h"

class VertexObject
{
public:
	VertexObject() noexcept = default;
	~VertexObject() noexcept = default;

	void Initialize(std::shared_ptr<Model> objectModel, DirectX::XMVECTOR pos, DirectX::XMVECTOR rot, DirectX::XMVECTOR scale);

	void Update(float deltaTime);

	const DirectX::XMFLOAT4X4& GetTransform() const { return m_Transform; }
	const std::shared_ptr<Model>& GetModel() const { return m_pModel; }
	void SetTransform(const DirectX::XMFLOAT4X4& transform) noexcept { m_Transform = transform; }
	[[nodiscard]] constexpr ConstantBufferView& GetTransformConstantBufferView() { return m_TransformConstantBufferView; }
private:
	void SetConstantBufferView(const ConstantBufferView& transformBuffer) noexcept { m_TransformConstantBufferView = transformBuffer; }
private:
	DirectX::XMFLOAT4X4 m_Transform = {};
	ConstantBufferView m_TransformConstantBufferView;
	std::shared_ptr<Model> m_pModel = nullptr;
};