#pragma once

#include "directx/d3dx12.h"
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include "resource.h"

class DepthStencil {

	public:

		DepthStencil(Microsoft::WRL::ComPtr<ID3D12Device2> dev, int width, int height, DXGI_FORMAT format, UINT sampleCount = 1);
		~DepthStencil() {}

		D3D12_CPU_DESCRIPTOR_HANDLE GetHandle();

	private:

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descHeap; 
		std::unique_ptr<Resource> depthBuf; 

};