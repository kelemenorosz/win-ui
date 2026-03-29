
#include "directx/d3dx12.h"
#include <d3d12.h>

#include "dx12_window.h"
#include "root_signature.h"

Microsoft::WRL::ComPtr<ID3D12Debug> DX12Window::D12Debug = nullptr;
Microsoft::WRL::ComPtr<IDXGIDebug> DX12Window::DXGIDebug = nullptr;
		
DX12Window::DX12Window(LPCWSTR windowName, int x, int y, int nWidth, int nHeight) : ThreadedWindow(windowName, x, y, nWidth, nHeight) {

	// -- Adapter

	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
	Microsoft::WRL::ComPtr<IDXGIAdapter> t_dxgiAdapter;
	DXGI_ADAPTER_DESC dxgiAdapterDescription = { 0 };
	SIZE_T maxDedicatedVideoMemory = 0;

	if (FAILED(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory)))) { ::DestroyWindow(this->hWnd); return; }
	for (uint32_t i = 0; dxgiFactory->EnumAdapters(i, &t_dxgiAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {

		if (FAILED(t_dxgiAdapter->GetDesc(&dxgiAdapterDescription))) continue; 
		if (SUCCEEDED(D3D12CreateDevice(t_dxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)) &&
			maxDedicatedVideoMemory < dxgiAdapterDescription.DedicatedVideoMemory) {

			maxDedicatedVideoMemory = dxgiAdapterDescription.DedicatedVideoMemory;
			dxgiAdapter = t_dxgiAdapter;

		}

	}
	if (dxgiAdapter == nullptr) { ::DestroyWindow(this->hWnd); return; }
	
	// -- Device

	if(FAILED(D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&D12Device)))) { ::DestroyWindow(this->hWnd); return; }

	// -- Command queues

	directCQ = std::make_shared<CommandQueue>(D12Device, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL); 
	copyCQ = std::make_shared<CommandQueue>(D12Device, D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL); 

	// -- Swap chain

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> D12DirectCQ = this->directCQ->GetCommandQueue();
	
	BOOL tearingAllowed = FALSE;
	dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, (void*) &tearingAllowed, sizeof(tearingAllowed));
	
	DXGI_SWAP_CHAIN_DESC1 swapChainDescription = {};
	swapChainDescription.Width = nWidth;
	swapChainDescription.Height = nHeight;
	swapChainDescription.Format = SWAP_CHAIN_BUFFER_FORMAT;
	swapChainDescription.Stereo = FALSE;
	swapChainDescription.SampleDesc = { 1, 0 };
	swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDescription.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	swapChainDescription.Scaling = DXGI_SCALING_STRETCH;
	swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDescription.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDescription.Flags = tearingAllowed ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	
	IDXGISwapChain1* swapChain = nullptr;
	if (FAILED(dxgiFactory->CreateSwapChainForHwnd(D12DirectCQ.Get(), this->hWnd, &swapChainDescription, nullptr, nullptr, &swapChain))) { ::DestroyWindow(this->hWnd); return; }
	this->DXGISwapChain = reinterpret_cast<IDXGISwapChain3*>(swapChain);
	swapChain->Release();

	// -- Render target views

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDescription = {};
	descriptorHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDescription.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
	descriptorHeapDescription.NodeMask = 0;
	descriptorHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	if (FAILED(this->D12Device->CreateDescriptorHeap(&descriptorHeapDescription, IID_PPV_ARGS(&this->D12RTVDescHeap)))) { ::DestroyWindow(this->hWnd); return; }

	D3D12_CPU_DESCRIPTOR_HANDLE RTVDescriptor = this->D12RTVDescHeap->GetCPUDescriptorHandleForHeapStart();
	UINT RTVDescriptorIncrementSize = D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {

		if(FAILED(this->DXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&this->swapChainBuffers[i])))) { ::DestroyWindow(this->hWnd); return; }
		this->D12Device->CreateRenderTargetView(this->swapChainBuffers[i].Get(), nullptr, RTVDescriptor);

		RTVDescriptor.ptr = SIZE_T(UINT64(RTVDescriptor.ptr) + UINT64(RTVDescriptorIncrementSize));

	}

	RootSignature::SetRootSignatureVer(this->D12Device);

	return;
}

DX12Window::~DX12Window() {

	if (this->directCQ != nullptr) this->directCQ->Flush();
	if (this->copyCQ != nullptr) this->copyCQ->Flush();

	printf("[INFO] \"%ws\" | DX12Window::~DX12Window().\n", windowName);

}

void DX12Window::InitDebug() {

	// -- Debug layer

	D3D12GetDebugInterface(IID_PPV_ARGS(&D12Debug));
	D12Debug->EnableDebugLayer();

	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&DXGIDebug));

}

void DX12Window::ShutdownDebug() {

	DXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
	D12Debug.Reset();
	DXGIDebug.Reset();

	return;

}


LRESULT DX12Window::OnPaint(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	this->OnUpdate();
	this->OnRender();

	return S_OK;

}		

LRESULT DX12Window::OnMouseWheel(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	DX12Layer_Event_MouseWheel e_mousewheel = { ((SHORT) HIWORD(wParam)) / (FLOAT) WHEEL_DELTA };
	DX12Layer_Event e = {};
	e.type = DX12Layer_Event_Type::DX12LAYER_EVENT_TYPE_MOUSEWHEEL;
	e.data = reinterpret_cast<void*>(&e_mousewheel);
	if (layer != nullptr) return layer->OnEvent(e);
	return S_OK;	

}

LRESULT DX12Window::OnMouseMove(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	DX12Layer_Event_MouseMove e_mousemove = { LOWORD(lParam), HIWORD(lParam), static_cast<int>(wParam) };
	DX12Layer_Event e = {};
	e.type = DX12Layer_Event_Type::DX12LAYER_EVENT_TYPE_MOUSEMOVE;
	e.data = reinterpret_cast<void*>(&e_mousemove);
	if (layer != nullptr) return layer->OnEvent(e);

	return S_OK;

}
		

LRESULT DX12Window::OnCustomEventOther(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	
	DX12Wnd_CustomEvent_Other* e = reinterpret_cast<DX12Wnd_CustomEvent_Other*>(lParam);
	switch(e->type) {
		case DX12Wnd_CustomEvent_Other_Type::DX12WND_CUSTOMEVENT_OTHER_TYPE_DX12LAYER:
			if (layer != nullptr) return layer->OnEvent(*reinterpret_cast<DX12Layer_Event*>(e->data));
			break;
		default:
			break;
	}

	return S_OK;

}

LRESULT DX12Window::LayerMessage(TWHANDLE handle, DX12Layer_Event_Type t, void* data) {
	
	if (auto err = GetWndObject(handle); !err.has_value()) {
		return S_OK;
	}
	else {

		std::shared_ptr<ThreadedWindow> thWnd = *err;
		DX12Window* dx12Wnd = dynamic_cast<DX12Window*>(thWnd.get());
	
		DX12Layer_Event e;
		e.type = t;
		e.data = data;
		DX12Wnd_CustomEvent_Other dx_wnd_e;
		dx_wnd_e.type = DX12Wnd_CustomEvent_Other_Type::DX12WND_CUSTOMEVENT_OTHER_TYPE_DX12LAYER;
		dx_wnd_e.data = reinterpret_cast<void*>(&e);
		return ThreadedWindow::CustomEventOtherWnd(dx12Wnd->GetHwnd(), reinterpret_cast<void*>(&dx_wnd_e));

	}

}

void DX12Window::OnUpdate() {

	LRESULT res = S_OK;
	if (layer != nullptr)
		res = layer->OnUpdate(0.0f);
	return;

}

void DX12Window::OnRender() {

	//Get command list
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = this->directCQ->GetCommandList();
	
	//Get current back buffer index and the corresponding render target view descriptor
	UINT currentBackBufferIndex = this->DXGISwapChain->GetCurrentBackBufferIndex();
	UINT RTVDescriptorSize = this->D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE RTVDescriptor = this->D12RTVDescHeap->GetCPUDescriptorHandleForHeapStart();
	RTVDescriptor.ptr = SIZE_T(UINT64(RTVDescriptor.ptr) + UINT64(RTVDescriptorSize) * currentBackBufferIndex);

	LRESULT res = S_OK;
	if (layer != nullptr)
		res = layer->OnRender(commandList, this->swapChainBuffers[currentBackBufferIndex], RTVDescriptor);

	UINT64 fenceValue = this->directCQ->ExecuteCommandList(commandList);
	this->fenceValues[currentBackBufferIndex] = fenceValue;
	this->DXGISwapChain->Present(1, 0);
	this->directCQ->WaitForFenceValue(this->fenceValues[this->DXGISwapChain->GetCurrentBackBufferIndex()]);

}
