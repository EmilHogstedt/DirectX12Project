#include "pch.h"
#include "Model.h"

void Model::Initialize(std::wstring path)
{
	//Use assimp to load the model.

	m_name = path;

	std::unique_ptr<Triangle> tempTri = std::make_unique<Triangle>();

	m_pVertexBuffer = tempTri->GetVertexBuffer();
	m_pIndexBuffer = tempTri->GetIndexBuffer();
	m_indexCount = tempTri->GetNrOfIndices();
	m_vertexCount = tempTri->GetNrOfVertices();

	RenderCommand::TransitionResource(m_pVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	RenderCommand::TransitionResource(m_pIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}
