#pragma once

#include "control.h"
#include <commctrl.h>
#include <functional>

struct TrackbarDesc {

	TrackbarDesc() = delete;
	TrackbarDesc(HINSTANCE hInstance, HWND hWnd) : controlDesc(hInstance, hWnd, 0, 0, 100, 25), nMin(0), nMax(10), style(0) {}
	TrackbarDesc(HINSTANCE hInstance, HWND hWnd, int x, int y, int len, bool vertical = false) 
		: controlDesc(hInstance, hWnd, x, y, vertical ? 25 : len, vertical ? len : 25), nMin(0), nMax(10), style(vertical ? TBS_VERT | TBS_LEFT | TBS_DOWNISLEFT : TBS_HORZ) {}
	TrackbarDesc(HINSTANCE hInstance, HWND hWnd, int x, int y, int len, int nMin, int nMax, bool vertical = false) 
		: controlDesc(hInstance, hWnd, x, y, vertical ? 25 : len, vertical ? len : 25), nMin(nMin), nMax(nMax), style(vertical ? TBS_VERT | TBS_LEFT | TBS_DOWNISLEFT : TBS_HORZ) {}
	~TrackbarDesc() {}

	TrackbarDesc(TrackbarDesc& other) = delete;
	TrackbarDesc& operator=(TrackbarDesc& other) = delete;

	TrackbarDesc(TrackbarDesc&& other) = delete;
	TrackbarDesc& operator=(TrackbarDesc&& other) = delete;

	ControlDesc controlDesc;
	int nMin;
	int nMax;
	DWORD style;

};

class Trackbar : public Control {

	public:

		Trackbar() = delete;
		Trackbar(TrackbarDesc& desc) : Control(desc.controlDesc, TRACKBAR_CLASS, L"", desc.style | WS_CHILD | WS_VISIBLE), onThumbTrack(&Trackbar::_onThumbTrack) {
			::SendMessage(this->hWnd, TBM_SETRANGEMIN, TRUE, desc.nMin);
			::SendMessage(this->hWnd, TBM_SETRANGEMAX, TRUE, desc.nMax);
		}
		~Trackbar() {}

		Trackbar(Trackbar& other) = delete;
		Trackbar& operator=(Trackbar& other) = delete;
		Trackbar(Trackbar&& other) = delete;
		Trackbar& operator=(Trackbar&& other) = delete;

		virtual LRESULT OnNotify(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		virtual LRESULT OnHscroll(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		virtual LRESULT OnVscroll(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		virtual void SetOnThumbTrack(std::function<void(Trackbar&)> func) final;

		int GetPos();
		void SetPos(int pos);
		void SetRange(int nMin, int nMax);

	private:

		std::function<void(Trackbar&)> onThumbTrack;

		virtual void _onThumbTrack();

};