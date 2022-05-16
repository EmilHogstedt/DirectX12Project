#include "pch.h"
#include "Model.h"

void Model::Initialize(std::wstring path)
{
	//Use assimp to load the model.

	m_name = path;

	m_pTriangle = std::make_unique<Triangle>();

	m_pVertexBuffer = m_pTriangle->GetVertexBuffer();
	m_pIndexBuffer = m_pTriangle->GetIndexBuffer();
	m_indexCount = m_pTriangle->GetNrOfIndices();
	m_vertexCount = m_pTriangle->GetNrOfVertices();

	RenderCommand::TransitionResource(m_pVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	RenderCommand::TransitionResource(m_pIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}
