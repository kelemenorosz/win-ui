#pragma once

#include "threaded_window.h"
#include "app_2.h"

class Application : public ThreadedWindow {

	public:
		
		Application(LPCWSTR windowName, int x, int y, int nWidth, int nHeight);
};