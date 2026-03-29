#pragma once

#include "control.h"
#include <commctrl.h>
#include <string>

struct StaticTextDesc {

	StaticTextDesc() = default;
	StaticTextDesc(HINSTANCE hInstance, HWND hWnd) : controlDesc(hInstance, hWnd) {}
	StaticTextDesc(HINSTANCE hInstance, HWND hWnd, int x, int y, int nWidth, int nHeight) : controlDesc(hInstance, hWnd, x, y, nWidth, nHeight) {}
	StaticTextDesc(HINSTANCE hInstance, HWND hWnd, int x, int y, int nWidth, int nHeight, std::wstring id) : controlDesc(hInstance, hWnd, x, y, nWidth, nHeight), id(id) {}
	~StaticTextDesc() {}

	StaticTextDesc(StaticTextDesc& other) = delete;
	StaticTextDesc& operator=(StaticTextDesc& other) = delete;

	StaticTextDesc(StaticTextDesc&& other) = delete;
	StaticTextDesc& operator=(StaticTextDesc&& other) = delete;

	ControlDesc controlDesc;
	std::wstring id;

};

class StaticText : public Control { 

	public:

		StaticText() = delete;
		StaticText(StaticTextDesc& desc) : Control(desc.controlDesc, WC_STATIC, desc.id.c_str(), WS_VISIBLE | WS_CHILD | SS_LEFT), hbrBkgnd(CreateSolidBrush(RGB(0xE9, 0xE9, 0xE9))) {}
		~StaticText() {
			DeleteObject(hbrBkgnd);
		}

		StaticText(StaticText& other) = delete;
		StaticText& operator=(StaticText& other) = delete;
		StaticText(StaticText&& other) = delete;
		StaticText& operator=(StaticText&& other) = delete;

		void SetText(std::wstring text);
		void SetText(float val, int digits);

		virtual LRESULT OnCtlColorStatic(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		HBRUSH hbrBkgnd;

	private:

};
