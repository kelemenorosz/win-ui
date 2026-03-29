
#include "envelope.h"
#include <cmath>
#include <cstdio>

double Envelope::FastExpEnvGen(double start, double end, int nSample) {

	double mult = 1.0f + ((std::log(end) - std::log(start)) / static_cast<double>(nSample));
	return mult;

}


Env::Env(float level1, float level2, float level3, float level4, float rate1, float rate2, float rate3, float rate4)
	: level1(level1), level2(level2), level3(level3), level4(level4), rate1(rate1), rate2(rate2), rate3(rate3), rate4(rate4) {

	// fmgen_file = fopen("fmgen.txt", "w");

	current_level = 1;
	current_count = 0;

	amp = level4;

	rate_change_l41 = (level1 - level4) / rate1;
	rate_change_l12 = (level1 - level2) / rate2;
	rate_change_l23 = (level2 - level3) / rate3;
	rate_change_l34 = (level3 - level4) / rate4;

	// fprintf(fmgen_file, "L1: %f, L2: %f, L3: %f, L4: %f.\n", level1, level2, level3, level4);
	// fprintf(fmgen_file, "R1: %f, R2: %f, R3: %f, R4: %f.\n", rate1, rate2, rate3, rate4);
	// fprintf(fmgen_file, "S41: %f, S12: %f, S23: %f, S34: %f.\n", rate_change_l41, rate_change_l12, rate_change_l23, rate_change_l34);

	return;

}

Env::~Env() {

	// fclose(fmgen_file);

	return;

}

float Env::operator()(bool down) {

	switch(current_level) {
		case(1):
			amp += rate_change_l41;
			current_count++;
			if (current_count == rate1) {
				current_count = 0;
				current_level = 2;
			}
			break;
		case(2):
			amp -= rate_change_l12;
			current_count++;
			if (current_count == rate2) {
				current_count = 0;
				current_level = 3;
			}
			break;
		case(3):
			if (current_count < rate3) {
				amp -= rate_change_l23;
				current_count++;
			}
			if (current_count == rate3 && down == false) {
				current_count = 0;
				current_level = 4;
			}
			break;
		case(4):
			amp -= rate_change_l34;
			current_count++;
			if (current_count == rate4) {
				current_count = 0;
				current_level = 5;
			}
			break;
		case(5):
			amp = 0.0f;
			break;
		default:
			break;
	}

	// fprintf(fmgen_file, "Amp: %f, Level: %d, Count %d.\n", amp, current_level, current_count);
	return amp;

}

void Env::Reset() {

	current_level = 1;
	current_count = 0;
	amp = level4;
	
	return;

}

void Env::GetSettings(EnvSettings* env_settings) {
	
	env_settings->level1 = level1;
	env_settings->level2 = level2;
	env_settings->level3 = level3;
	env_settings->level4 = level4;

	env_settings->rate1 = rate1;
	env_settings->rate2 = rate2;
	env_settings->rate3 = rate3;
	env_settings->rate4 = rate4;

	return;

}