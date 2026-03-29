
#include "trackbar.h"
#include <cstdio>

LRESULT Trackbar::OnNotify(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	NMHDR* notification_msg = reinterpret_cast<NMHDR*>(lParam);

	switch(notification_msg->code) {
		case NM_CUSTOMDRAW:
			break;
		case TRBN_THUMBPOSCHANGING:
			break;
		default:
			break;
	}

	return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

}

LRESULT Trackbar::OnHscroll(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	WORD notification_code = LOWORD(static_cast<DWORD>(wParam));

	switch(notification_code) {
		case TB_BOTTOM:
			printf("[INFO] TB_BOTTOM.\n");
			break;
		case TB_THUMBTRACK:
			this->onThumbTrack(*this);
			break;
		default:
			break;

	}

	return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

}

LRESULT Trackbar::OnVscroll(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	WORD notification_code = LOWORD(static_cast<DWORD>(wParam));

	switch(notification_code) {
		case TB_BOTTOM:
			printf("[INFO] TB_BOTTOM.\n");
			break;
		case TB_THUMBTRACK:
			this->onThumbTrack(*this);
			break;
		case TB_PAGEDOWN:
			this->onThumbTrack(*this);
			break;
		case TB_PAGEUP:
			this->onThumbTrack(*this);
			break;
		case TB_THUMBPOSITION:
			this->onThumbTrack(*this);
			break;
		case TB_LINEDOWN:
			printf("[INFO] TB_LINEDOWN.\n");
			break;
		case TB_LINEUP:
			printf("[INFO] TB_LINEUP.\n");
			break;
		case TB_ENDTRACK:
			printf("[INFO] TB_ENDTRACK.\n");
			break;
		default:
			break;

	}

	return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

}

void Trackbar::_onThumbTrack() {

	return;

}

void Trackbar::SetOnThumbTrack(std::function<void(Trackbar&)> func) {

	onThumbTrack = func;
	return;

}

int Trackbar::GetPos() {

	int pos = ::SendMessage(this->hWnd, TBM_GETPOS, 0, 0);

	return pos;

}

void Trackbar::SetPos(int pos) {

	::SendMessage(this->hWnd, TBM_SETPOS, TRUE, pos);
	return;

}

void Trackbar::SetRange(int nMin, int nMax) {

	::SendMessage(this->hWnd, TBM_SETRANGEMIN, TRUE, nMin);
	::SendMessage(this->hWnd, TBM_SETRANGEMAX, TRUE, nMax);
		
	return;

}
