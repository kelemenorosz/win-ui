#pragma once

#include "threaded_window.h"
#include "button.h"
#include "oscillator.h"
#include <xaudio2.h>
#include <wrl.h>
#include <array>
#include "fm_synth_win_resource.h"

class FMGen {

	public:

		FMGen(std::function<bool(void)> IsDown);
		~FMGen();

		void AddFM(std::shared_ptr<FreqMod> carrier);

		BYTE* operator()(int sample_count);
		bool IsDone();
		void Reset();

	private:

		std::vector<std::shared_ptr<FreqMod>> fm;
		uint64_t sampleCounter;
		float last_val;
		std::function<bool(void)> IsDown; 
		bool hasComeUp;

};
	
enum class FMSynth_CustomEvent_Other_Type : uint8_t {

	FMSYNTH_CUSTOMEVENT_OTHER_TYPE_MIDI_DATA = 0,

};

struct FMSynth_CustomEvent_Other {

	FMSynth_CustomEvent_Other() = default;
	~FMSynth_CustomEvent_Other() {}

	FMSynth_CustomEvent_Other(const FMSynth_CustomEvent_Other& other) = delete;
	FMSynth_CustomEvent_Other& operator=(const FMSynth_CustomEvent_Other& other) = delete;

	FMSynth_CustomEvent_Other_Type type;
	void* data;

};

class FMSynthVoiceCallback : public IXAudio2VoiceCallback {

	public:

		FMSynthVoiceCallback(Microsoft::WRL::ComPtr<IXAudio2> audio_engine, std::function<bool(void)> sustain_callback) 
			: audio_engine(audio_engine), ft(std::future<BYTE*>()), sustain_callback(sustain_callback) {}

		void OnBufferEnd(void* pBufferContext);
		void OnBufferStart(void* pBufferContext);
		void OnLoopEnd(void* pBufferContext) {}
		void OnStreamEnd() {}
		void OnVoiceError(void* pBufferContext, HRESULT Error) {}
		void OnVoiceProcessingPassEnd() {}
		void OnVoiceProcessingPassStart(UINT32 BytesRequired) {}

		HRESULT CreateSource();
		std::shared_ptr<FMGen> CreateFMGen(float freq);
		std::shared_ptr<FMGen> CreateFMGen(float freq, float* freq_settings, EnvSettings* env_settings);
		std::shared_ptr<FMGen> GetFMGen();

		HRESULT SubmitBuffer(BYTE* buf, int sample_count);
		HRESULT Start();

	private:

		Microsoft::WRL::ComPtr<IXAudio2> audio_engine;
		IXAudio2SourceVoice* source_voice;

		std::function<bool(void)> sustain_callback; 
		std::shared_ptr<FMGen> fmgen;
		std::vector<std::shared_ptr<FreqMod>> ops;
		std::future<BYTE*> ft;
		int index;

};

class FMSynth : public ThreadedWindow {

	public:

		FMSynth() = delete;
		FMSynth(LPCWSTR windowName, int x, int y, int nWidth, int nHeight);
		virtual ~FMSynth();

	private:

		Microsoft::WRL::ComPtr<IXAudio2> xaudio_engine;
		IXAudio2MasteringVoice* mastering_voice;
		std::shared_ptr<FMGen> fmgen;
		std::vector<std::shared_ptr<FreqMod>> ops;
		std::vector<std::unique_ptr<FMSynthVoiceCallback>> voice_callback_vec;
		std::array<int, 200> key_velocity_arr;
		
		HMENU menu;
		HMENU midi_menu;
		int midi_menu_n;
		int midi_sub_i;
		UINT64 midi_sub_token;
		bool menu_open;

		HMIDIIN midi_handle;
		
		LRESULT OnMidiKeyPress(UINT8 key, UINT8 velocity);
		LRESULT OnMidiInterfaceReload();

		virtual LRESULT OnCustomEventOther(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		virtual LRESULT OnCommand(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		virtual LRESULT OnMenuSelect(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		struct EGControlsUnit {

			EGControlsUnit() = delete;
			EGControlsUnit(FMSynth* instance, int x, int y, int min, int max, std::wstring label);

			int GetVal();
			void SetVal(int val);

			std::shared_ptr<Control> trb;
			std::shared_ptr<Control> st;
			std::shared_ptr<Control> val_st;

		};

		struct EGControlsOp {

			EGControlsOp();
			
			int freq;

			int level1;
			int level2;
			int level3;
			int level4;

			int rate1;
			int rate2;
			int rate3;
			int rate4;


		};

		struct EGControlsOpAdjusted {

			EGControlsOpAdjusted();

			float level1;
			float level2;
			float level3;
			float level4;

			float rate1;
			float rate2;
			float rate3;
			float rate4;

		};

		struct EGControls {

			EGControls(FMSynth* instance, int x, int y, int min, int max, float ratio_level, int ratio_rate);

			void SaveOp(int op, float freq, EnvSettings* env_settings);
			void SaveOp(int op);
			void SaveOp();
			void LoadOp(int op);

			void LoadOp(int op, float* freq, EnvSettings* env_settings);

			EGControlsUnit freq;

			EGControlsUnit level1;
			EGControlsUnit level2;
			EGControlsUnit level3;
			EGControlsUnit level4;

			EGControlsUnit rate1;
			EGControlsUnit rate2;
			EGControlsUnit rate3;
			EGControlsUnit rate4;

			std::vector<EGControlsOp> ops;

			std::shared_ptr<Control> cb;

			int current_op;
			float ratio_level;
			int ratio_rate;

		};

		EGControls control_board;

};