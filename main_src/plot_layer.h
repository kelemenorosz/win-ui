#pragma once

#include "directx/d3dx12.h"
#include <d3d12.h>
#include "shader.h"
#include "root_signature.h"
#include "dx12_window.h"
#include "pipeline_state.h"
#include "depth_stencil.h"
#include "dx12_layer.h"
#include <memory>

class PlotLayer : public DX12Layer {

	public:

		PlotLayer(Microsoft::WRL::ComPtr<ID3D12Device2> dev, ThWnd_Dim dim);
		virtual ~PlotLayer();

		PlotLayer(const PlotLayer& other) = delete;
		PlotLayer& operator=(const PlotLayer& other) = delete;

		virtual LRESULT OnRender(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer, D3D12_CPU_DESCRIPTOR_HANDLE backBufferDescriptor) override;
		virtual LRESULT OnUpdate(double ts) override;
		virtual LRESULT OnEvent(const DX12Layer_Event& e) override;

		struct Position {
			float x;
			float y;
			float z;
		};

		struct InitDesc {
	
			void* bufX;
			void* bufY;
			int bufLen;
			float scaleX;
			float scaleY;
			float centreX;
			float centreY;

		};

	private:

		void OnInit(PlotLayer::InitDesc* desc);
		void OnMouseWheel(DX12Layer_Event_MouseWheel* desc);
		void OnMouseMove(DX12Layer_Event_MouseMove* desc);

		std::unique_ptr<Shader> vs;
		std::unique_ptr<Shader> pixel_shader;
		std::unique_ptr<RootSignature> rs;
		std::unique_ptr<PipelineState> ps;
		std::unique_ptr<DepthStencil> ds;

		std::unique_ptr<Resource> vb_x;
		std::unique_ptr<Resource> vb_y;

		std::unique_ptr<Resource> msaa_rt;
		std::unique_ptr<DepthStencil> msaa_ds;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> msaa_rtv_dh;
		D3D12_CPU_DESCRIPTOR_HANDLE msaa_rt_handle;
		D3D12_CPU_DESCRIPTOR_HANDLE msaa_ds_handle;

		std::unique_ptr<Shader> mono2d_vs;
		std::unique_ptr<Shader> mono2d_ps;
		std::unique_ptr<RootSignature> mono2d_rs;
		std::unique_ptr<PipelineState> mono2d_pipeline;

		std::unique_ptr<CommandQueue> copy_queue;

		std::unique_ptr<Resource> scrollbar_bg;
		std::unique_ptr<Resource> scrollbar_fg;

		D3D12_VIEWPORT viewport;
		D3D12_RECT scissorRectangle;

		PlotLayer::InitDesc initDesc;
		float mwheel_mult;
		float scrollbar_fg_start; 
		float scrollbar_fg_len;
		float mag_start;
		int cursor_pos_x;
		int cursor_pos_y;
		float cursor_pos_norm_x;
		float cursor_pos_norm_y;
		bool cursor_down;
		bool cursor_valid;
		bool cursor_first;

		int scrollbar_fg_fence;

};