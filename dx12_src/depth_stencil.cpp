
#include "directx/d3dx12.h"
#include <d3d12.h>
#include "depth_stencil.h"
#include "resource.h"
#include <cstdio>

DepthStencil::DepthStencil(Microsoft::WRL::ComPtr<ID3D12Device2> dev, int width, int height, DXGI_FORMAT format, UINT sampleCount) : depthBuf(nullptr) {

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};

	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NodeMask = 0;

	HRESULT hr = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&this->descHeap));
	if (hr != S_OK) {
		printf("CreateDescriptorHeap() failed with error code: %d. \n", hr);
	}

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	depthBuf = std::make_unique<Resource>(dev, RESOURCE_NO_READBACK | RESOURCE_NO_UPLOAD, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_DIMENSION_TEXTURE2D, width, height,
	 D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, 1, 1, format, sampleCount, 0, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &clearValue);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDescription = {};
	dsvDescription.Flags = D3D12_DSV_FLAG_NONE;
	dsvDescription.Format = format;
	if (sampleCount == 1) {
		dsvDescription.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	}
	else {
		dsvDescription.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	}
	dsvDescription.Texture2D.MipSlice = 0;

	dev->CreateDepthStencilView(this->depthBuf->GetDefaultBuffer().Get(), &dsvDescription, this->descHeap->GetCPUDescriptorHandleForHeapStart());

	return;

}


D3D12_CPU_DESCRIPTOR_HANDLE DepthStencil::GetHandle() {

	return descHeap->GetCPUDescriptorHandleForHeapStart();

}