#pragma once

#include <windows.h>
#include <string>

struct ControlDesc {

	ControlDesc() = default;
	ControlDesc(HINSTANCE hInstance, HWND hWnd) : hInstance(hInstance), hWnd(hWnd), x(0), y(0), nWidth(0), nHeight(0) {}
	ControlDesc(HINSTANCE hInstance, HWND hWnd, int x, int y, int nWidth, int nHeight) : hInstance(hInstance), hWnd(hWnd), x(x), y(y), nWidth(nWidth), nHeight(nHeight) {}
	~ControlDesc() {}

	ControlDesc(ControlDesc& other) {
		this->hInstance = other.hInstance;
		this->hWnd = other.hWnd;
		this->x = other.x;
		this->y = other.y;
		this->nWidth = other.nWidth;
		this->nHeight = other.nHeight;
	}
	ControlDesc& operator=(ControlDesc& other) = delete;
	
	ControlDesc(ControlDesc&& other) = delete;
	ControlDesc& operator=(ControlDesc&& other) = delete;

	HINSTANCE hInstance;
	HWND hWnd;
	int x;
	int y;
	int nWidth;
	int nHeight;

};

class Control {

	public:

		Control() = delete;
		Control(ControlDesc desc, LPCWSTR clss, LPCWSTR name, DWORD style) : desc(desc) {

			this->hWnd = ::CreateWindow(clss, name, style, desc.x, desc.y, desc.nWidth, desc.nHeight, desc.hWnd, NULL, desc.hInstance, NULL);
			return;

		}
		virtual ~Control() {}

		virtual LRESULT OnCommand(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {
			return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);
		}
		virtual LRESULT OnNotify(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {
			return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);
		}
		virtual LRESULT OnCtlColorStatic(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {
			return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);
		}
		virtual LRESULT OnHscroll(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {
			return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);
		}
		virtual LRESULT OnVscroll(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {
			return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);
		}


		HWND GetHwnd() { return this->hWnd; }

		template<class T> T GetText(bool& err);

		template <>	std::wstring GetText<std::wstring>(bool& err);
		template <>	float GetText<float>(bool& err);


	protected:

		ControlDesc desc;
		HWND hWnd;

	private:

};

template<class T>
T Control::GetText(bool& err) {
	err = true;
	T t;
	return t;
}