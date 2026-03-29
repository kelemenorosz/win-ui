#include <commandqueue.h>

//DirectX includes
#include <d3d12.h>
#include <wrl.h>

//Include chrono
#if defined(max)
#undef max
#endif

#include <chrono>

namespace mWRL = Microsoft::WRL;

inline void ThrowIfFailed(HRESULT result) {

	if (FAILED(result)) throw std::exception();

}

//Safe release COM pointers
template<class T> void SafeRelease(T** ptr) {

	if (*ptr != nullptr) {

		(*ptr)->Release();
		(*ptr) = nullptr;

	}
	return;

}

CommandQueue::CommandQueue(mWRL::ComPtr<ID3D12Device> dxDevice, D3D12_COMMAND_LIST_TYPE commandListType, D3D12_COMMAND_QUEUE_PRIORITY commandQueuePriority) : m_fenceValue(0) {

	m_dxDevice = dxDevice;
	m_commandListType = commandListType;

	//Create command queue
	D3D12_COMMAND_QUEUE_DESC commandQueueDescription = {};
	commandQueueDescription.Type = m_commandListType;
	commandQueueDescription.Priority = commandQueuePriority;
	commandQueueDescription.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDescription.NodeMask = 0;
	ThrowIfFailed(m_dxDevice->CreateCommandQueue(&commandQueueDescription, IID_PPV_ARGS(&m_commandQueue)));

	//Create fence
	ThrowIfFailed(m_dxDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

	//Create event
	m_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);

}

CommandQueue::~CommandQueue() {

	this->Flush();
	::CloseHandle(m_event);

}

mWRL::ComPtr<ID3D12CommandQueue> CommandQueue::GetCommandQueue() { return m_commandQueue; }

mWRL::ComPtr<ID3D12GraphicsCommandList> CommandQueue::GetCommandList() {

	ID3D12CommandAllocator* commandAllocator = nullptr;
	ID3D12GraphicsCommandList* commandList = nullptr;

	//Get command allocator
	if (!m_commandAllocatorQueue.empty() && IsFenceComplete(m_commandAllocatorQueue.front().fenceValue)) {

		commandAllocator = m_commandAllocatorQueue.front().commandAllocator.Get();
		commandAllocator->AddRef();
		m_commandAllocatorQueue.pop();
		ThrowIfFailed(commandAllocator->Reset());

	}
	else {

		ThrowIfFailed(m_dxDevice->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(&commandAllocator)));

	}

	//Get command list
	if (!m_commandListQueue.empty()) {

		commandList = m_commandListQueue.front().Get();
		commandList->AddRef();
		m_commandListQueue.pop();
		ThrowIfFailed(commandList->Reset(commandAllocator, nullptr));

	}
	else {

		ThrowIfFailed(m_dxDevice->CreateCommandList(0, m_commandListType, commandAllocator, nullptr, IID_PPV_ARGS(&commandList)));

	}

	ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator));

	mWRL::ComPtr<ID3D12GraphicsCommandList> commandListCOM = commandList;
	SafeRelease(&commandAllocator);
	SafeRelease(&commandList);

	return commandListCOM;

}

UINT64 CommandQueue::ExecuteCommandList(mWRL::ComPtr<ID3D12GraphicsCommandList> commandList) {

	//Close the command list
	ThrowIfFailed(commandList->Close());

	//Get the command allocator
	ID3D12CommandAllocator* commandAllocator = nullptr;
	UINT commandAllocatorSize = sizeof(ID3D12CommandAllocator);
	ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &commandAllocatorSize, &commandAllocator));

	//Execute command list
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	//increment fence value and signal the fence
	++m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));

	//Add the command list and allocator to queue
	m_commandListQueue.push(commandList);
	m_commandAllocatorQueue.emplace(CommandAllocator{ m_fenceValue, commandAllocator });

	SafeRelease(&commandAllocator);

	return m_fenceValue;

}

void CommandQueue::WaitForFenceValue(UINT64 fenceValue) {

	if (!IsFenceComplete(fenceValue)) {

		ThrowIfFailed(m_fence->SetEventOnCompletion(fenceValue, m_event));
		::WaitForSingleObject(m_event, static_cast<DWORD>(std::chrono::milliseconds::max().count()));

	}

}

void CommandQueue::Flush() {

	//Increment fence value and signal the fence
	++m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));
	this->WaitForFenceValue(m_fenceValue);

}



BOOL CommandQueue::IsFenceComplete(UINT64 fenceValue) {

	return (m_fence->GetCompletedValue() >= fenceValue);

}