#pragma once

#include <windows.h>
#include <memory>
#include <vector>
#include "envelope.h"

namespace Oscillator {
	
	struct BufDesc {
		BYTE* buf;
		int len;
	};

	extern BufDesc wavetable;
	void Init();

	BufDesc Sine(int sampleRate, int bPerSample, float freq, int nCycle);
	BufDesc Square(int sampleRate, int bPerSample, float freq, int nCycle);

	BufDesc SineWT(int sampleRate, int bPerSample, float freq, int nCycle);

	BufDesc FreqModulation(int sampleRate, float modIndex, float carrierFreq, float modFreq, int nCycle);

}

class FreqMod {

	public:

		FreqMod(float freq, float sampleRate, float modIndex = 1.0f);
		~FreqMod();

		float operator()(uint64_t sampleCount, bool down);
		void AddFeedback(float feedback);

		void AddMod(std::shared_ptr<FreqMod> mod);
		void SetEnv(std::shared_ptr<Env> env);
		void SetEnv(EnvSettings* env_settings);
		void GetEnv(EnvSettings* env_settings);
		void Reset();
		float GetFreq();
		void SetFreq(float freq);

		std::vector<std::shared_ptr<FreqMod>> mods;
		std::vector<std::shared_ptr<FreqMod>> feedback_mods;

	private:

		float freq;
		float sampleRate;
		float modIndex;

		float stepRate;
		float step;

		float callback;

		uint64_t sampleCount;
		float val;

		std::shared_ptr<Env> env;

};