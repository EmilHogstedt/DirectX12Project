#include "pch.h"
#include "VertexObject.h"

void VertexObject::Initialize(std::shared_ptr<Model> objectModel, DirectX::XMVECTOR pos, DirectX::XMVECTOR rot, DirectX::XMVECTOR scale)
{
	m_pModel = objectModel;
	
	DirectX::XMMATRIX tempPosMatrix = {};
	tempPosMatrix = DirectX::XMMatrixTranslationFromVector(pos);
	DirectX::XMMATRIX tempRotMatrix = {};
	tempRotMatrix = DirectX::XMMatrixRotationRollPitchYawFromVector(rot);
	DirectX::XMMATRIX tempScaleMatrix = {};
	tempScaleMatrix = DirectX::XMMatrixScalingFromVector(scale);
	DirectX::XMStoreFloat4x4(&m_Transform, tempScaleMatrix * tempRotMatrix * tempPosMatrix);
}

void VertexObject::Update()
{
}
