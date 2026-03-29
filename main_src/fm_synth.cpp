
#include "fm_synth.h"
#include "button.h"
#include "trackbar.h"
#include "static_text.h"
#include "combobox.h"
#include "midi_manager.h"
#include <xaudio2.h>
#include <iostream>
#include <fstream>

float gFreqs[] = {
	2.0f,
	6.0f,
	8.0f,
	6.0f,
	8.0f,
	10.0f,
};

EnvSettings gEnvSettings[] = {
	{ 0.99f, 0.80f, 0.50f, 0.0f, 4200.0f, 3000.0f, 1200.0f, 4500.0f },
	{ 0.99f, 0.96f, 0.89f, 0.0f, 5500.0f, 9500.0f, 100.0f, 100.0f },
	{ 0.87f, 0.86f, 0.60f, 0.0f, 5400.0f, 8700.0f, 1004.0f, 100.0f },
	{ 0.99f, 0.90f, 0.70f, 0.0f, 6700.0f, 9200.0f, 2800.0f, 6000.0f },
	{ 0.99f, 0.65f, 0.60f, 0.0f, 8500.0f, 7000.0f, 9700.0f, 100.0f },
	{ 0.99f, 0.99f, 0.97f, 0.0f, 7300.0f, 7000.0f, 6000.0f, 100.0f },
};

FMSynth::FMSynth(LPCWSTR windowName, int x, int y, int nWidth, int nHeight)
	: ThreadedWindow(windowName, x, y, nWidth, nHeight), control_board(this, 200, 50, 0, 99, 0.01f, 100), menu(NULL), midi_menu(NULL), midi_menu_n(0), midi_sub_i(-1),
	menu_open(false) {

	menu = LoadMenu((HINSTANCE)::GetWindowLongPtr(this->hWnd, GWLP_HINSTANCE), MAKEINTRESOURCE(IDR_MYMENU));
	if (menu == NULL) {
		printf("[INFO] LoadMenu() failed.\n");
	}
	MENUITEMINFO menuitem_info = {};
	menuitem_info.cbSize = sizeof(MENUITEMINFO);
	menuitem_info.dwTypeData = nullptr;
	menuitem_info.fMask = MIIM_STRING | MIIM_SUBMENU;
	menuitem_info.fType = MFT_STRING;
	BOOL ret = 0;
	ret = GetMenuItemInfo(menu, 0, true, &menuitem_info);
	menuitem_info.cch++;
	menuitem_info.dwTypeData = new WCHAR[menuitem_info.cch];
	int menuitem_count = GetMenuItemCount(menu); 
	ret = GetMenuItemInfo(menu, 0, true, &menuitem_info);
	
	if (wcscmp(L"MIDI", menuitem_info.dwTypeData) == 0) midi_menu = menuitem_info.hSubMenu;
	if (midi_menu == NULL) {
		printf("[INFO] Failed to obtain MIDI submenu handle.\n");
	}

	delete[] menuitem_info.dwTypeData;

	SetMenu(this->hWnd, menu);

	float freq_base = 440.0f;

	for (int i = 0; i < 6; ++i) {
		control_board.SaveOp(i, gFreqs[i], &gEnvSettings[i]);
	}
	control_board.LoadOp(0);

	HRESULT hr;
	hr = ::XAudio2Create(this->xaudio_engine.GetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr)) { ::DestroyWindow(this->hWnd); return; }

	hr = this->xaudio_engine->CreateMasteringVoice(&this->mastering_voice, 1, 48000, 0, NULL, NULL, AudioCategory_SoundEffects);
	if (FAILED(hr)) { ::DestroyWindow(this->hWnd); return; }

	key_velocity_arr.fill(0);

	return;

}

LRESULT FMSynth::OnMidiInterfaceReload() {

	printf("[INFO] FMSynth::OnMidiInterfaceReload().\n");

	for (int i = 0; i < midi_menu_n; ++i) {
		DeleteMenu(midi_menu, ID_MIDI_INTERFACE_0 + i, MF_BYCOMMAND);
		DrawMenuBar(this->hWnd);
	}

	MidiInterfaceList interface_list = {};
	ListMidiInterfaces(&interface_list);

	if (interface_list.count <= midi_sub_i) {
		MidiMagagerUnsubscribe(midi_sub_i, this->midi_sub_token);
		midi_sub_i = -1;
		printf("Unsub.\n");
	}

	if (interface_list.count == 0) return S_OK;

	interface_list.data = new MidiInterfaceDesc[interface_list.count];
	ListMidiInterfaces(&interface_list);

	int actual_int_count = 0;
	for (int i = 0; i < interface_list.count; ++i) {
		if (interface_list.data[i].index != -1) {

			MENUITEMINFO menuitem_info = {};
			menuitem_info.cbSize = sizeof(MENUITEMINFO);
			menuitem_info.dwTypeData = &interface_list.data[i].stringDesc[0];
			menuitem_info.wID = ID_MIDI_INTERFACE_0 + actual_int_count;
			menuitem_info.fMask = MIIM_STRING | MIIM_ID;
			menuitem_info.fType = MFT_STRING;
			if (midi_sub_i == i) {
				menuitem_info.fMask |= MIIM_STATE;
				menuitem_info.fState = MFS_CHECKED;
			}
			InsertMenuItem(midi_menu, ID_MIDI_INTERFACE_0 + actual_int_count, false, &menuitem_info);
			DrawMenuBar(this->hWnd);
			actual_int_count++;

		}
	}
	midi_menu_n = actual_int_count;

	delete[] interface_list.data;
	return S_OK;

}

LRESULT FMSynth::OnCommand(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	printf("[INFO] FMSynth::OnCommand().\n");
	switch (LOWORD(wParam)) {
		case ID_MIDI_RELOAD:
			printf("[INFO] ID_MIDI_RELOAD.\n");
			return OnMidiInterfaceReload();
			break;
		default:
			break; 
	}

	if ((LOWORD(wParam) >= ID_MIDI_INTERFACE_0) && (LOWORD(wParam) < (ID_MIDI_INTERFACE_0 + midi_menu_n))) {

		int interface_index = LOWORD(wParam) - ID_MIDI_INTERFACE_0;

		MENUITEMINFO menuitem_info = {};
		menuitem_info.cbSize = sizeof(MENUITEMINFO);
		menuitem_info.fMask = MIIM_STATE;
		GetMenuItemInfo(midi_menu, LOWORD(wParam), false, &menuitem_info);
		if (menuitem_info.fState == MFS_CHECKED) {

			printf("[INFO] Checked.\n");
			// -- Unsub

			MidiMagagerUnsubscribe(midi_sub_i, this->midi_sub_token);
			midi_sub_i = -1;

			menuitem_info.cbSize = sizeof(MENUITEMINFO);
			menuitem_info.fMask = MIIM_STATE;
			menuitem_info.fState = MFS_UNCHECKED;
			SetMenuItemInfo(midi_menu, LOWORD(wParam), false, &menuitem_info);
			DrawMenuBar(this->hWnd);

		}
		else {

			// -- Unsub && Sub

			MidiMagagerUnsubscribe(midi_sub_i, this->midi_sub_token);
			midi_sub_i = -1;

			HWND hWnd = this->hWnd;
			MMRESULT res = MidiManagerSubscribe(interface_index, [hWnd](MidiMessage msg) { 
				MidiMessage* pMsg = new MidiMessage(msg);
				FMSynth_CustomEvent_Other* e = new FMSynth_CustomEvent_Other();
				e->type = FMSynth_CustomEvent_Other_Type::FMSYNTH_CUSTOMEVENT_OTHER_TYPE_MIDI_DATA;
				e->data = reinterpret_cast<void*>(pMsg);
				ThreadedWindow::CustomEventOtherWnd_Post(hWnd, reinterpret_cast<void*>(e));
				return; 
			}, &this->midi_sub_token);
			if (res != MMSYSERR_NOERROR) return S_OK;

			midi_sub_i = interface_index;
			
			menuitem_info.cbSize = sizeof(MENUITEMINFO);
			menuitem_info.fMask = MIIM_STATE;
			menuitem_info.fState = MFS_CHECKED;
			SetMenuItemInfo(midi_menu, LOWORD(wParam), false, &menuitem_info);
			DrawMenuBar(this->hWnd);

		}


	}

	return S_OK;

}

LRESULT FMSynth::OnMenuSelect(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	// printf("[INFO] FMSynth::OnMenuSelect(). loword: %u, hiword: %u.\n", LOWORD(wParam), HIWORD(wParam));

	switch (LOWORD(wParam)) {
		case 0:
			if (HIWORD(wParam) != 0xFFFF && menu_open == false) {
				menu_open = true;
				return OnMidiInterfaceReload();
			}
			else if (HIWORD(wParam) == 0xFFFF) {
				menu_open = false;
			}
			break;
		default:
			break;
	}

	return S_OK;

}

LRESULT FMSynth::OnCustomEventOther(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	FMSynth_CustomEvent_Other* e = reinterpret_cast<FMSynth_CustomEvent_Other*>(lParam);

	switch (e->type) {
		case FMSynth_CustomEvent_Other_Type::FMSYNTH_CUSTOMEVENT_OTHER_TYPE_MIDI_DATA:
			{
				MidiMessage* msg = reinterpret_cast<MidiMessage*>(e->data);
				
				UINT16 hiword = msg->dwp1 >> 0x10;
				UINT16 loword = msg->dwp1;

				UINT8 status = loword;
				UINT8 d1 = loword >> 0x8;
				UINT8 d2 = hiword;

				printf("[INFO] status: %u, d1: %u, d2: %u.\n", status, d1, d2);

				if (status == 144) {
					OnMidiKeyPress(d1, d2);
				}

				delete msg;
			}
			break;
		default:
			break;
	}

	delete e;
	return S_OK;

}

LRESULT FMSynth::OnMidiKeyPress(UINT8 key, UINT8 velocity) {

	key_velocity_arr[key]++;

	if (velocity == 0) {
		return S_OK;
	}
	
	control_board.SaveOp();
	EnvSettings* env_settings = new EnvSettings[6];
	float* freq_settings = new float[6];
	for (int i = 0; i < 6; ++i) {
		control_board.LoadOp(i, &freq_settings[i], &env_settings[i]);
	}

	std::array<int, 200>* pArr = &this->key_velocity_arr;
	int index = key_velocity_arr[key];
	voice_callback_vec.push_back(std::make_unique<FMSynthVoiceCallback>(xaudio_engine, [key, pArr, index](){ 
		if ((*pArr)[key] == index) {
			return true; 
		} 
		else {
			return false; 
		}
	}));
	voice_callback_vec[voice_callback_vec.size() - 1]->CreateSource();

	float key_freq = std::pow(2, (static_cast<float>(key) - 69.0f) / 12.0f) * 440.0f;

	std::shared_ptr<FMGen> fmgen = voice_callback_vec[voice_callback_vec.size() - 1]->CreateFMGen(key_freq, freq_settings, env_settings);

	delete[] env_settings;
	delete[] freq_settings;

	BYTE* dummy_buf = new BYTE[128];
	memset(dummy_buf, 0, 128);

	voice_callback_vec[voice_callback_vec.size() - 1]->SubmitBuffer(dummy_buf, 128 / sizeof(float));
	voice_callback_vec[voice_callback_vec.size() - 1]->Start();

	return S_OK;

}

FMSynth::~FMSynth() {

	if (this->mastering_voice != nullptr) this->mastering_voice->DestroyVoice();
	// DX12Window::ShutdownDebug();

	if (menu != NULL) {
		DestroyMenu(menu);
	}

	return;

}

void FMSynthVoiceCallback::OnBufferEnd(void* pBufferContext) {

	// fprintf(Logging::log_file, "[INFO] FMSynthVoiceCallback::OnBufferEnd().\n");

	delete[] (BYTE*)pBufferContext;
	
	if (ft.valid()) {

		BYTE* buf = ft.get();

		XAUDIO2_BUFFER xaudio_buffer = {};
		xaudio_buffer.Flags = 0;
		xaudio_buffer.AudioBytes = 1024 * 4;
		xaudio_buffer.pAudioData = reinterpret_cast<BYTE*>(buf);
		xaudio_buffer.PlayBegin = 0;
		xaudio_buffer.PlayLength = 0;
		xaudio_buffer.LoopBegin = 0;
		xaudio_buffer.LoopLength = 0;
		xaudio_buffer.LoopCount = 0;
		xaudio_buffer.pContext = reinterpret_cast<void*>(buf);

		HRESULT hr;
		hr = source_voice->SubmitSourceBuffer(&xaudio_buffer, nullptr);
		if (FAILED(hr)) {
			printf("IXAudio2SourceVoice::SubmitSourceBuffer() failed.\n");
			return;
		}

	}
	else {
		source_voice->Stop(0, XAUDIO2_COMMIT_NOW);
	}
	
	return;

}

void FMSynthVoiceCallback::OnBufferStart(void* pBufferContext) {

	// fprintf(Logging::log_file, "[INFO] FMSynthVoiceCallback::OnBufferStart().\n");

	if (fmgen->IsDone()) {
		fmgen->Reset();
		ft = std::future<BYTE*>();
	}
	else {
		ft = std::async(std::launch::async, &FMGen::operator(), fmgen.get(), 1024);
	}

	return;

}

HRESULT FMSynthVoiceCallback::CreateSource() {

	WAVEFORMATEX waveformat = {};
	waveformat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	waveformat.nChannels = 1;
	waveformat.nSamplesPerSec = 48000;
	waveformat.wBitsPerSample = 32;
	waveformat.nBlockAlign = (waveformat.nChannels * waveformat.wBitsPerSample) / 8;
	waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec * waveformat.nBlockAlign;
	waveformat.cbSize = 0;

	HRESULT hr;
	hr = audio_engine->CreateSourceVoice(&source_voice, &waveformat, 0, XAUDIO2_MAX_FREQ_RATIO, this, nullptr, nullptr);

	return hr;

}

std::shared_ptr<FMGen> FMSynthVoiceCallback::CreateFMGen(float freq) {

	for (int i = 0; i < 6; ++i) {
		ops.push_back(std::make_shared<FreqMod>(freq * gFreqs[i], 48000.0f));
	}

	ops[0]->AddMod(ops[1]);
	ops[0]->AddMod(ops[2]);
	ops[0]->AddMod(ops[4]);
	ops[2]->AddMod(ops[3]);
	ops[4]->AddMod(ops[5]);

	fmgen = std::make_shared<FMGen>(sustain_callback);
	fmgen->AddFM(ops[0]);

	for (int i = 0; i < 6; ++i) {
		ops[i]->SetEnv(&gEnvSettings[i]);
	}

	return fmgen;

}

std::shared_ptr<FMGen> FMSynthVoiceCallback::CreateFMGen(float freq, float* freq_settings, EnvSettings* env_settings) {

	for (int i = 0; i < 6; ++i) {
		ops.push_back(std::make_shared<FreqMod>(freq * freq_settings[i], 48000.0f));
	}

	ops[0]->AddMod(ops[2]);
	ops[1]->AddMod(ops[4]);
	ops[2]->AddMod(ops[3]);
	ops[4]->AddMod(ops[5]);

	fmgen = std::make_shared<FMGen>(sustain_callback);
	fmgen->AddFM(ops[0]);
	fmgen->AddFM(ops[1]);

	for (int i = 0; i < 6; ++i) {
		ops[i]->SetEnv(&env_settings[i]);
	}

	return fmgen;
}

std::shared_ptr<FMGen> FMSynthVoiceCallback::GetFMGen() {

	return fmgen;

}


HRESULT FMSynthVoiceCallback::SubmitBuffer(BYTE* buf, int sample_count) {

	XAUDIO2_BUFFER xaudio_buffer = {};
	xaudio_buffer.Flags = 0;
	xaudio_buffer.AudioBytes = sample_count * 4;
	xaudio_buffer.pAudioData = buf;
	xaudio_buffer.PlayBegin = 0;
	xaudio_buffer.PlayLength = 0;
	xaudio_buffer.LoopBegin = 0;
	xaudio_buffer.LoopLength = 0;
	xaudio_buffer.LoopCount = 0;
	xaudio_buffer.pContext = reinterpret_cast<void*>(buf);

	HRESULT hr;
	hr = source_voice->SubmitSourceBuffer(&xaudio_buffer, nullptr);
		
	return hr;

}

HRESULT FMSynthVoiceCallback::Start() {

	HRESULT hr;
	hr = source_voice->Start(0, XAUDIO2_COMMIT_NOW);
			
	return hr;

}

FMGen::FMGen(std::function<bool(void)> IsDown) : IsDown(IsDown), sampleCounter(0), last_val(0.0001f), hasComeUp(false) {

	return;

}

FMGen::~FMGen() {

	return;

}


void FMGen::AddFM(std::shared_ptr<FreqMod> carrier) {

	fm.push_back(carrier);
	return;

}


BYTE* FMGen::operator()(int sample_count) {

	float* fm_wave = new float[sample_count];
	for (int i = 0; i < sample_count; ++i) {
		sampleCounter++;
		fm_wave[i] = 0.0f;
		for (auto &gen : fm) {
			bool isDown = IsDown();
			float val = gen->operator()(sampleCounter, isDown);
			fm_wave[i] += val * 0.3f;
		}
	}

	last_val = fm_wave[sample_count - 1];
	return reinterpret_cast<BYTE*>(fm_wave);

}

bool FMGen::IsDone() {

	if (last_val == 0.0f) return true;

	return false;

}

void FMGen::Reset() {

	sampleCounter = 0;
	last_val = 0.0001f;

	for (auto &gen : fm) {
		gen->Reset();
	}

	return;

}

FMSynth::EGControls::EGControls(FMSynth* instance, int x, int y, int min, int max, float ratio_level, int ratio_rate) : 
	freq(instance, x, y, min, max, L"FRQ"),
	level1(instance, x + 50, y, min, max, L"L1"),
	level2(instance, x + 100, y, min, max, L"L2"),
	level3(instance, x + 150, y, min, max, L"L3"),
	level4(instance, x + 200, y, min, max, L"L4"),
	rate1(instance, x + 250, y, min, max, L"R1"),
	rate2(instance, x + 300, y, min, max, L"R2"),
	rate3(instance, x + 350, y, min, max, L"R3"),
	rate4(instance, x + 400, y, min, max, L"R4"),
	current_op(1), ratio_level(ratio_level), ratio_rate(ratio_rate) {

	{
		ComboboxDesc desc = { (HINSTANCE)::GetWindowLongPtr(instance->hWnd, GWLP_HINSTANCE), instance->hWnd, x, y + 300, 75, 25 };
		this->cb = instance->AddControl<ComboBox, ComboboxDesc>(desc);
		dynamic_cast<ComboBox*>(this->cb.get())->AddString(L"OP1");
		dynamic_cast<ComboBox*>(this->cb.get())->AddString(L"OP2");
		dynamic_cast<ComboBox*>(this->cb.get())->AddString(L"OP3");
		dynamic_cast<ComboBox*>(this->cb.get())->AddString(L"OP4");
		dynamic_cast<ComboBox*>(this->cb.get())->AddString(L"OP5");
		dynamic_cast<ComboBox*>(this->cb.get())->AddString(L"OP6");

		FMSynth::EGControls* eg_controls = this;
		dynamic_cast<ComboBox*>(this->cb.get())->SetOnSelChange([eg_controls](ComboBox& cb) {

			std::wstring op_string = cb.GetString().c_str();
			int new_op = 0;

			if (op_string.compare(L"OP1") == 0) {
				new_op = 1;
			}
			else if (op_string.compare(L"OP2") == 0) {
				new_op = 2;
			}
			else if (op_string.compare(L"OP3") == 0) {
				new_op = 3;
			}
			else if (op_string.compare(L"OP4") == 0) {
				new_op = 4;
			}
			else if (op_string.compare(L"OP5") == 0) {
				new_op = 5;
			}
			else if (op_string.compare(L"OP6") == 0) {
				new_op = 6;
			}

			eg_controls->SaveOp(eg_controls->current_op - 1);
			eg_controls->LoadOp(new_op - 1);

			eg_controls->current_op = new_op;
			return;
		
		});

	}

	ops.push_back(EGControlsOp());
	ops.push_back(EGControlsOp());
	ops.push_back(EGControlsOp());
	ops.push_back(EGControlsOp());
	ops.push_back(EGControlsOp());
	ops.push_back(EGControlsOp());

	return;

}


void FMSynth::EGControls::LoadOp(int op) {
	
	freq.SetVal(ops[op].freq);

	level1.SetVal(ops[op].level1);
	level2.SetVal(ops[op].level2);
	level3.SetVal(ops[op].level3);
	level4.SetVal(ops[op].level4);

	rate1.SetVal(ops[op].rate1);
	rate2.SetVal(ops[op].rate2);
	rate3.SetVal(ops[op].rate3);
	rate4.SetVal(ops[op].rate4);

	return;

}

void FMSynth::EGControls::SaveOp(int op) {
	
	ops[op].freq = freq.GetVal();

	ops[op].level1 = level1.GetVal();
	ops[op].level2 = level2.GetVal();
	ops[op].level3 = level3.GetVal();
	ops[op].level4 = level4.GetVal();

	ops[op].rate1 = rate1.GetVal();
	ops[op].rate2 = rate2.GetVal();
	ops[op].rate3 = rate3.GetVal();
	ops[op].rate4 = rate4.GetVal();

	return;

}

void FMSynth::EGControls::SaveOp(int op, float freq, EnvSettings* env_settings) {

	ops[op].freq = freq;

	ops[op].level1 = env_settings->level1 / ratio_level;
	ops[op].level2 = env_settings->level2 / ratio_level;
	ops[op].level3 = env_settings->level3 / ratio_level;
	ops[op].level4 = env_settings->level4 / ratio_level;

	ops[op].rate1 = env_settings->rate1 / ratio_rate;
	ops[op].rate2 = env_settings->rate2 / ratio_rate;
	ops[op].rate3 = env_settings->rate3 / ratio_rate;
	ops[op].rate4 = env_settings->rate4 / ratio_rate;

	return;

}

void FMSynth::EGControls::SaveOp() {

	std::wstring op_string = dynamic_cast<ComboBox*>(this->cb.get())->GetString().c_str();
	int op = 0;

	if (op_string.compare(L"OP1") == 0) {
		op = 1;
	}
	else if (op_string.compare(L"OP2") == 0) {
		op = 2;
	}
	else if (op_string.compare(L"OP3") == 0) {
		op = 3;
	}
	else if (op_string.compare(L"OP4") == 0) {
		op = 4;
	}
	else if (op_string.compare(L"OP5") == 0) {
		op = 5;
	}
	else if (op_string.compare(L"OP6") == 0) {
		op = 6;
	}

	op--;

	ops[op].freq = freq.GetVal();

	ops[op].level1 = level1.GetVal();
	ops[op].level2 = level2.GetVal();
	ops[op].level3 = level3.GetVal();
	ops[op].level4 = level4.GetVal();

	ops[op].rate1 = rate1.GetVal();
	ops[op].rate2 = rate2.GetVal();
	ops[op].rate3 = rate3.GetVal();
	ops[op].rate4 = rate4.GetVal();

	return; 

}

void FMSynth::EGControls::LoadOp(int op, float* freq, EnvSettings* env_settings) {

	*freq = ops[op].freq;

	env_settings->level1 = ops[op].level1 * ratio_level;
	env_settings->level2 = ops[op].level2 * ratio_level;
	env_settings->level3 = ops[op].level3 * ratio_level;
	env_settings->level4 = ops[op].level4 * ratio_level;

	env_settings->rate1 = ops[op].rate1 * ratio_rate;
	env_settings->rate2 = ops[op].rate2 * ratio_rate;
	env_settings->rate3 = ops[op].rate3 * ratio_rate;
	env_settings->rate4 = ops[op].rate4 * ratio_rate;

	return;

}


FMSynth::EGControlsUnit::EGControlsUnit(FMSynth* instance, int x, int y, int min, int max, std::wstring label) {

	{
		TrackbarDesc desc = { (HINSTANCE)::GetWindowLongPtr(instance->hWnd, GWLP_HINSTANCE), instance->hWnd, x, y + 35, 200, min, max, true };
		this->trb = instance->AddControl<Trackbar, TrackbarDesc>(desc);
	}
	{
		StaticTextDesc desc = { (HINSTANCE)::GetWindowLongPtr(instance->hWnd, GWLP_HINSTANCE), instance->hWnd, x, y + 250, 25, 25, label };
		this->st = instance->AddControl<StaticText, StaticTextDesc>(desc);
	}
	{
		StaticTextDesc desc = { (HINSTANCE)::GetWindowLongPtr(instance->hWnd, GWLP_HINSTANCE), instance->hWnd, x, y, 25, 25, std::to_wstring(dynamic_cast<Trackbar*>(this->trb.get())->GetPos()) };
		this->val_st = instance->AddControl<StaticText, StaticTextDesc>(desc);
		std::shared_ptr<Control> st = this->val_st;
		dynamic_cast<Trackbar*>(this->trb.get())->SetOnThumbTrack([st](Trackbar& trackbar) {
			dynamic_cast<StaticText*>(st.get())->SetText(std::to_wstring(trackbar.GetPos()));
			return;
		});
	}
	
	return;

}


int FMSynth::EGControlsUnit::GetVal() {

	return dynamic_cast<Trackbar*>(this->trb.get())->GetPos();

}

void FMSynth::EGControlsUnit::SetVal(int val) {

	dynamic_cast<Trackbar*>(this->trb.get())->SetPos(val);
	dynamic_cast<StaticText*>(this->val_st.get())->SetText(std::to_wstring(val));

	return;

}

FMSynth::EGControlsOp::EGControlsOp()
	: level1(0), level2(0), level3(0), level4(0), rate1(0), rate2(0), rate3(0), rate4(0), freq(0) {

		return;

}

FMSynth::EGControlsOpAdjusted::EGControlsOpAdjusted() 
	: level1(0.0f), level2(0.0f), level3(0.0f), level4(0.0f), rate1(0.0f), rate2(0.0f), rate3(0.0f), rate4(0.0f) {

	return;

}
