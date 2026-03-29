#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <queue>

class CommandQueue {

public:

	CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device> dxDevice, D3D12_COMMAND_LIST_TYPE commandListType, D3D12_COMMAND_QUEUE_PRIORITY commandQueuePriority);
	~CommandQueue();

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue();
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList();
	UINT64 ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	void WaitForFenceValue(UINT64 fenceValue);
	void Flush();

private:

	BOOL IsFenceComplete(UINT64 fenceValue);

	struct CommandAllocator {

		UINT64 fenceValue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

	};

	using CommandAllocatorQueue = std::queue<CommandAllocator>;
	using CommandListQueue = std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>>;

	Microsoft::WRL::ComPtr<ID3D12Device> m_dxDevice;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;

	HANDLE m_event;

	UINT64 m_fenceValue;

	D3D12_COMMAND_LIST_TYPE m_commandListType;

	CommandAllocatorQueue m_commandAllocatorQueue;
	CommandListQueue m_commandListQueue;

};
