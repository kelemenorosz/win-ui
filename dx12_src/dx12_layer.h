#pragma once

#include "directx/d3dx12.h"
#include <d3d12.h>

#include <cstdint>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <wrl.h>
#include "threaded_window.h"

enum class DX12Layer_Event_Type : uint8_t {

	DX12LAYER_EVENT_TYPE_INIT = 0,
	DX12LAYER_EVENT_TYPE_MOUSEWHEEL,
	DX12LAYER_EVENT_TYPE_MOUSEMOVE

};

struct DX12Layer_Event_MouseWheel {
	float delta;
};

struct DX12Layer_Event_MouseMove {
	int x;
	int y;
	int vk_down;
};

struct DX12Layer_Event {

	DX12Layer_Event() = default;
	~DX12Layer_Event() {}

	DX12Layer_Event(const DX12Layer_Event& other) = delete;
	DX12Layer_Event& operator=(const DX12Layer_Event& other) = delete;

	DX12Layer_Event_Type type;
	void* data;

};

class DX12Layer {

	protected:

		enum class State : uint8_t {
			DEFAULT = 0,
			READY
		};

	public:

		DX12Layer(Microsoft::WRL::ComPtr<ID3D12Device2> dev, ThWnd_Dim dim) : dev(dev), dim(dim), state(DX12Layer::State::DEFAULT) {}
		virtual ~DX12Layer() {}

		virtual LRESULT OnRender(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer, D3D12_CPU_DESCRIPTOR_HANDLE backBufferDescriptor) = 0;
		virtual LRESULT OnUpdate(double ts) = 0;
		virtual LRESULT OnEvent(const DX12Layer_Event& e) = 0;

	protected:

		Microsoft::WRL::ComPtr<ID3D12Device2> dev;
		ThWnd_Dim dim;
		DX12Layer::State state; 

	private:

};

