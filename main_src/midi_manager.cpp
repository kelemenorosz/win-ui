#include "midi_manager.h"
#include <thread>
#include <cstdio>
#include <unordered_map>
#include <mutex>
#include <queue>

void CALLBACK MidiCallback(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

struct MidiMessage_Internal {

	UINT8 interface_index;
	DWORD_PTR dwp1;
	DWORD_PTR dwp2;

};

struct MidiInterface {

	MidiInterface() { printf("[INFO] MidiInterface::MidiInterface().\n"); }
	MidiInterface(HMIDIIN handle) : handle(handle), token(0) {}
	MidiInterface(HMIDIIN handle, std::function<void(MidiMessage)> func) : handle(handle), token(0) {
		callback_list.emplace(token, func);
		token++;
	}
	~MidiInterface() = default;

	UINT64 InsertCallback(std::function<void(MidiMessage)> func) {
		callback_list.emplace(token, func);
		token++;
		return token - 1;
	}

	bool DeleteCallback(UINT64 token) {
		int ret = callback_list.erase(token);
		if (ret == 0) return false;
		return true;
	}

	HMIDIIN handle;
	UINT64 token;
	std::unordered_map<UINT64, std::function<void(MidiMessage)>> callback_list;

};

struct MidiManager {

	MidiManager() : isRunning(true) {}
	~MidiManager() = default;
	void operator()();

	std::unordered_map<UINT8, MidiInterface> interface_map;
	std::queue<MidiMessage_Internal> q;

	bool isRunning;

};

std::thread g_midi_thread;
MidiManager g_midimanager;
std::mutex g_midi_mutex_map;
std::mutex g_midi_mutex_queue;
std::mutex g_midi_mutex_cv;
std::condition_variable g_midi_cv;

void StartMidiManager() {

	g_midi_thread = std::thread(&MidiManager::operator(), &g_midimanager);

	return;

}

void StopMidiManager() {

	printf("[INFO] StopMidiManager().\n");

	g_midimanager.isRunning = false;
	g_midi_cv.notify_all();
	g_midi_thread.join();

	return;

}

MMRESULT ListMidiInterfaces(MidiInterfaceList* interfaceList) {

	if (interfaceList->count == 0) {
		interfaceList->count = midiInGetNumDevs();
		return MMSYSERR_NOERROR;
	}
	else {
		for (int i = 0; i < interfaceList->count; ++i) {
			MIDIINCAPS midiCaps = {};
			MMRESULT res = midiInGetDevCaps(i, &midiCaps, sizeof(MIDIINCAPS));
			if (res != MMSYSERR_NOERROR) {
				interfaceList->data[i].index = -1;
				continue;
			}
			interfaceList->data[i].index = i;
			wcscpy(interfaceList->data[i].stringDesc, midiCaps.szPname);
		}
	}

	return MMSYSERR_NOERROR;

}

MMRESULT MidiManagerCheck(int index) {

	{
		std::lock_guard<std::mutex> lg(g_midi_mutex_map);

		MMRESULT mm_res = MMSYSERR_NOERROR;
		HMIDIIN handle;
		mm_res = midiInOpen(&handle, 0, NULL, NULL, CALLBACK_NULL);
	
	}

	return MMSYSERR_NOERROR;

}

MMRESULT MidiManagerSubscribe(int index, std::function<void(MidiMessage)> func, UINT64* token) {

	{
		std::lock_guard<std::mutex> lg(g_midi_mutex_map);
		if (!g_midimanager.interface_map.contains(index)) {

			MMRESULT mm_res = MMSYSERR_NOERROR;
			HMIDIIN handle;

			mm_res = midiInOpen(&handle, index, reinterpret_cast<DWORD_PTR>(MidiCallback), index, CALLBACK_FUNCTION);
			if (mm_res != MMSYSERR_NOERROR) return mm_res; 

			mm_res = midiInStart(handle);
			if (mm_res != MMSYSERR_NOERROR) return mm_res;
		
			g_midimanager.interface_map.emplace(std::piecewise_construct, std::forward_as_tuple(index), std::forward_as_tuple(handle, func));
		
			*token = 0;

		}
		else {

			// -- Once opened with midiInOpen(), the device isn't closed even after it is plugged out.
			// -- Thus, close and reopen each time MidiManagerSubscribe() is invoked.
			// -- If the device was not plugged out and plugged in in the meantime, nothing should change.
			// -- If the device was removed and inserted again, there is no guarantee that the ID of the device is the same after insertion.
			// -- Thus closing and reopening the device might switch devices if multiple are present.
		
			MidiInterface& midi_interface = g_midimanager.interface_map.at(index); 
	
			MMRESULT mm_res = MMSYSERR_NOERROR;

			mm_res = midiInReset(midi_interface.handle);
			if (mm_res != MMSYSERR_NOERROR) {
				printf("[ERROR] MidiManagerSubscribe() midiInReset() failed.\n");
			}

			mm_res = midiInStop(midi_interface.handle);
			if (mm_res != MMSYSERR_NOERROR) {
				printf("[ERROR] MidiManagerSubscribe() midiInStop() failed.\n");
			}

			mm_res = midiInClose(midi_interface.handle);
			if (mm_res != MMSYSERR_NOERROR) {
				printf("[ERROR] MidiManagerSubscribe() midiInClose() failed.\n");
			}

			HMIDIIN handle;

			mm_res = midiInOpen(&handle, index, reinterpret_cast<DWORD_PTR>(MidiCallback), index, CALLBACK_FUNCTION);
			if (mm_res != MMSYSERR_NOERROR) return mm_res; 

			mm_res = midiInStart(handle);
			if (mm_res != MMSYSERR_NOERROR) return mm_res;

			midi_interface.handle = handle;
			*token = midi_interface.InsertCallback(func);
		
		}
	}

	return MMSYSERR_NOERROR;

}

MMRESULT MidiMagagerUnsubscribe(int index, UINT64 token) {

	if (g_midimanager.interface_map.contains(index) == false) {
		// -- No interface with index 'index'
		return MMSYSERR_INVALPARAM;
	}

	MidiInterface& midi_interface = g_midimanager.interface_map.at(index); 
	bool ret = midi_interface.DeleteCallback(token);

	if (ret == false) {
		// -- No callback registered with token 'token'
		return MMSYSERR_INVALPARAM;
	}

	// -- Close interface if no callbacks are registered

	return MMSYSERR_NOERROR;

}

void MidiManager::operator()() {
	
	printf("[INFO] MidiManager::operator()().\n");

	while (isRunning) {

		printf("[INFO] Hello.\n");

		{
			std::lock_guard<std::mutex> lg_map(g_midi_mutex_map);
			std::lock_guard<std::mutex> lg_q(g_midi_mutex_queue);
			
			while (!this->q.empty()) {
				MidiMessage_Internal msg = this->q.front();
				for (auto& it : this->interface_map[msg.interface_index].callback_list) {
					it.second(MidiMessage(msg.dwp1, msg.dwp2));
				}
				this->q.pop();
			}			

		}

		std::unique_lock<std::mutex> ul(g_midi_mutex_cv);
		g_midi_cv.wait(ul);

	}

	return;
	
}

void CALLBACK MidiCallback(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {

	printf("[INFO] MidiCallback().\n");

	switch (wMsg) {

		case MIM_OPEN:
			// printf("[INFO] MIM_OPEN.\n");
			break;
		case MIM_CLOSE:
			// printf("[INFO] MIM_CLOSE.\n");
			break;
		case MIM_DATA:
			{
				std::lock_guard<std::mutex> lg(g_midi_mutex_queue);
				g_midimanager.q.emplace(static_cast<UINT8>(dwInstance), dwParam1, dwParam2);
				g_midi_cv.notify_all();
			}
			break;
		case MIM_LONGDATA:
			// printf("[INFO] MIM_LONGDATA.\n");
			break;
		case MIM_ERROR:
			// printf("[INFO] MIM_ERROR.\n");
			break;
		case MIM_LONGERROR:
			// printf("[INFO] MIM_LONGERROR.\n");
			break;
		case MIM_MOREDATA:
			// printf("[INFO] MIM_MOREDATA.\n");
			break;
		default:
			// printf("[ERROR] MidiCallback() wMsg default switch.\n");
			break;
	}

	return;

}