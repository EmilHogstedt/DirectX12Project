#include "pch.h"
#include "Model.h"

void Model::Initialize(const std::string path) noexcept
{
	//Use assimp to load the model.

	m_Name = path;

	LoadModel();
}

void Model::LoadModel() noexcept
{
	Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(m_Name, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);
	DBG_ASSERT(pScene, "Error! Could not read .obj file.");

	ProcessNode(pScene->mRootNode, pScene);
}

void Model::ProcessNode(aiNode* node, const aiScene* scene) noexcept
{
	for (uint32_t i{ 0u }; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh);
	}

	for (uint32_t i{ 0u }; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene);
	}
}

void Model::ProcessMesh(aiMesh* mesh)
{
	std::vector<Vertex> vertices = {};
	std::vector<uint32_t> indices = {};

	for (uint32_t i{ 0u }; i < mesh->mNumVertices; i++)
	{
		Vertex vertex = {};
		vertex.pos.x = mesh->mVertices[i].x;
		vertex.pos.y = mesh->mVertices[i].y;
		vertex.pos.z = mesh->mVertices[i].z;
		
		/*
		Handle textures.
		if (mesh->mTextureCoords[0])
		{
			
		}
		*/
		//Setting color to red.
		vertex.color.x = 1.0f;
		vertex.color.y = 0.0f;
		vertex.color.z = 0.0f;
		vertex.color.w = 1.0f;

		vertex.normal.x = 0.0f;
		vertex.normal.y = 0.0f;
		vertex.normal.z = 0.0f;

		vertices.push_back(vertex);
	}

	for (uint32_t i{ 0u }; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		for (uint32_t j{ 0u }; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	m_Meshes.push_back(std::make_unique<Mesh>(vertices, indices));
}