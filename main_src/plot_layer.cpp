
#include "directx/d3dx12.h"
#include <d3d12.h>
#include "plot_layer.h"
#include "shader.h"
#include "root_signature.h"
#include "dx12_window.h"
#include "pipeline_state.h"
#include "depth_stencil.h"
#include "commandqueue.h"
#include <cstdio>

PlotLayer::PlotLayer(Microsoft::WRL::ComPtr<ID3D12Device2> dev, ThWnd_Dim dim) : DX12Layer(dev, dim), vs(nullptr), rs(nullptr), ps(nullptr), ds(nullptr), mwheel_mult(1.0f),
	scrollbar_fg_start(-1.0f), scrollbar_fg_len(2.0f), scrollbar_fg_fence(0), cursor_pos_x(0), cursor_pos_y(0), mag_start(-1.0f), cursor_pos_norm_x(0.0f), cursor_pos_norm_y(0.0f),
	cursor_down(false), cursor_valid(false), cursor_first(true) {

	scrollbar_fg_len = (1.0f / mwheel_mult) * 2.0f;

	// -- Plotting pipeline

	vs = std::make_unique<Shader>("./main_src/plot.hlsl", Shader_Type::SHADER_TYPE_VS);
	pixel_shader = std::make_unique<Shader>("./main_src/plot.hlsl", Shader_Type::SHADER_TYPE_PS);
	
	{
		D3D12_ROOT_PARAMETER1 rootParameter_array[2] = {};
		
		// -- ScalingDesc: scaleX, scaleY, centreX, centreY 
		rootParameter_array[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParameter_array[0].Constants.ShaderRegister = 0;
		rootParameter_array[0].Constants.RegisterSpace = 0;
		rootParameter_array[0].Constants.Num32BitValues = 4;
		rootParameter_array[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		
		// -- MWheel mult
		rootParameter_array[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParameter_array[1].Constants.ShaderRegister = 1;
		rootParameter_array[1].Constants.RegisterSpace = 0;
		rootParameter_array[1].Constants.Num32BitValues = 2;
		rootParameter_array[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rs = std::make_unique<RootSignature>(this->dev, &rootParameter_array[0], static_cast<UINT>(_countof(rootParameter_array)), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	}
	
	{
		D3D12_INPUT_LAYOUT_DESC inputLayoutDescription = {};
		D3D12_INPUT_ELEMENT_DESC inputLayoutElementsDescription[] = {

				{"POSITION", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{"POSITION", 1, DXGI_FORMAT_R32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		
		};
		inputLayoutDescription.pInputElementDescs = inputLayoutElementsDescription;
		inputLayoutDescription.NumElements = _countof(inputLayoutElementsDescription);

		D3D12_RT_FORMAT_ARRAY rt_format = {
			{SWAP_CHAIN_BUFFER_FORMAT},
			1
		};

		CD3DX12_RASTERIZER_DESC rasterizer_desc(
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_BACK,
			FALSE,
			0,
			0.0f,
			0.0f,
			TRUE,
			TRUE,
			TRUE,
			0,
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		);

		DXGI_SAMPLE_DESC sample_desc = { 8, 0 };

		ps = std::make_unique<PipelineState>(this->dev, &inputLayoutDescription, rs->GetRootSignature(), D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE, vs->GetBytecode(), pixel_shader->GetBytecode(),
			DXGI_FORMAT_D24_UNORM_S8_UINT, &rt_format, &rasterizer_desc, nullptr, &sample_desc);
	}

	ds = std::make_unique<DepthStencil>(dev, dim.nWidth, dim.nHeight, DXGI_FORMAT_D24_UNORM_S8_UINT);

	// -- Mono2d pipeline
	// -- Draws in 2d space with a single color

	{

		mono2d_vs = std::make_unique<Shader>("./main_src/mono2d.hlsl", Shader_Type::SHADER_TYPE_VS);
		mono2d_ps = std::make_unique<Shader>("./main_src/mono2d.hlsl", Shader_Type::SHADER_TYPE_PS);

		D3D12_ROOT_PARAMETER1 rootParameter_array[1] = {};
			
		// -- Color: red, green, blue + alpha blend factor + z
		rootParameter_array[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParameter_array[0].Constants.ShaderRegister = 0;
		rootParameter_array[0].Constants.RegisterSpace = 0;
		rootParameter_array[0].Constants.Num32BitValues = 5;
		rootParameter_array[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	
		mono2d_rs = std::make_unique<RootSignature>(this->dev, &rootParameter_array[0], static_cast<UINT>(_countof(rootParameter_array)), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	
		D3D12_INPUT_LAYOUT_DESC inputLayoutDescription = {};
		D3D12_INPUT_ELEMENT_DESC inputLayoutElementsDescription[] = {

				{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		
		};
		inputLayoutDescription.pInputElementDescs = inputLayoutElementsDescription;
		inputLayoutDescription.NumElements = _countof(inputLayoutElementsDescription);

		D3D12_RT_FORMAT_ARRAY rt_format = {
			{SWAP_CHAIN_BUFFER_FORMAT},
			1
		};

		CD3DX12_RASTERIZER_DESC rasterizer_desc(
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_NONE,
			FALSE,
			0,
			0.0f,
			0.0f,
			TRUE,
			TRUE,
			TRUE,
			0,
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		);

		D3D12_BLEND_DESC blend_desc = {};
		blend_desc.AlphaToCoverageEnable = FALSE;
		blend_desc.IndependentBlendEnable = FALSE;
		blend_desc.RenderTarget[0].BlendEnable = TRUE;
		blend_desc.RenderTarget[0].LogicOpEnable = FALSE;
		blend_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blend_desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blend_desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blend_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
		blend_desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blend_desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blend_desc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_AND;
		blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		CD3DX12_BLEND_DESC c_blend_desc(blend_desc);

		DXGI_SAMPLE_DESC sample_desc = { 8, 0 };

		mono2d_pipeline = std::make_unique<PipelineState>(this->dev, &inputLayoutDescription, mono2d_rs->GetRootSignature(), D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			mono2d_vs->GetBytecode(), mono2d_ps->GetBytecode(),	DXGI_FORMAT_D24_UNORM_S8_UINT, &rt_format, &rasterizer_desc, &c_blend_desc, &sample_desc);

	}

	// -- Viewport; Scissor rectangle

	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(dim.nWidth);
	viewport.Height = static_cast<float>(dim.nHeight);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	scissorRectangle.left = 0;
	scissorRectangle.top = 0;
	scissorRectangle.right = LONG_MAX;
	scissorRectangle.bottom = LONG_MAX;

	// -- Multisample render target; depth buffer
	// -- Depth buffer might be unnecessary...

	D3D12_CLEAR_VALUE clearVal = {};
	clearVal.Format = SWAP_CHAIN_BUFFER_FORMAT;
	clearVal.Color[0] = 0.0f;
	clearVal.Color[1] = 0.0f;
	clearVal.Color[2] = 0.0f;
	clearVal.Color[3] = 1.0f;
	msaa_rt = std::make_unique<Resource>(dev, RESOURCE_NO_READBACK | RESOURCE_NO_UPLOAD, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		dim.nWidth, dim.nHeight, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, 1, 1, SWAP_CHAIN_BUFFER_FORMAT, 8, 0, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &clearVal);
	Resource::CreateDescriptorHeap(dev, &msaa_rtv_dh, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	msaa_rt_handle = Resource::CreateRenderTargetView(dev, msaa_rtv_dh, 0, msaa_rt->GetDefaultBuffer());

	msaa_ds = std::make_unique<DepthStencil>(dev, dim.nWidth, dim.nHeight, DXGI_FORMAT_D24_UNORM_S8_UINT, 8);
	msaa_ds_handle = msaa_ds->GetHandle();

	// -- Scrollbar background & foreground

	float scrollbar_bg_array[] = {
		-1.0f, -0.95f,
		1.0f, -0.95f,
		-1.0f, -1.0f,

		-1.0f, -1.0f,
		1.0f, -0.95f,
		1.0f, -1.0f,
	};

	float scrollbar_fg_array[] = {
		scrollbar_fg_start, -0.95f,
		scrollbar_fg_start + scrollbar_fg_len, -0.95f,
		scrollbar_fg_start, -1.0f,

		scrollbar_fg_start, -1.0f,
		scrollbar_fg_start + scrollbar_fg_len, -0.95f,
		scrollbar_fg_start + scrollbar_fg_len, -1.0f,
	};

	scrollbar_bg = std::make_unique<Resource>(dev, RESOURCE_NO_READBACK, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_DIMENSION_BUFFER, sizeof(scrollbar_bg_array), 1,
	D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE, nullptr);
	scrollbar_bg->SetVertexBufferView(sizeof(scrollbar_bg_array), sizeof(float) * 2);

	scrollbar_fg = std::make_unique<Resource>(dev, RESOURCE_NO_READBACK, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_DIMENSION_BUFFER, sizeof(scrollbar_fg_array), 1,
	D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE, nullptr);
	scrollbar_fg->SetVertexBufferView(sizeof(scrollbar_fg_array), sizeof(float) * 2);

	BYTE* upload_buf = nullptr;
	D3D12_RANGE writeRange = {0, sizeof(scrollbar_bg_array) };
	scrollbar_bg->MapUploadBufferPtr(0, reinterpret_cast<void**>(&upload_buf));
	memcpy(upload_buf, &scrollbar_bg_array[0], sizeof(scrollbar_bg_array));
	scrollbar_bg->UnmapUploadBufferPtr(0, &writeRange);
	scrollbar_fg->MapUploadBufferPtr(0, reinterpret_cast<void**>(&upload_buf));
	memcpy(upload_buf, &scrollbar_fg_array[0], sizeof(scrollbar_fg_array));
	scrollbar_fg->UnmapUploadBufferPtr(0, &writeRange);

	CommandQueue cq(dev, D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL);
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cl = cq.GetCommandList();
	scrollbar_bg->CopyUploadToDefault(cl);
	scrollbar_fg->CopyUploadToDefault(cl);
	cq.WaitForFenceValue(cq.ExecuteCommandList(cl));

	copy_queue = std::make_unique<CommandQueue>(dev, D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL);

	return;

}

PlotLayer::~PlotLayer() {

	printf("[INFO] PlotLayer::~PlotLayer().\n");
	return;

}

LRESULT PlotLayer::OnRender(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer, D3D12_CPU_DESCRIPTOR_HANDLE backBufferDescriptor) {

	if (state == DX12Layer::State::DEFAULT) return S_OK;

	Resource::TransitionBackBuffer(commandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	FLOAT backBufferClearValue[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	D3D12_RECT clearRectangle = { 0, 0, this->dim.nWidth, this->dim.nHeight };
	commandList->ClearRenderTargetView(this->msaa_rt_handle, backBufferClearValue, 1, &clearRectangle);
	commandList->ClearDepthStencilView(this->msaa_ds_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);

	// -- Set render target, viewport, scissor rect
	{
		commandList->OMSetRenderTargets(1, &this->msaa_rt_handle, FALSE, &this->msaa_ds_handle);
		commandList->RSSetViewports(1, &this->viewport);
		commandList->RSSetScissorRects(1, &this->scissorRectangle);
	}

	// -- plot pipeline
	{
		commandList->SetPipelineState(this->ps->GetPipelineStateObject().Get());
		commandList->SetGraphicsRootSignature(this->rs->GetRootSignature().Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
	}

	// -- Plot
	{
		D3D12_VERTEX_BUFFER_VIEW vb_views[] = {
			vb_x->GetVertexBufferView(),
			vb_y->GetVertexBufferView(),
		};
		commandList->IASetVertexBuffers(0, _countof(vb_views), &vb_views[0]);
		commandList->SetGraphicsRoot32BitConstants(0, 4, &initDesc.scaleX, 0);
		float mwheel_desc[] = { mwheel_mult, mag_start };
		commandList->SetGraphicsRoot32BitConstants(1, 2, &mwheel_desc[0], 0);
		commandList->DrawInstanced(this->initDesc.bufLen, 1, 0, 0);
	}
	
	// -- mono2d pipeline
	{
		commandList->SetPipelineState(this->mono2d_pipeline->GetPipelineStateObject().Get());
		commandList->SetGraphicsRootSignature(this->mono2d_rs->GetRootSignature().Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	// -- Scrollbar bg
	{
		D3D12_VERTEX_BUFFER_VIEW vb_views[] = {
			scrollbar_bg->GetVertexBufferView(),
		};
		commandList->IASetVertexBuffers(0, _countof(vb_views), &vb_views[0]);
		float color[] = { 0.863f, 0.863f, 0.863f, 0.3f, 0.5f };
		commandList->SetGraphicsRoot32BitConstants(0, 5, &color[0], 0);
		commandList->DrawInstanced(6, 1, 0, 0);
	}

	// -- Scrollbar fg
	{
		if (scrollbar_fg_fence != 0) copy_queue->WaitForFenceValue(scrollbar_fg_fence);
		D3D12_VERTEX_BUFFER_VIEW vb_views[] = {
			scrollbar_fg->GetVertexBufferView(),
		};
		commandList->IASetVertexBuffers(0, _countof(vb_views), &vb_views[0]);
		float color[] = { 0.412f, 0.412f, 0.412f, 0.7f, 0.0f };
		commandList->SetGraphicsRoot32BitConstants(0, 5, &color[0], 0);
		commandList->DrawInstanced(6, 1, 0, 0);
	}

	Resource::TransitionBackBuffer(commandList, msaa_rt->GetDefaultBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
	Resource::TransitionBackBuffer(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_DEST);
	commandList->ResolveSubresource(backBuffer.Get(), 0, msaa_rt->GetDefaultBuffer().Get(), 0, SWAP_CHAIN_BUFFER_FORMAT);
	Resource::TransitionBackBuffer(commandList, msaa_rt->GetDefaultBuffer(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	Resource::TransitionBackBuffer(commandList, backBuffer, D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);

	Resource::TransitionBackBuffer(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	return S_OK;

}

LRESULT PlotLayer::OnUpdate(double ts) {

	float scrollbar_fg_array[] = {
		scrollbar_fg_start, -0.95f,
		scrollbar_fg_start + scrollbar_fg_len, -0.95f,
		scrollbar_fg_start, -1.0f,

		scrollbar_fg_start, -1.0f,
		scrollbar_fg_start + scrollbar_fg_len, -0.95f,
		scrollbar_fg_start + scrollbar_fg_len, -1.0f,
	};

	BYTE* upload_buf = nullptr;
	D3D12_RANGE writeRange = {0, sizeof(scrollbar_fg_array) };
	scrollbar_fg->MapUploadBufferPtr(0, reinterpret_cast<void**>(&upload_buf));
	memcpy(upload_buf, &scrollbar_fg_array[0], sizeof(scrollbar_fg_array));
	scrollbar_fg->UnmapUploadBufferPtr(0, &writeRange);

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cl = copy_queue->GetCommandList();
	scrollbar_fg->CopyUploadToDefault(cl);
	scrollbar_fg_fence = copy_queue->ExecuteCommandList(cl);

	return S_OK;

}

LRESULT PlotLayer::OnEvent(const DX12Layer_Event& e) {

	PlotLayer::InitDesc* initDesc = nullptr;
	switch(e.type) {
		case DX12Layer_Event_Type::DX12LAYER_EVENT_TYPE_INIT:
			this->OnInit(reinterpret_cast<PlotLayer::InitDesc*>(e.data));
			break;
		case DX12Layer_Event_Type::DX12LAYER_EVENT_TYPE_MOUSEWHEEL:
			this->OnMouseWheel(reinterpret_cast<DX12Layer_Event_MouseWheel*>(e.data));
			break;
		case DX12Layer_Event_Type::DX12LAYER_EVENT_TYPE_MOUSEMOVE:
			this->OnMouseMove(reinterpret_cast<DX12Layer_Event_MouseMove*>(e.data));
			break;
		default:
			break;
	}

	return S_OK;

}

void PlotLayer::OnInit(PlotLayer::InitDesc* desc) {

	vb_x = std::make_unique<Resource>(dev, RESOURCE_NO_READBACK, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_DIMENSION_BUFFER, sizeof(float) * desc->bufLen, 1,
	D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE, nullptr);
	vb_x->SetVertexBufferView(sizeof(float) * desc->bufLen, sizeof(float));
	vb_y = std::make_unique<Resource>(dev, RESOURCE_NO_READBACK, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_DIMENSION_BUFFER, sizeof(float) * desc->bufLen, 1,
		D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE, nullptr);
	vb_y->SetVertexBufferView(sizeof(float) * desc->bufLen, sizeof(float));

	BYTE* upload_buf = nullptr;
	D3D12_RANGE writeRange = {0, sizeof(float) * desc->bufLen };
	vb_x->MapUploadBufferPtr(0, reinterpret_cast<void**>(&upload_buf));
	memcpy(upload_buf, desc->bufX, sizeof(float) * desc->bufLen);
	vb_x->UnmapUploadBufferPtr(0, &writeRange);
	vb_y->MapUploadBufferPtr(0, reinterpret_cast<void**>(&upload_buf));
	memcpy(upload_buf, desc->bufY, sizeof(float) * desc->bufLen);
	vb_y->UnmapUploadBufferPtr(0, &writeRange);

	CommandQueue cq(dev, D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL);
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cl = cq.GetCommandList();
	vb_x->CopyUploadToDefault(cl);
	vb_y->CopyUploadToDefault(cl);
	cq.WaitForFenceValue(cq.ExecuteCommandList(cl));

	this->initDesc = *desc;
	this->state = DX12Layer::State::READY;	

	return;

}

void PlotLayer::OnMouseWheel(DX12Layer_Event_MouseWheel* desc) {

	BOOL res = FALSE;
	POINT cursor_pos = {};
	res = ::GetCursorPos(&cursor_pos);
	printf("[INFO] cursor pos: %d, %d.\n", cursor_pos_x, cursor_pos_y);
	printf("[INFO] cursor pos normalized: %f, %f.\n", cursor_pos_norm_x, cursor_pos_norm_y);

	printf("[INFO] DX12Layer_Event_Type::DX12LAYER_EVENT_TYPE_MOUSEWHEEL delta: %f.\n", desc->delta);	
	if (desc->delta > 0) {
		mwheel_mult *= 2.0f;
		scrollbar_fg_len = (1.0f / mwheel_mult) * 2.0f;
		// -- 
		mag_start -= cursor_pos_norm_x;
		mag_start *= 2.0f;
		mag_start += cursor_pos_norm_x;
		float shift = -mwheel_mult - mag_start;
		scrollbar_fg_start = (-1.0f - mag_start) / (mwheel_mult * 2.0f);
		scrollbar_fg_start *= 2;
		scrollbar_fg_start -= 1;
		printf("mag_start: %f.\n", mag_start);
		printf("mwheel_mult: %f.\n", mwheel_mult);
		printf("scrollbar_fg_start: %f.\n", scrollbar_fg_start);
		printf("scrollbar_fg_len: %f.\n", scrollbar_fg_len);
	}
	else {
		if (mwheel_mult > 1.0f) {
			mwheel_mult /= 2.0f;
			scrollbar_fg_len = (1.0f / mwheel_mult) * 2.0f;
			mag_start -= cursor_pos_norm_x;
			mag_start /= 2.0f;
			mag_start += cursor_pos_norm_x;
			if (mag_start > -1.0f) {
				mag_start = -1.0f;
			}
			else if ((mag_start + 2.0f * mwheel_mult) < 1.0f) {
				mag_start = 1.0f - 2.0f * mwheel_mult;
			}
			// if (mwheel_mult == 1.0f) {
			// 	mag_start = -1.0f;
			// }
			scrollbar_fg_start = (-1.0f - mag_start) / (mwheel_mult * 2.0f);
			scrollbar_fg_start *= 2;
			scrollbar_fg_start -= 1;
			printf("mag_start: %f.\n", mag_start);
			printf("mwheel_mult: %f.\n", mwheel_mult);
			printf("scrollbar_fg_start: %f.\n", scrollbar_fg_start);
			printf("scrollbar_fg_len: %f.\n", scrollbar_fg_len);
		}
		printf("mwheel_mult: %f.\n", mwheel_mult);
	}

}

void PlotLayer::OnMouseMove(DX12Layer_Event_MouseMove* desc) {

	int prev_cursor_pos_x = cursor_pos_x;
	int prev_cursor_pos_y = cursor_pos_y;

	cursor_pos_x = desc->x;
	cursor_pos_y = desc->y;
	
	cursor_pos_norm_x = (static_cast<float>(cursor_pos_x) / (static_cast<float>(dim.nWidth) / 2.0f)) - 1.0f;
	cursor_pos_norm_y = (static_cast<float>(cursor_pos_y) / (static_cast<float>(dim.nHeight) / 2.0f)) - 1.0f;
	cursor_pos_norm_y = -cursor_pos_norm_y;

	if ((desc->vk_down & MK_LBUTTON) > 0) {
		if (cursor_down == true) {
			cursor_first = false;
		}
		if (cursor_first == true && cursor_pos_norm_y <= -0.95f) {
			cursor_valid = true;
		}
		cursor_down = true;
	}
	else {
		cursor_down = false;
		cursor_first = true;
		cursor_valid = false;
	}

	if (cursor_first == false && cursor_valid == true) {
		int dist_x = prev_cursor_pos_x - cursor_pos_x;
		printf("dist_x: %d.\n", dist_x);
		float dist_norm_x = (static_cast<float>(dist_x) / (static_cast<float>(dim.nWidth) / 2.0f));
		printf("dist_norm_x: %f.\n", dist_norm_x);
		// -- Positive diff -> left
		if ((scrollbar_fg_start - dist_norm_x) > -1.0f && (scrollbar_fg_start - dist_norm_x + scrollbar_fg_len) < 1.0f) {
			scrollbar_fg_start = scrollbar_fg_start - dist_norm_x;
			mag_start = scrollbar_fg_start + 1.0f;
			mag_start /= 2.0f;
			mag_start *= mwheel_mult * 2.0f;
			mag_start = -1.0f - mag_start;
		}
	} 
		
	return;

}