#include "app.h"
#include "button.h"

Application::Application(LPCWSTR windowName, int x, int y, int nWidth, int nHeight) : ThreadedWindow(windowName, x, y, nWidth, nHeight) {

	ButtonDesc desc = { (HINSTANCE)::GetWindowLongPtr(this->hWnd, GWLP_HINSTANCE), this->hWnd, 10, 10, 100, 200, L"A" };
	std::shared_ptr<Control> button = AddControlMacro(Button, desc);

	return;

}