
#include "oscillator.h"
#include <numbers>

Oscillator::BufDesc Oscillator::wavetable = {};

Oscillator::BufDesc Oscillator::Sine(int sampleRate, int bPerSample, float freq, int nCycle) {

	int CYCLE_LENGTH = sampleRate / freq; 
	int AUDIOBUFFER_SIZEINBYTES = (nCycle * CYCLE_LENGTH * bPerSample) / 8;

	BYTE* buf = new BYTE[AUDIOBUFFER_SIZEINBYTES];

	int iBuf = 0;
	float phase = 0;

	// -- For now we assume that bPerSample is always 32 and that it contains a float
	// -- Otherwise the *(float*) cast is absolute nonsense

	while (iBuf < AUDIOBUFFER_SIZEINBYTES) {

		phase += (2.0f * std::numbers::pi) / CYCLE_LENGTH;
		*(float*)(buf + iBuf) = (float)sin(phase);

		iBuf += bPerSample / 8;

	}

	Oscillator::BufDesc bufDesc = { buf, AUDIOBUFFER_SIZEINBYTES };

	return bufDesc;
}

Oscillator::BufDesc Oscillator::Square(int sampleRate, int bPerSample, float freq, int nCycle) {

	int CYCLE_LENGTH = sampleRate / freq; 
	int AUDIOBUFFER_SIZEINBYTES = (nCycle * CYCLE_LENGTH * bPerSample) / 8;

	int NYQUIST_LIMIT = sampleRate / 2;
	float DUTY_CYCLE = 0.5f;
	int nCOEF = NYQUIST_LIMIT / freq; 	

	float* coef_buf = new float[nCOEF];

	// -- Calculate coefficients

	coef_buf[0] = 2.0f * DUTY_CYCLE - 1.0f;
	for (int i = 1; i < nCOEF; ++i) {
		coef_buf[i] = (4.0f * sin(DUTY_CYCLE * std::numbers::pi * i)) / ( i * std::numbers::pi);
	}

	BYTE* buf = new BYTE[AUDIOBUFFER_SIZEINBYTES];
	int iBuf = 0;
	int i = 0;

	// for (int i = 0; i < (CYCLE_LENGTH * nCycle); ++i) {
	while (iBuf < AUDIOBUFFER_SIZEINBYTES) {
		float theta = (i * freq * 2.0f * std::numbers::pi) / sampleRate;
		*(float*)(buf + iBuf) = 0.0f;
		for (int j = 0; j < nCOEF; ++j) {
			*(float*)(buf + iBuf) += coef_buf[j] * cos(j * theta);
		}
		iBuf += bPerSample / 8;
		++i;
	}

	delete[] coef_buf;

	Oscillator::BufDesc bufDesc = { buf, AUDIOBUFFER_SIZEINBYTES };

	return bufDesc;

}

Oscillator::BufDesc Oscillator::FreqModulation(int sampleRate, float modIndex, float carrierFreq, float modFreq, int nCycle) {

	int CYCLE_LENGTH = sampleRate / carrierFreq;
	int AUDIOBUFFER_SIZEINBYTES = (nCycle * CYCLE_LENGTH * 32) / 8;

	BYTE* buf = new BYTE[AUDIOBUFFER_SIZEINBYTES];

	int iBuf = 0;
	int phaseIndex_carrier = 0;
	int phaseIndex_mod = 0;
	int increment_carrier = static_cast<int>(carrierFreq);
	int increment_mod = static_cast<int>(modFreq);

	float step = 1.0f / sampleRate;
	float curr_step = 0.0f;

	while (iBuf < AUDIOBUFFER_SIZEINBYTES) {

		// float carrier = reinterpret_cast<float*>(Oscillator::wavetable.buf)[phaseIndex_carrier] * 2.0f * std::numbers::pi;
		// float mod = reinterpret_cast<float*>(Oscillator::wavetable.buf)[phaseIndex_mod] * 2.0f * std::numbers::pi;

		float carrier = curr_step * 2.0f * std::numbers::pi * carrierFreq;
		float mod = curr_step * 2.0f * std::numbers::pi * modFreq;

		*(float*)(buf + iBuf) = sin(carrier + modIndex * sin(mod));

		curr_step += step;
		iBuf += 32 / 8;
		phaseIndex_carrier = (phaseIndex_carrier + increment_carrier) % sampleRate;
		phaseIndex_mod = (phaseIndex_mod + increment_mod) % sampleRate;

	}

	Oscillator::BufDesc bufDesc = { buf, AUDIOBUFFER_SIZEINBYTES };
	return bufDesc;

}

void Oscillator::Init() {

	Oscillator::wavetable = Oscillator::Sine(48000, 32, 1.0f, 1);
	return;

}


Oscillator::BufDesc Oscillator::SineWT(int sampleRate, int bPerSample, float freq, int nCycle) {

	int CYCLE_LENGTH = sampleRate / freq;
	int AUDIOBUFFER_SIZEINBYTES = (nCycle * CYCLE_LENGTH * bPerSample) / 8;

	BYTE* buf = new BYTE[AUDIOBUFFER_SIZEINBYTES];

	int iBuf = 0;
	int phaseIndex = 0;
	int increment = static_cast<int>(freq);

	while (iBuf < AUDIOBUFFER_SIZEINBYTES) {

		*(float*)(buf + iBuf) = reinterpret_cast<float*>(Oscillator::wavetable.buf)[phaseIndex];

		iBuf += bPerSample / 8;
		phaseIndex = (phaseIndex + increment) % sampleRate;

	}

	Oscillator::BufDesc bufDesc = { buf, AUDIOBUFFER_SIZEINBYTES };
	return bufDesc;
}

FreqMod::FreqMod(float freq, float sampleRate, float modIndex) : freq(freq), sampleRate(sampleRate), modIndex(modIndex), env(nullptr), sampleCount(0) {

	stepRate = 1.0f / sampleRate;
	step = 0.0f;

}

FreqMod::~FreqMod() {

}

float FreqMod::operator()(uint64_t sampleCount, bool down) {

	float val = 0.0f;
	if (this->sampleCount < sampleCount) {

		float phase = step * 2.0f * std::numbers::pi * freq;
		step += stepRate;
		float ret = 0.0f;
		for (auto &mod : mods) {
			ret += (*mod)(sampleCount, down);
		}
		val = modIndex * sin(phase + ret);
		if (env != nullptr) val = env->operator()(down) * val; 
		this->sampleCount++;
		this->val = val;

	}
	else {
		val = this->val;
	}

	return val;

}

void FreqMod::AddFeedback(float feedback) {

}

void FreqMod::AddMod(std::shared_ptr<FreqMod> mod) {

	mods.push_back(mod);
	return;

}

void FreqMod::SetEnv(std::shared_ptr<Env> env) {

	this->env = env;
	return;

}

void FreqMod::SetEnv(EnvSettings* env_settings) {
	
	this->env = std::make_shared<Env>(env_settings->level1, env_settings->level2, env_settings->level3, env_settings->level4,
		env_settings->rate1, env_settings->rate2, env_settings->rate3, env_settings->rate4);
	return;

}

void FreqMod::GetEnv(EnvSettings* env_settings) {

	if (env != nullptr) env->GetSettings(env_settings);
	return;

}

float FreqMod::GetFreq() {

	return freq;

}

void FreqMod::SetFreq(float freq) {

	this->freq = freq;

	return;

}

void FreqMod::Reset() {

	env->Reset();
	step = 0.0f;
	sampleCount = 0;
	val = 0.0f;

	for (auto mod : mods) {
		mod->Reset();
	}
	
	return;

}
