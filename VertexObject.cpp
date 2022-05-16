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
	SetConstantBufferView(std::move(MemoryManager::Get().CreateConstantBuffer("Transforms", "ShaderBindables", "TransformsRange", sizeof(DirectX::XMFLOAT4X4))));
}

void VertexObject::Update(float deltaTime)
{
	static const float speed = 1.0f;

	auto m = DirectX::XMLoadFloat4x4(&m_Transform);
	DirectX::XMVECTOR scale;
	DirectX::XMVECTOR rotationQuat;
	DirectX::XMVECTOR translation;
	DirectX::XMMatrixDecompose(&scale, &rotationQuat, &translation, m);

	DirectX::XMFLOAT3 scaleF;
	DirectX::XMStoreFloat3(&scaleF, scale);
	DirectX::XMFLOAT3 translationF;
	DirectX::XMStoreFloat3(&translationF, translation);
	translationF.x += (speed * deltaTime);

	float angleX = 0.0f;
	float angleY = 0.0f;
	float angleZ = 0.0f;
	m = DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&scaleF)) * DirectX::XMMatrixRotationX(angleX) * DirectX::XMMatrixRotationY(angleY) * DirectX::XMMatrixRotationZ(angleZ) * DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&translationF));

	DirectX::XMFLOAT4X4 tempTransform;
	DirectX::XMStoreFloat4x4(&tempTransform, m);
	SetTransform(tempTransform);

	m = DirectX::XMMatrixTranspose(m);
	MemoryManager::Get().UpdateConstantBuffer(m_TransformConstantBufferView, &m, sizeof(DirectX::XMFLOAT4X4));
}
