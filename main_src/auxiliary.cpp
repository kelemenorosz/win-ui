#include "auxiliary.h"

double rst::radian_to_degree(double angle) {
	return angle * (180.0 / std::numbers::pi);
}
	
double rst::degree_to_radian(double angle) {
	return angle * (std::numbers::pi / 180.0);
}
