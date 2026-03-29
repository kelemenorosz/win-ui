#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <memory>

typedef uint8_t RESOURCE_CREATION_FLAG;

constexpr RESOURCE_CREATION_FLAG RESOURCE_NO_FLAG = 0x0;
constexpr RESOURCE_CREATION_FLAG RESOURCE_NO_READBACK = 0x1;
constexpr RESOURCE_CREATION_FLAG RESOURCE_NO_UPLOAD = 0x2;
constexpr RESOURCE_CREATION_FLAG RESOURCE_NO_DEFAULT = 0x4;

class Resource {

	public:

		Resource(Microsoft::WRL::ComPtr<ID3D12Device2> device, RESOURCE_CREATION_FLAG creationFlag, D3D12_RESOURCE_STATES resourceState,
			D3D12_RESOURCE_DIMENSION dimension, UINT64 width, UINT height, UINT64 alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, UINT16 depthOrArraySize = 1,
			UINT16 mipLevels = 1, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, UINT sampleCount = 1, UINT sampleQuality = 0,
			D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, D3D12_CLEAR_VALUE* clearValue = nullptr);
		~Resource();
		void MapUploadBufferPtr(UINT subresource, void** bufferPtr);
		void MapReadbackBufferPtr(UINT subresource, void** bufferPtr, D3D12_RANGE* range);
		void UnmapUploadBufferPtr(UINT subresource, D3D12_RANGE* range);
		void UnmapReadbackBufferPtr(UINT subresource);
		void CopyUploadToDefault(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
		void CopyUploadToDefault(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT subresource);
		void CopyDefaultToReadback(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
		void CopyDefaultToReadback(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT subresource);
		D3D12_GPU_VIRTUAL_ADDRESS GetDefaultGPUVirtualAddress();
		Microsoft::WRL::ComPtr<ID3D12Resource> GetDefaultBuffer();
		void SetVertexBufferView(UINT sizeInBytes, UINT strideInBytes);
		D3D12_VERTEX_BUFFER_VIEW* pGetVertexBufferView();
		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();

		static void TransitionBackBuffer(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Resource> resource,
			D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);
		static HRESULT CreateDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device2> dev, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>* descHeap,
			D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags, UINT nodeMask = 0);
		static D3D12_CPU_DESCRIPTOR_HANDLE CreateRenderTargetView(Microsoft::WRL::ComPtr<ID3D12Device2> dev, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descHeap,
			UINT descPos, Microsoft::WRL::ComPtr<ID3D12Resource> resource);
		static D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetViewHandle(Microsoft::WRL::ComPtr<ID3D12Device2>dev, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descHeap, UINT descPos);

	private:

		Microsoft::WRL::ComPtr<ID3D12Resource> m_default = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_upload = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_readback = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Device2> m_device = nullptr;

		D3D12_VERTEX_BUFFER_VIEW vb_view;

};