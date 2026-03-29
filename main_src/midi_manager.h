#pragma once
#include <windows.h>
#include <functional>

struct MidiMessage {
	DWORD_PTR dwp1;
	DWORD_PTR dwp2;
};

struct MidiInterfaceDesc {
	int index;
	WCHAR stringDesc[MAXPNAMELEN];
};

struct MidiInterfaceList {
	int count;
	MidiInterfaceDesc* data;
};

void StartMidiManager();
void StopMidiManager();

MMRESULT ListMidiInterfaces(MidiInterfaceList* interfaceList);

MMRESULT MidiManagerSubscribe(int index, std::function<void(MidiMessage)> func, UINT64* token);
MMRESULT MidiMagagerUnsubscribe(int index, UINT64 token);