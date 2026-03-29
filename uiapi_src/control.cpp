
#include "control.h"
#include <stdexcept>

template <>
std::wstring Control::GetText<std::wstring>(bool& err) {

	err = false;
	LRESULT res;
	
	res = ::SendMessage(this->hWnd, WM_GETTEXTLENGTH, 0, 0);
	if (res == 0) {
		return std::wstring(L"");
	}

	int lString = static_cast<int>(res);
	wchar_t* buf = new wchar_t[lString + 1]; 

	res = ::SendMessage(this->hWnd, WM_GETTEXT, lString + 1, reinterpret_cast<LPARAM>(&buf[0]));
	if (res == 0) {
		return std::wstring(L"");
	}

	std::wstring ret = std::wstring(buf);
	delete[] buf;
	return ret;

}

template <>
float Control::GetText<float>(bool& err) {

	LRESULT res;
	err = true;

	res = ::SendMessage(this->hWnd, WM_GETTEXTLENGTH, 0, 0);
	if (res == 0) {
		return 0.0f;
	}

	int lString = static_cast<int>(res);
	wchar_t* buf = new wchar_t[lString + 1]; 

	res = ::SendMessage(this->hWnd, WM_GETTEXT, lString + 1, reinterpret_cast<LPARAM>(&buf[0]));
	if (res == 0) {
		return 0.0f;
	}

	std::wstring str = std::wstring(buf);
	delete[] buf;

	float ret;
	try {
		ret = std::stof(str, nullptr);
	}
	catch(std::invalid_argument const& e) {
		return 0.0f;
	}

	err = false;
	return ret;

}