#pragma once

#include "directx/d3dx12.h"
#include <d3d12.h>
#include <wrl.h>

class PipelineState {

	public:


		PipelineState(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_INPUT_LAYOUT_DESC* inputLayout, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE primiviteTopology, D3D12_SHADER_BYTECODE* vertexShader, D3D12_SHADER_BYTECODE* pixelShader,
			DXGI_FORMAT depthStencilFormat, D3D12_RT_FORMAT_ARRAY* renderTargetFormats, CD3DX12_RASTERIZER_DESC* rasterizer = nullptr, CD3DX12_BLEND_DESC* blendDescription = nullptr,
			DXGI_SAMPLE_DESC* sampleDesc = nullptr);
		PipelineState(Microsoft::WRL::ComPtr<ID3D12Device2> device, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, D3D12_SHADER_BYTECODE* computeShader);
		~PipelineState();
		Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineStateObject();

	private:

		struct PSO_SUBSTREAMS_GRAPHICS {

			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT PSOInputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE PSORootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PSOPrimitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS PSOVertexShader;
			CD3DX12_PIPELINE_STATE_STREAM_PS PSOPixelShader;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT PSODepthStencilFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS PSORenderTargetFormats;
			CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER PSORasterizer;
			CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC PSOBlendDescription;
			CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC PSOSampleDesc;

		};

		struct PSO_SUBSTREAMS_COMPUTE {

			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE PSORootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CS PSOComputeShader;

		};

		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineStateObject;

};

