#include "app_2.h"
#include "app.h"

Application2::Application2(LPCWSTR windowName, int x, int y, int nWidth, int nHeight) : ThreadedWindow(windowName, x, y, nWidth, nHeight) {
	// ::StartWnd<Application>(L"Application", 0, 0, 640, 640);
}
