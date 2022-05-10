#pragma once
#include "Model.h"

class VertexObject
{
public:
	VertexObject() noexcept = default;
	~VertexObject() noexcept = default;

	void Initialize(std::shared_ptr<Model> objectModel, DirectX::XMVECTOR pos, DirectX::XMVECTOR rot, DirectX::XMVECTOR scale);

	void Update();

	const DirectX::XMFLOAT4X4& GetTransform() const { return m_Transform; }
private:
	DirectX::XMFLOAT4X4 m_Transform = {};
	std::shared_ptr<Model> m_pModel = nullptr;
};