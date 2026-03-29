
#include "pipeline_state.h"
#include "directx/d3dx12.h"
#include <d3d12.h>
#include <wrl.h>
#include <iostream>

namespace mWRL = Microsoft::WRL;

PipelineState::PipelineState(mWRL::ComPtr<ID3D12Device2> device, D3D12_INPUT_LAYOUT_DESC* inputLayout, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE primiviteTopology, D3D12_SHADER_BYTECODE* vertexShader, D3D12_SHADER_BYTECODE* pixelShader,
	DXGI_FORMAT depthStencilFormat, D3D12_RT_FORMAT_ARRAY* renderTargetFormats, CD3DX12_RASTERIZER_DESC* rasterizer, CD3DX12_BLEND_DESC* blendDescription,
	DXGI_SAMPLE_DESC* sampleDesc) {

	PSO_SUBSTREAMS_GRAPHICS psoSubstreams = {};
	if (inputLayout != nullptr) psoSubstreams.PSOInputLayout = *inputLayout;
	psoSubstreams.PSORootSignature = rootSignature.Get();
	psoSubstreams.PSOPrimitiveTopology = primiviteTopology;
	psoSubstreams.PSOVertexShader = *vertexShader;
	if (pixelShader != nullptr) psoSubstreams.PSOPixelShader = *pixelShader;
	psoSubstreams.PSODepthStencilFormat = depthStencilFormat;
	psoSubstreams.PSORenderTargetFormats = *renderTargetFormats;
	if (rasterizer != nullptr) psoSubstreams.PSORasterizer = *rasterizer;
	if (blendDescription != nullptr) psoSubstreams.PSOBlendDescription = *blendDescription;
	if (sampleDesc != nullptr) psoSubstreams.PSOSampleDesc = *sampleDesc;

	// uint8_t* buf = new uint8_t[2048];
	// uint8_t* buf_start = buf;

	// *reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT*>(buf) = CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT();
	// *reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT*>(buf) = *inputLayout;
	// buf += sizeof(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT);

	// *reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE*>(buf) = CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE();
	// *reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE*>(buf) = rootSignature.Get();
	// buf += sizeof(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE);

	// *reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY*>(buf) = CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY();
	// *reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY*>(buf) = primiviteTopology;
	// buf += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY);

	// printf("sizeof CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY: %zd.\n", sizeof(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY));
	// printf("alignof CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY: %zd.\n", alignof(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY));
	// printf("alignof CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY: %zd.\n", alignof(void*));

	// *reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_VS*>(buf) = CD3DX12_PIPELINE_STATE_STREAM_VS();
	// *reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_VS*>(buf) = *vertexShader;
	// buf += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS);

	// if (pixelShader != nullptr) {
	// 	*reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_PS*>(buf) = CD3DX12_PIPELINE_STATE_STREAM_PS();
	// 	*reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_PS*>(buf) = *pixelShader;
	// 	buf += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS);
	// }

	// *reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT*>(buf) = CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT();
	// *reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT*>(buf) = depthStencilFormat;
	// buf += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT);
	
	// *reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS*>(buf) = CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS();
	// *reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS*>(buf) = *renderTargetFormats;
	// buf += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS);

	// if (rasterizer != nullptr) {
	// 	*reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER*>(buf) = *rasterizer;
	// 	buf += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER);
	// }

	// if (blendDescription != nullptr) {
	// 	*reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC*>(buf) = *blendDescription;
	// 	buf += sizeof(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC);
	// }	

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDescription = {};
	// pipelineStateStreamDescription.pPipelineStateSubobjectStream = buf_start;
	// pipelineStateStreamDescription.SizeInBytes = buf - buf_start;
	pipelineStateStreamDescription.pPipelineStateSubobjectStream = &psoSubstreams;
	pipelineStateStreamDescription.SizeInBytes = sizeof(PSO_SUBSTREAMS_GRAPHICS);
	HRESULT hr = device->CreatePipelineState(&pipelineStateStreamDescription, IID_PPV_ARGS(&m_pipelineStateObject));
	if (hr != S_OK) {
	
		std::cout << "[ERROR] Pipeline state object creation failed with error code: " << hr << std::endl;

	}

	// delete[] buf_start;

}

PipelineState::PipelineState(Microsoft::WRL::ComPtr<ID3D12Device2> device, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE* computeShader) {

	PSO_SUBSTREAMS_COMPUTE psoSubstreams = {};
	psoSubstreams.PSORootSignature = rootSignature.Get();
	psoSubstreams.PSOComputeShader = *computeShader;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDescription = {};
	pipelineStateStreamDescription.pPipelineStateSubobjectStream = &psoSubstreams;
	pipelineStateStreamDescription.SizeInBytes = sizeof(PSO_SUBSTREAMS_COMPUTE);
	HRESULT hr = device->CreatePipelineState(&pipelineStateStreamDescription, IID_PPV_ARGS(&m_pipelineStateObject));
	if (hr != S_OK) {
	
		std::cout << "[ERROR] Pipeline state object creation failed with error code: " << hr << std::endl;

	}

}

PipelineState::~PipelineState() {

}

mWRL::ComPtr<ID3D12PipelineState> PipelineState::GetPipelineStateObject() {

	return m_pipelineStateObject;

}