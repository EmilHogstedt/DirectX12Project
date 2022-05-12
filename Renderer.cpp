#include "pch.h"
#include "Renderer.h"
#include "DXCore.h"
#include "Window.h"
#include "RenderCommand.h"
#include "Camera.h"

void Renderer::Initialize() noexcept
{
	CreateDepthBuffer();
	CreateRootSignature();
	CreatePipelineStateObject();
	CreateViewportAndScissorRect();
	CreateAllDescriptorHeaps();
	CreateConstantBuffersForTriangle();
	m_pTriangle = std::make_unique<Triangle>();

	auto pCommandList = DXCore::GetCommandList();

	HR(pCommandList->Close());
	ID3D12CommandList* commandLists[] = { pCommandList.Get() };
	STDCALL(DXCore::GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists));
	RenderCommand::Flush();
	RenderCommand::ResetFenceValue();

	DirectX::XMStoreFloat4x4(&worldConstantBuffer.WorldMatrix, DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity()));
}

void Renderer::Begin(Camera* const pCamera) noexcept
{
	auto pCommandAllocator = DXCore::GetCommandAllocators()[m_CurrentBackBufferIndex];
	auto pCommandList = DXCore::GetCommandList();
	auto pBackBuffer = Window::Get().GetBackBuffers()[m_CurrentBackBufferIndex];

	DescriptorHeap backBufferRTVDescHeap = Window::Get().GetBackBufferRTVHeap();
	auto backBufferDescriptorHandle = backBufferRTVDescHeap.GetCPUStartHandle();
	backBufferDescriptorHandle.ptr += m_CurrentBackBufferIndex * backBufferRTVDescHeap.GetDescriptorTypeSize();
	auto depthBufferDSVHandle = m_pDSVDescriptorHeap->GetCPUStartHandle();

	HR(pCommandAllocator->Reset());
	HR(pCommandList->Reset(pCommandAllocator.Get(), nullptr));

	//Clear current back buffer & depth buffer:
	RenderCommand::TransitionResource(pBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	
	RenderCommand::ClearRenderTarget(backBufferDescriptorHandle, DirectX::Colors::Black);
	RenderCommand::ClearDepth(depthBufferDSVHandle, 1.0f);

	//Set back buffer RTV and depth buffer DSV:
	STDCALL(pCommandList->OMSetRenderTargets(1u, &backBufferDescriptorHandle, false, &depthBufferDSVHandle));

	//PSO and Root sig:
	STDCALL(pCommandList->SetPipelineState(m_pPSO.Get()));
	STDCALL(pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get()));
	STDCALL(pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST));

	STDCALL(pCommandList->RSSetViewports(1u, &m_ViewPort));
	STDCALL(pCommandList->RSSetScissorRects(1u, &m_ScissorRect));

	STDCALL(pCommandList->SetDescriptorHeaps(1u, m_pShaderBindableDescriptorHeap->GetInterface().GetAddressOf()));

	static VP vpMatrixCBuffer;
	auto vpMatrix = DirectX::XMLoadFloat4x4(&(pCamera->GetVPMatrix()));
	vpMatrix = DirectX::XMMatrixTranspose(vpMatrix);
	DirectX::XMStoreFloat4x4(&vpMatrixCBuffer.VPMatrix, vpMatrix);
	STDCALL(pCommandList->SetGraphicsRoot32BitConstants(3u, 4*4, &vpMatrixCBuffer, 0u));
}

void Renderer::Submit() noexcept
{
	auto pCommandList = DXCore::GetCommandList();
	auto index = Window::Get().GetCurrentBackbufferIndex();

	auto m = DirectX::XMLoadFloat4x4(&worldConstantBuffer.WorldMatrix);
	m *= DirectX::XMMatrixTranslation(1.0f, 0.0f, 0.0f);
	m = DirectX::XMMatrixTranspose(m);
	DirectX::XMStoreFloat4x4(&worldConstantBuffer.WorldMatrix, m);

	UpdateTriangleConstantBuffer(&worldConstantBuffer.WorldMatrix);

	auto gpuHandle = m_pShaderBindableDescriptorHeap->GetGPUStartHandle();
	gpuHandle.ptr += m_pShaderBindableDescriptorHeap->GetDescriptorTypeSize() * index * 20'000;
	STDCALL(pCommandList->SetGraphicsRootDescriptorTable(0, gpuHandle));

	STDCALL(pCommandList->SetGraphicsRootShaderResourceView(1u, m_pTriangle->GetVertexBuffer()->GetGPUVirtualAddress()));
	STDCALL(pCommandList->SetGraphicsRootShaderResourceView(2u, m_pTriangle->GetIndexBuffer()->GetGPUVirtualAddress()));
	STDCALL(pCommandList->DrawInstanced(m_pTriangle->GetNrOfIndices(), 1u, 0u, 0u));
}

void Renderer::End() noexcept
{
	auto pCommandList = DXCore::GetCommandList();
	auto pBackBuffer = Window::Get().GetBackBuffers()[m_CurrentBackBufferIndex];

	//Present:
	{
		RenderCommand::TransitionResource(pBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		HR(pCommandList->Close());

		ID3D12CommandList* commandLists[] = { pCommandList.Get() };
		STDCALL(DXCore::GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists));

		m_FrameFenceValues[m_CurrentBackBufferIndex] =  RenderCommand::SignalFenceFromGPU();
		Window::Get().Present();

		m_CurrentBackBufferIndex = Window::Get().GetCurrentBackbufferIndex();

		RenderCommand::WaitForFenceValue(m_FrameFenceValues[m_CurrentBackBufferIndex]);
	}
}

void Renderer::OnShutDown() noexcept
{
	RenderCommand::Flush();
}

void Renderer::CreateDepthBuffer() noexcept
{
	auto [width, height] {Window::Get().GetDimensions()};

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 0u;
	heapProperties.VisibleNodeMask = 0u;

	D3D12_RESOURCE_DESC DepthBufferResourceDesc{};
	DepthBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	DepthBufferResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	DepthBufferResourceDesc.Width = width;
	DepthBufferResourceDesc.Height = height;
	DepthBufferResourceDesc.DepthOrArraySize = 1u;
	DepthBufferResourceDesc.MipLevels = 1u;
	DepthBufferResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	DepthBufferResourceDesc.SampleDesc = { 1u, 0u };
	DepthBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	DepthBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;

	HR(DXCore::GetDevice()->CreateCommittedResource
	(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&DepthBufferResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&m_pDepthBuffer)
	));
	HR(m_pDepthBuffer->SetName(L"Main Depth Buffer"));

	m_pDSVDescriptorHeap = std::make_unique<DescriptorHeap>(1u, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);

	STDCALL(DXCore::GetDevice()->CreateDepthStencilView
	(
		m_pDepthBuffer.Get(), 
		nullptr, 
		m_pDSVDescriptorHeap->GetCPUStartHandle()
	));
}

void Renderer::CreateRootSignature() noexcept
{
	std::vector<D3D12_ROOT_PARAMETER> rootParameters;

	D3D12_DESCRIPTOR_RANGE descriptorRange = {};
	descriptorRange.BaseShaderRegister = 1u;
	descriptorRange.RegisterSpace = 0u;
	descriptorRange.NumDescriptors = 1u;
	descriptorRange.OffsetInDescriptorsFromTableStart = 0u;
	descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

	D3D12_ROOT_PARAMETER transformsRootParameterVS = {};
	transformsRootParameterVS.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	transformsRootParameterVS.DescriptorTable.NumDescriptorRanges = 1u;
	transformsRootParameterVS.DescriptorTable.pDescriptorRanges = &descriptorRange;
	transformsRootParameterVS.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters.push_back(transformsRootParameterVS);

	D3D12_ROOT_PARAMETER vertexBufferSRVParameter = {};
	vertexBufferSRVParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;		
	vertexBufferSRVParameter.Descriptor.ShaderRegister = 0u;					
	vertexBufferSRVParameter.Descriptor.RegisterSpace = 0u;						
	vertexBufferSRVParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters.push_back(vertexBufferSRVParameter);

	D3D12_ROOT_PARAMETER indexBufferSRVParameter = {};
	indexBufferSRVParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;		//The type is a SRV.
	indexBufferSRVParameter.Descriptor.ShaderRegister = 1u;						//Basically :register(1,0)
	indexBufferSRVParameter.Descriptor.RegisterSpace = 0u;						//^
	indexBufferSRVParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;	//This should be visible in the VERTEX SHADER.
	rootParameters.push_back(indexBufferSRVParameter);

	D3D12_ROOT_PARAMETER vpRootParameterVS = {};
	vpRootParameterVS.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	vpRootParameterVS.Constants.Num32BitValues = 4*4;
	vpRootParameterVS.Constants.ShaderRegister = 0u;
	vpRootParameterVS.Constants.RegisterSpace = 0u;
	vpRootParameterVS.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters.push_back(vpRootParameterVS);

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDescriptor = {};
	rootSignatureDescriptor.NumParameters = static_cast<UINT>(rootParameters.size());
	rootSignatureDescriptor.pParameters = rootParameters.data();
	rootSignatureDescriptor.NumStaticSamplers = 0u;
	rootSignatureDescriptor.pStaticSamplers = nullptr;
	rootSignatureDescriptor.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	Microsoft::WRL::ComPtr<ID3DBlob> pRootSignatureBlob = nullptr;
	SERIALIZE_ROOT_SIGNATURE(rootSignatureDescriptor, pRootSignatureBlob);
	HR(DXCore::GetDevice()->CreateRootSignature
	(
		0u,
		pRootSignatureBlob->GetBufferPointer(),
		pRootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&m_pRootSignature)
	));
}

void Renderer::CreatePipelineStateObject() noexcept
{
	//We need a rasterizer descriptor:
	D3D12_RASTERIZER_DESC rasterizerDescriptor = {};
	rasterizerDescriptor.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDescriptor.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDescriptor.FrontCounterClockwise = FALSE;
	rasterizerDescriptor.DepthBias = 0;
	rasterizerDescriptor.DepthBiasClamp = 0.0f;
	rasterizerDescriptor.SlopeScaledDepthBias = 0.0f;
	rasterizerDescriptor.DepthClipEnable = TRUE;
	rasterizerDescriptor.MultisampleEnable = FALSE;
	rasterizerDescriptor.AntialiasedLineEnable = FALSE;
	rasterizerDescriptor.ForcedSampleCount = 0u;
	rasterizerDescriptor.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	//We also need a blend descriptor:
	D3D12_RENDER_TARGET_BLEND_DESC blendDescriptor = {};
	blendDescriptor.BlendEnable = FALSE;
	blendDescriptor.LogicOpEnable = FALSE;
	blendDescriptor.SrcBlend = D3D12_BLEND_ONE;
	blendDescriptor.DestBlend = D3D12_BLEND_ZERO;
	blendDescriptor.BlendOp = D3D12_BLEND_OP_ADD;
	blendDescriptor.SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDescriptor.DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDescriptor.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDescriptor.LogicOp = D3D12_LOGIC_OP_NOOP;
	blendDescriptor.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//And a depth stencil descriptor:
	D3D12_DEPTH_STENCIL_DESC depthStencilDescriptor = {};
	depthStencilDescriptor.DepthEnable = TRUE;
	depthStencilDescriptor.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDescriptor.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthStencilDescriptor.StencilEnable = FALSE;
	depthStencilDescriptor.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	depthStencilDescriptor.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	depthStencilDescriptor.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDescriptor.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDescriptor.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDescriptor.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDescriptor.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDescriptor.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDescriptor.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	depthStencilDescriptor.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	//We also need a Stream Output Descriptor:
	D3D12_STREAM_OUTPUT_DESC streamOutputDescriptor = {};
	streamOutputDescriptor.pSODeclaration = nullptr;
	streamOutputDescriptor.NumEntries = 0u;
	streamOutputDescriptor.pBufferStrides = nullptr;
	streamOutputDescriptor.NumStrides = 0u;
	streamOutputDescriptor.RasterizedStream = 0u;

	//We need the shaders:
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob = nullptr;
	COMPILE_FROM_FILE(L"VertexShader.hlsl", "main", "vs_5_1", vertexShaderBlob);
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob = nullptr;
	COMPILE_FROM_FILE(L"PixelShader.hlsl", "main", "ps_5_1", pixelShaderBlob);

	//We now create the Graphics Pipe line state, the PSO:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescriptor = { 0 };
	std::vector<DXGI_FORMAT> rtvFormats = { DXGI_FORMAT_R8G8B8A8_UNORM };
	psoDescriptor.pRootSignature = m_pRootSignature.Get();
	psoDescriptor.VS.pShaderBytecode = vertexShaderBlob->GetBufferPointer();
	psoDescriptor.VS.BytecodeLength = vertexShaderBlob->GetBufferSize();
	psoDescriptor.PS.pShaderBytecode = pixelShaderBlob->GetBufferPointer();
	psoDescriptor.PS.BytecodeLength = pixelShaderBlob->GetBufferSize();

	psoDescriptor.SampleMask = UINT_MAX;
	psoDescriptor.RasterizerState = rasterizerDescriptor;
	psoDescriptor.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDescriptor.NumRenderTargets = static_cast<UINT>(rtvFormats.size());

	psoDescriptor.BlendState.AlphaToCoverageEnable = false;
	psoDescriptor.BlendState.IndependentBlendEnable = false;
	psoDescriptor.RTVFormats[0] = rtvFormats[0];
	psoDescriptor.BlendState.RenderTarget[0] = blendDescriptor;

	psoDescriptor.SampleDesc.Count = 1u;
	psoDescriptor.SampleDesc.Quality = 0u;
	psoDescriptor.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDescriptor.DepthStencilState = depthStencilDescriptor;
	psoDescriptor.StreamOutput = streamOutputDescriptor;
	psoDescriptor.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HR(DXCore::GetDevice()->CreateGraphicsPipelineState(&psoDescriptor, IID_PPV_ARGS(&m_pPSO)));
}

void Renderer::CreateViewportAndScissorRect() noexcept
{
	auto [width, height] = Window::Get().GetDimensions();

	m_ViewPort.TopLeftX = 0u;
	m_ViewPort.TopLeftY = 0u;
	m_ViewPort.MinDepth = 0.0f;
	m_ViewPort.MaxDepth = 1.0f;
	m_ViewPort.Width = static_cast<float>(width);
	m_ViewPort.Height = static_cast<float>(height);

	m_ScissorRect.left = 0u;
	m_ScissorRect.top = 0u;
	m_ScissorRect.right = static_cast<LONG>(m_ViewPort.Width);
	m_ScissorRect.bottom = static_cast<LONG>(m_ViewPort.Height);
}

void Renderer::CreateAllDescriptorHeaps() noexcept
{
	m_pShaderBindableDescriptorHeap = std::make_unique<DescriptorHeap>(600'000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	
	for (uint8_t i{ 0u }; i < NR_OF_FRAMES; ++i)
	{
		m_pShaderBindableNonVisibleDescriptorHeaps[i] = std::make_unique<DescriptorHeap>(600'000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, false);
	}
}

void Renderer::CreateConstantBuffersForTriangle() noexcept
{
	D3D12_HEAP_PROPERTIES bufferHeapProperties = {};
	bufferHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	bufferHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	bufferHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	bufferHeapProperties.CreationNodeMask = 0u;
	bufferHeapProperties.VisibleNodeMask = 0u;

	unsigned int byteWidth = sizeof(float) * 16;
	byteWidth = (byteWidth + 255) & ~255;

	D3D12_RESOURCE_DESC bufferDescriptor = {};
	bufferDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDescriptor.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	bufferDescriptor.Width = byteWidth;
	bufferDescriptor.Height = 1u;
	bufferDescriptor.DepthOrArraySize = 1u;
	bufferDescriptor.MipLevels = 1u;
	bufferDescriptor.Format = DXGI_FORMAT_UNKNOWN;
	bufferDescriptor.Flags = D3D12_RESOURCE_FLAG_NONE;
	bufferDescriptor.SampleDesc.Count = 1u;
	bufferDescriptor.SampleDesc.Quality = 0u;
	bufferDescriptor.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	for (uint8_t i{ 0u }; i < NR_OF_FRAMES; i++)
	{
		HR(DXCore::GetDevice()->CreateCommittedResource(&bufferHeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&bufferDescriptor, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_pTriangleConstantBuffers[i])));

		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pShaderBindableNonVisibleDescriptorHeaps[i]->GetCPUStartHandle();

		D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferDescriptor = {};
		constantBufferDescriptor.BufferLocation = m_pTriangleConstantBuffers[i]->GetGPUVirtualAddress();
		constantBufferDescriptor.SizeInBytes = (byteWidth + 255) & ~255;

		STDCALL(DXCore::GetDevice()->CreateConstantBufferView(&constantBufferDescriptor, handle));
	}
}

void Renderer::UpdateTriangleConstantBuffer(void* pData) noexcept
{
	auto index = Window::Get().GetCurrentBackbufferIndex();

	D3D12_RANGE range = { 0,0 };
	auto address = m_pTriangleConstantBuffers[index]->GetGPUVirtualAddress();
	size_t dataSize = sizeof(float) * 16;

	HR(m_pTriangleConstantBuffers[index]->Map(0u, &range, reinterpret_cast<void**>(&address)));
	std::memcpy(reinterpret_cast<void*>(address), reinterpret_cast<unsigned char*>(pData), dataSize);
	STDCALL(m_pTriangleConstantBuffers[index]->Unmap(0u, nullptr));

	auto srcHandle = m_pShaderBindableNonVisibleDescriptorHeaps[index]->GetCPUStartHandle();

	auto dstHandle = m_pShaderBindableDescriptorHeap->GetCPUStartHandle();
	dstHandle.ptr += DXCore::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 20'000 * index;

	STDCALL(DXCore::GetDevice()->CopyDescriptorsSimple(1u, dstHandle, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
}
