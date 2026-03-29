#pragma once

#include "threaded_window.h"

class Application2 : public ThreadedWindow {

	public:
		
		Application2(LPCWSTR windowName, int x, int y, int nWidth, int nHeight);
};