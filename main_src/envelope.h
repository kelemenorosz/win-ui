#pragma once

#define ApplyEnvelope(bufDesc, mult, coef) 																						\
	for (float* buf = reinterpret_cast<float*>(bufDesc.buf); reinterpret_cast<BYTE*>(buf) < bufDesc.buf + bufDesc.len; buf++) { \
		*buf = *buf * coef; 																									\
		coef = coef * mult; 																									\
	}

namespace Envelope {

	double FastExpEnvGen(double start, double end, int nSample);

}

struct EnvSettings {

	EnvSettings() = default;
	EnvSettings(float l1, float l2, float l3, float l4, float r1, float r2, float r3, float r4) 
		: level1(l1), level2(l2), level3(l3), level4(l4), rate1(r1), rate2(r2), rate3(r3), rate4(r4) {}

	float level1;
	float level2;
	float level3;
	float level4;

	float rate1;
	float rate2;
	float rate3;
	float rate4;

};

class Env {
	
	public:

		Env() = delete;
		Env(float level1, float level2, float level3, float level4, float rate1, float rate2, float rate3, float rate4);
		~Env();

		float operator()(bool down);
		void Reset();
		void GetSettings(EnvSettings* env_settings);

	private:

		float level1;
		float level2;
		float level3;
		float level4;

		float rate1;
		float rate2;
		float rate3;
		float rate4;

		float rate_change_l41;
		float rate_change_l12;
		float rate_change_l23;
		float rate_change_l34;

		int current_level;
		int current_count;

		float amp;

};