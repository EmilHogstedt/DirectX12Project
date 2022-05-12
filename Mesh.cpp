#include "pch.h"
#include "Mesh.h"

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices) noexcept
{
	m_VertexCount = vertices.size();
	m_IndexCount = indices.size();
	Microsoft::WRL::ComPtr<ID3D12Heap> pVBHeap{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12Heap> pIBHeap{ nullptr };

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 0u;
	heapProperties.VisibleNodeMask = 0u;

	D3D12_HEAP_DESC heapDescriptor = {};
	heapDescriptor.SizeInBytes = sizeof(Vertex) * vertices.size();
	heapDescriptor.Properties = heapProperties;
	heapDescriptor.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	heapDescriptor.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES;
	HR(DXCore::GetDevice()->CreateHeap(&heapDescriptor, IID_PPV_ARGS(&pVBHeap)));

	D3D12_RESOURCE_DESC resourceDescriptor = {};
	resourceDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDescriptor.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDescriptor.Width = sizeof(Vertex) * vertices.size();
	resourceDescriptor.Height = 1u;
	resourceDescriptor.DepthOrArraySize = 1u;
	resourceDescriptor.MipLevels = 1u;
	resourceDescriptor.Format = DXGI_FORMAT_UNKNOWN;
	resourceDescriptor.SampleDesc = { 1u, 0u };
	resourceDescriptor.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDescriptor.Flags = D3D12_RESOURCE_FLAG_NONE;
	HR(DXCore::GetDevice()->CreatePlacedResource
	(
		pVBHeap.Get(),
		0u,
		&resourceDescriptor,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_pVertexBuffer)
	));

	auto pUploadBuffer = DXCore::GetUploadBuffer();

	D3D12_RANGE nullRange = { 0,0 };
	unsigned char* mappedPtr = nullptr;

	HR(pUploadBuffer->Map(0u, &nullRange, reinterpret_cast<void**>(&mappedPtr)));
	std::memcpy(mappedPtr, reinterpret_cast<unsigned char*>(vertices.data()), resourceDescriptor.Width);
	STDCALL(DXCore::GetCommandList()->CopyBufferRegion(m_pVertexBuffer.Get(), 0u, pUploadBuffer.Get(), 0u, resourceDescriptor.Width));
	STDCALL(pUploadBuffer->Unmap(0u, nullptr));
	mappedPtr = nullptr;

	heapDescriptor.SizeInBytes = sizeof(unsigned int) * indices.size();
	HR(DXCore::GetDevice()->CreateHeap(&heapDescriptor, IID_PPV_ARGS(&pIBHeap)));

	resourceDescriptor.Width = sizeof(unsigned int) * indices.size();
	HR(DXCore::GetDevice()->CreatePlacedResource
	(
		pIBHeap.Get(),
		0u,
		&resourceDescriptor,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_pIndexBuffer)
	));

	HR(pUploadBuffer->Map(0u, &nullRange, reinterpret_cast<void**>(&mappedPtr)));
	std::memcpy(static_cast<unsigned char*>(mappedPtr) + (sizeof(Vertex) * vertices.size()), reinterpret_cast<unsigned char*>(indices.data()), resourceDescriptor.Width);
	STDCALL(DXCore::GetCommandList()->CopyBufferRegion(m_pIndexBuffer.Get(), 0u, pUploadBuffer.Get(), sizeof(Vertex) * vertices.size(), resourceDescriptor.Width));
	STDCALL(pUploadBuffer->Unmap(0u, nullptr));
	mappedPtr = nullptr;

	RenderCommand::TransitionResource(m_pVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	RenderCommand::TransitionResource(m_pIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}
