#pragma once

#include "directx/d3dx12.h"
#include <d3d12.h>

#include "threaded_window.h"
#include "commandqueue.h"
#include "dx12_layer.h"
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <wrl.h>

constexpr auto SWAP_CHAIN_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr auto SWAP_CHAIN_BUFFER_COUNT = 3;

enum class DX12Wnd_CustomEvent_Other_Type : uint8_t {

	DX12WND_CUSTOMEVENT_OTHER_TYPE_DX12LAYER = 0,

};

struct DX12Wnd_CustomEvent_Other {

	DX12Wnd_CustomEvent_Other() = default;
	~DX12Wnd_CustomEvent_Other() {}

	DX12Wnd_CustomEvent_Other(const DX12Wnd_CustomEvent_Other& other) = delete;
	DX12Wnd_CustomEvent_Other& operator=(const DX12Wnd_CustomEvent_Other& other) = delete;

	DX12Wnd_CustomEvent_Other_Type type;
	void* data;

};

class DX12Window : public ThreadedWindow {

	public:

		DX12Window() = delete;
		DX12Window(LPCWSTR windowName, int x, int y, int nWidth, int nHeight);
		virtual ~DX12Window();

		template<class _Lyr>
		static void RegisterLayer(TWHANDLE handle);

		static LRESULT LayerMessage(TWHANDLE handle, DX12Layer_Event_Type t, void* data);
		static Microsoft::WRL::ComPtr<ID3D12Debug> D12Debug;
		static Microsoft::WRL::ComPtr<IDXGIDebug> DXGIDebug;
		static void InitDebug();
		static void ShutdownDebug();

	private:

		virtual LRESULT OnPaint(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		virtual LRESULT OnMouseWheel(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		virtual LRESULT OnMouseMove(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		virtual LRESULT OnCustomEventOther(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void OnRender();
		void OnUpdate();

		Microsoft::WRL::ComPtr<ID3D12Device2> D12Device;
		std::shared_ptr<CommandQueue> directCQ;
		std::shared_ptr<CommandQueue> copyCQ;
		Microsoft::WRL::ComPtr<IDXGISwapChain3> DXGISwapChain;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> D12RTVDescHeap;
		Microsoft::WRL::ComPtr<ID3D12Resource> swapChainBuffers[SWAP_CHAIN_BUFFER_COUNT]; 
		uint64_t fenceValues[SWAP_CHAIN_BUFFER_COUNT] = { 0 };

		std::shared_ptr<DX12Layer> layer;

};

template<class _Lyr>
void DX12Window::RegisterLayer(TWHANDLE handle) {

	if (auto err = GetWndObject(handle); !err.has_value()) {
		return;
	}
	else {
		std::shared_ptr<ThreadedWindow> thWnd = *err;
		DX12Window* dx12Wnd = dynamic_cast<DX12Window*>(thWnd.get());
		dx12Wnd->layer = std::make_shared<_Lyr>(dx12Wnd->D12Device, dx12Wnd->GetDim());
	}

	// if (this->IsClosed()) {
	// 	printf("[WARNING] Attempting to DX12Window::RegisterLayer() to closed window.\n");
	// 	return;
	// }

	return;

}
