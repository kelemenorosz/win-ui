
#include "resource.h"
#include <d3d12.h>
#include <wrl.h>

namespace mWRL = Microsoft::WRL;

inline void ThrowIfFailed(HRESULT result) {

	if (FAILED(result)) throw std::exception();

}

Resource::Resource(mWRL::ComPtr<ID3D12Device2> device, RESOURCE_CREATION_FLAG creationFlag, D3D12_RESOURCE_STATES resourceState,
	D3D12_RESOURCE_DIMENSION dimension, UINT64 width, UINT height, UINT64 alignment, UINT16 depthOrArraySize,
	UINT16 mipLevels, DXGI_FORMAT format, UINT sampleCount, UINT sampleQuality,
	D3D12_TEXTURE_LAYOUT layout, D3D12_RESOURCE_FLAGS flags, D3D12_CLEAR_VALUE* clearValue) : m_device(device) {

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 0;
	heapProperties.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resourceDescription = {};
	resourceDescription.Dimension = dimension;
	resourceDescription.Alignment = alignment;
	resourceDescription.Width = width;
	resourceDescription.Height = height;
	resourceDescription.DepthOrArraySize = depthOrArraySize;
	resourceDescription.MipLevels = mipLevels;
	resourceDescription.Format = format;
	resourceDescription.SampleDesc.Count = sampleCount;
	resourceDescription.SampleDesc.Quality = sampleQuality;
	resourceDescription.Layout = layout;
	resourceDescription.Flags = flags;

	//RESOURCE_NO_READBACK flag disabled
	if (!(creationFlag & RESOURCE_NO_READBACK)) {

		heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
		resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		if (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {

			resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;
			ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, D3D12_RESOURCE_STATE_COPY_DEST, clearValue, IID_PPV_ARGS(&m_readback)));

		}
		else if (dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {

			resourceDescription.Width = width * height * 4;
			resourceDescription.Height = 1;
			resourceDescription.Format = DXGI_FORMAT_UNKNOWN;
			resourceDescription.MipLevels = 1;
			resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;
			ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, D3D12_RESOURCE_STATE_COPY_DEST, clearValue, IID_PPV_ARGS(&m_readback)));

		}

	}

	//RESOURCE_NO_UPLOAD flag disabled
	if (!(creationFlag & RESOURCE_NO_UPLOAD)) {

		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

		if (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {

			resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;
			ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, D3D12_RESOURCE_STATE_GENERIC_READ, clearValue, IID_PPV_ARGS(&m_upload)));

		}
		else if (dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {

			resourceDescription.Width = width * height * 4;
			resourceDescription.Height = 1;
			resourceDescription.Format = DXGI_FORMAT_UNKNOWN;
			resourceDescription.MipLevels = 1;
			resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;
			ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, D3D12_RESOURCE_STATE_GENERIC_READ, clearValue, IID_PPV_ARGS(&m_upload)));

		}

	}

	//RESOURCE_NO_DEFAULT flag disabled
	if (!(creationFlag & RESOURCE_NO_DEFAULT)) {

		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		resourceDescription.Dimension = dimension;
		resourceDescription.Width = width;
		resourceDescription.Height = height;
		resourceDescription.Format = format;
		resourceDescription.MipLevels = mipLevels;
		resourceDescription.Layout = layout;
		resourceDescription.Flags = flags;
		ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, resourceState, clearValue, IID_PPV_ARGS(&m_default)));

	}

}

Resource::~Resource() {



}

void Resource::MapUploadBufferPtr(UINT subresource, void** bufferPtr) {

	if (m_upload == nullptr) {

		*bufferPtr = nullptr;
		return;

	}

	ThrowIfFailed(m_upload->Map(subresource, nullptr, bufferPtr));

}

void Resource::MapReadbackBufferPtr(UINT subresource, void** bufferPtr, D3D12_RANGE* range) {

	if (m_readback == nullptr) {

		throw std::exception();

	}

	ThrowIfFailed(m_readback->Map(subresource, range, bufferPtr));

}

void Resource::UnmapUploadBufferPtr(UINT subresource, D3D12_RANGE* range) {

	m_upload->Unmap(subresource, range);

}

void Resource::UnmapReadbackBufferPtr(UINT subresource) {

	m_readback->Unmap(subresource, nullptr);

}

void Resource::CopyUploadToDefault(mWRL::ComPtr<ID3D12GraphicsCommandList> commandList) {

	D3D12_RESOURCE_DESC defaultBufferDescription = m_default->GetDesc();

	if (defaultBufferDescription.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {

		throw std::exception();

	}

	commandList->CopyResource(m_default.Get(), m_upload.Get());
	
}

void Resource::CopyUploadToDefault(mWRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT subresource) {

	D3D12_RESOURCE_DESC defaultBufferDescription = m_default->GetDesc();

	if (defaultBufferDescription.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) {

		throw std::exception();

	}

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* placedSubresourceFootprints = new D3D12_PLACED_SUBRESOURCE_FOOTPRINT[defaultBufferDescription.MipLevels];
	m_device->GetCopyableFootprints(&defaultBufferDescription, 0, defaultBufferDescription.MipLevels, 0, placedSubresourceFootprints, nullptr, nullptr, nullptr);

	D3D12_TEXTURE_COPY_LOCATION destinationTextureCopyLocation = {};
	D3D12_TEXTURE_COPY_LOCATION sourceTextureCopyLocation = {};

	destinationTextureCopyLocation.pResource = m_default.Get();
	destinationTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	destinationTextureCopyLocation.SubresourceIndex = subresource;

	sourceTextureCopyLocation.pResource = m_upload.Get();
	sourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	sourceTextureCopyLocation.PlacedFootprint = placedSubresourceFootprints[subresource];

	commandList->CopyTextureRegion(&destinationTextureCopyLocation, 0, 0, 0, &sourceTextureCopyLocation, nullptr);

	delete[] placedSubresourceFootprints;

}

void Resource::CopyDefaultToReadback(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {

	D3D12_RESOURCE_DESC defaultBufferDescription = m_default->GetDesc();

	if (defaultBufferDescription.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {

		throw std::exception();

	}

	commandList->CopyResource(m_readback.Get(), m_default.Get());

}

void Resource::CopyDefaultToReadback(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT subresource) {

	D3D12_RESOURCE_DESC defaultBufferDescription = m_default->GetDesc();

	if (defaultBufferDescription.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) {

		throw std::exception();

	}

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* placedSubresourceFootprints = new D3D12_PLACED_SUBRESOURCE_FOOTPRINT[defaultBufferDescription.MipLevels];
	m_device->GetCopyableFootprints(&defaultBufferDescription, 0, defaultBufferDescription.MipLevels, 0, placedSubresourceFootprints, nullptr, nullptr, nullptr);

	D3D12_TEXTURE_COPY_LOCATION destinationTextureCopyLocation = {};
	D3D12_TEXTURE_COPY_LOCATION sourceTextureCopyLocation = {};

	destinationTextureCopyLocation.pResource = m_readback.Get();
	destinationTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	destinationTextureCopyLocation.PlacedFootprint = placedSubresourceFootprints[subresource];

	sourceTextureCopyLocation.pResource = m_default.Get();
	sourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	sourceTextureCopyLocation.SubresourceIndex = subresource;

	//Set m_default subresource state to D3D12_RESOURCE_STATE_COPY_SOURCE
	{
		D3D12_RESOURCE_BARRIER resourceBarrier = {};
		resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		resourceBarrier.Transition.pResource = m_default.Get();
		resourceBarrier.Transition.Subresource = subresource;
		resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		commandList->ResourceBarrier(1, &resourceBarrier);
	}

	commandList->CopyTextureRegion(&destinationTextureCopyLocation, 0, 0, 0, &sourceTextureCopyLocation, nullptr);

	//Set m_default subresource state to D3D12_RESOURCE_STATE_COPY_DEST
	{
		D3D12_RESOURCE_BARRIER resourceBarrier = {};
		resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		resourceBarrier.Transition.pResource = m_default.Get();
		resourceBarrier.Transition.Subresource = subresource;
		resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		commandList->ResourceBarrier(1, &resourceBarrier);
	}

	delete[] placedSubresourceFootprints;

}

D3D12_GPU_VIRTUAL_ADDRESS Resource::GetDefaultGPUVirtualAddress() {

	if (m_default == nullptr) {

		throw std::exception();

	}

	return m_default->GetGPUVirtualAddress();

}

mWRL::ComPtr<ID3D12Resource> Resource::GetDefaultBuffer() {

	return m_default;

}

void Resource::SetVertexBufferView(UINT sizeInBytes, UINT strideInBytes) {

	vb_view.BufferLocation = this->GetDefaultGPUVirtualAddress();
	vb_view.SizeInBytes = sizeInBytes;
	vb_view.StrideInBytes = strideInBytes;

}

D3D12_VERTEX_BUFFER_VIEW* Resource::pGetVertexBufferView() {
	return &this->vb_view;
}

D3D12_VERTEX_BUFFER_VIEW Resource::GetVertexBufferView() {
	return this->vb_view;
}

void Resource::TransitionBackBuffer(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer,
	D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) {

	D3D12_RESOURCE_BARRIER resourceBarrier = {};
	resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	resourceBarrier.Transition.pResource = backBuffer.Get();
	resourceBarrier.Transition.Subresource = 0;
	resourceBarrier.Transition.StateBefore = stateBefore;
	resourceBarrier.Transition.StateAfter = stateAfter;
	commandList->ResourceBarrier(1, &resourceBarrier);

}

HRESULT Resource::CreateDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device2> dev, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>* descHeap,
	D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags, UINT nodeMask) {

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Type = type;
	descHeapDesc.NumDescriptors = numDescriptors;
	descHeapDesc.Flags = flags;
	descHeapDesc.NodeMask = nodeMask;

	HRESULT hr = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&(*descHeap)));
	if (FAILED(hr)) {
		printf("[ERROR] Failed to CreateDescriptorHeap().\n");
	}
	return hr;

}

D3D12_CPU_DESCRIPTOR_HANDLE Resource::CreateRenderTargetView(Microsoft::WRL::ComPtr<ID3D12Device2> dev, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descHeap,
	UINT descPos, Microsoft::WRL::ComPtr<ID3D12Resource> resource){

	UINT RTVDescriptorIncrementSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor = descHeap->GetCPUDescriptorHandleForHeapStart();
	rtv_descriptor.ptr = SIZE_T(UINT64(rtv_descriptor.ptr) + (UINT64(RTVDescriptorIncrementSize) * UINT64(descPos)));

	dev->CreateRenderTargetView(resource.Get(), nullptr, rtv_descriptor);
	return rtv_descriptor;
}

D3D12_CPU_DESCRIPTOR_HANDLE Resource::GetRenderTargetViewHandle(Microsoft::WRL::ComPtr<ID3D12Device2>dev, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descHeap, UINT descPos) {
	
	UINT RTVDescriptorSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE RTVDescriptor = descHeap->GetCPUDescriptorHandleForHeapStart();
	RTVDescriptor.ptr = SIZE_T(UINT64(RTVDescriptor.ptr) + (UINT64(RTVDescriptorSize) * UINT64(descPos)));
	return RTVDescriptor;

}