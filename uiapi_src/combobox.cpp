
#include "combobox.h"

void ComboBox::AddString(std::wstring string) {

	::SendMessage(this->hWnd, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(string.c_str()));

	if (this->isEmpty == TRUE) {
		::SendMessage(this->hWnd, CB_SETCURSEL, 0, NULL);
		this->isEmpty = FALSE;
	}

	return;

}

std::wstring ComboBox::GetString() {

	LRESULT res;
	res = ::SendMessage(this->hWnd, CB_GETCURSEL, 0, 0);
	if (res == CB_ERR) {
		return std::wstring();
	}
	int iString = static_cast<int>(res);

	res = ::SendMessage(this->hWnd, CB_GETLBTEXTLEN, iString, NULL);
	if (res == CB_ERR) {
		return std::wstring();
	}
	int lString = static_cast<int>(res);
	wchar_t* buf = new wchar_t[lString + 1]; 

	res = ::SendMessage(this->hWnd, CB_GETLBTEXT, iString, reinterpret_cast<LPARAM>(&buf[0]));
	if (res == CB_ERR) {
		return std::wstring();
	}

	std::wstring string = std::wstring(buf);
	delete[] buf;

	return string;

}


LRESULT ComboBox::OnCommand(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	WORD notification_code = HIWORD(wParam);

	switch(notification_code) {

		case CBN_SELCHANGE:
			this->onSelChange(*this);
			break;
		default:
			break;

	}

	return S_OK;

}

void ComboBox::_onSelChange() {

	return;

}

void ComboBox::SetOnSelChange(std::function<void(ComboBox&)> func) {

	onSelChange = func;
	return;

}