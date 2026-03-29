
#include "static_text.h"
#include <stdexcept>

void StaticText::SetText(std::wstring text) {

	::SendMessage(this->hWnd, WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(text.c_str()));

	return;

}

void StaticText::SetText(float val, int digits) {

	std::wstring str = std::to_wstring(val);
	if (str.size() > (digits + 1)) str.resize(digits + 1);
	::SendMessage(this->hWnd, WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(str.c_str()));

}

LRESULT StaticText::OnCtlColorStatic(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	HDC hdc = reinterpret_cast<HDC>(wParam);
    SetTextColor(hdc, RGB(0x00, 0x00, 0x00));
    SetBkColor(hdc, RGB(0xE9, 0xE9, 0xE9));

	SetDCBrushColor(hdc, RGB(0xE9, 0xE9, 0xE9));
	SetDCPenColor(hdc, RGB(0xE9, 0xE9, 0xE9));
	SelectObject(hdc, GetStockObject(DC_BRUSH));
	SelectObject(hdc, GetStockObject(DC_PEN));
	RoundRect(hdc, 0, 0, this->desc.nWidth, this->desc.nHeight, 0, 0);

    return reinterpret_cast<LRESULT>(this->hbrBkgnd);

}
