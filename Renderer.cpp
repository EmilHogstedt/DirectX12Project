#include "pch.h"
#include "Renderer.h"
#include "DXCore.h"
#include "Window.h"
#include "RenderCommand.h"
#include "Camera.h"
#include "MemoryManager.h"

void Renderer::Initialize() noexcept
{
	CreateDepthBuffer();
	CreateRootSignature();
	CreatePipelineStateObject();
	CreateViewportAndScissorRect();


	auto pCommandList = DXCore::GetCommandList();
	
	//for (uint32_t i{ 0u }; i < 20; ++i)
	//{
	//	m_pTriangle[i] = std::make_unique<Triangle>();
	//
	//	auto m = DirectX::XMLoadFloat4x4(&m_pTriangle[i]->GetWorldMatrix());
	//
	//	DirectX::XMFLOAT3 translation = { static_cast<float>(i), 0.0f, 0.0f };
	//	DirectX::XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };
	//	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	//	float angleX = 0.0f;
	//	float angleY = 0.0f;
	//	float angleZ = 0.0f;
	//	m = DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&scale)) * DirectX::XMMatrixRotationX(angleX) * DirectX::XMMatrixRotationY(angleY) * DirectX::XMMatrixRotationZ(angleZ) * DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&translation));
	//
	//	DirectX::XMStoreFloat4x4(&m_pTriangle[i]->GetWorldMatrix(), m);
	//
	//	m_pTriangle[i]->SetConstantBufferView(std::move(memoryManager.CreateConstantBuffer("Transforms", "ShaderBindables", "TransformsRange", sizeof(World))));
	//}

	HR(pCommandList->Close());
	ID3D12CommandList* commandLists[] = { pCommandList.Get() };
	STDCALL(DXCore::GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists));
	RenderCommand::Flush();
}

void Renderer::Begin(Camera* const pCamera) noexcept
{
	auto pCommandAllocator = DXCore::GetCommandAllocators()[m_CurrentBackBufferIndex];
	auto pCommandList = DXCore::GetCommandList();
	auto pBackBuffer = Window::Get().GetBackBuffers()[m_CurrentBackBufferIndex];

	auto& pBackBufferRTVDescHeap = Window::Get().GetBackBufferRTVHeap();
	auto backBufferDescriptorHandle = pBackBufferRTVDescHeap->GetCPUStartHandle();
	backBufferDescriptorHandle.ptr += m_CurrentBackBufferIndex * pBackBufferRTVDescHeap->GetDescriptorTypeSize();
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

	auto pDescriptorHeap = MemoryManager::Get().GetActiveSRVCBVUAVDescriptorHeap();
	STDCALL(pCommandList->SetDescriptorHeaps(1u, pDescriptorHeap->GetInterface().GetAddressOf()));

	static VP vpMatrixCBuffer;
	auto vpMatrix = DirectX::XMLoadFloat4x4(&(pCamera->GetVPMatrix()));
	vpMatrix = DirectX::XMMatrixTranspose(vpMatrix);
	DirectX::XMStoreFloat4x4(&vpMatrixCBuffer.VPMatrix, vpMatrix);
	STDCALL(pCommandList->SetGraphicsRoot32BitConstants(3u, 4*4, &vpMatrixCBuffer, 0u));
}

void Renderer::Submit(const std::unordered_map<std::wstring, std::vector<std::shared_ptr<VertexObject>>>& vertexObjects, float deltaTime) noexcept
{
	static float speed = 1.0f;

	auto pCommandList = DXCore::GetCommandList();
	auto index = Window::Get().GetCurrentBackbufferIndex();

	for (auto& modelInstances : vertexObjects)
	{
		for (auto& object : modelInstances.second)
		{
			object->Update(deltaTime);

			auto& cbv = object->GetTransformConstantBufferView();
			auto gpuHandle = cbv.GpuHandles[index];

			STDCALL(pCommandList->SetGraphicsRootDescriptorTable(0, gpuHandle));
			STDCALL(pCommandList->SetGraphicsRootShaderResourceView(1u, object->GetModel()->GetVertexBufferGPUAddress()));
			STDCALL(pCommandList->SetGraphicsRootShaderResourceView(2u, object->GetModel()->GetIndexBufferGPUAddress()));
			STDCALL(pCommandList->DrawInstanced(object->GetModel()->GetIndexCount(), 1u, 0u, 0u));
		}
	}
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
