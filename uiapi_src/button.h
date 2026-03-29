#pragma once

#include "control.h"
#include <commctrl.h>
#include <string>
#include <functional>

struct ButtonDesc {

	ButtonDesc() = default;
	ButtonDesc(HINSTANCE hInstance, HWND hWnd) : controlDesc(hInstance, hWnd) {}
	ButtonDesc(HINSTANCE hInstance, HWND hWnd, int x, int y, int nWidth, int nHeight) : controlDesc(hInstance, hWnd, x, y, nWidth, nHeight) {}
	ButtonDesc(HINSTANCE hInstance, HWND hWnd, int x, int y, int nWidth, int nHeight, std::wstring id) : controlDesc(hInstance, hWnd, x, y, nWidth, nHeight), id(id) {}
	~ButtonDesc() {}

	ButtonDesc(ButtonDesc& other) = delete;
	ButtonDesc& operator=(ButtonDesc& other) = delete;

	ButtonDesc(ButtonDesc&& other) = delete;
	ButtonDesc& operator=(ButtonDesc&& other) = delete;

	ControlDesc controlDesc;
	std::wstring id;

};

class Button : public Control {

	public:

		Button() = delete;
		Button(ButtonDesc& desc) : Control(desc.controlDesc, WC_BUTTON, desc.id.c_str(), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON),
		 onMouseDown(&Button::_onMouseDown), onMouseUp(&Button::_onMouseUp), onMouseHover(&Button::_onMouseHover), onMouseIdle(&Button::_onMouseIdle), isDown(false) {}
		~Button() {}

		Button(Button& other) = delete;
		Button& operator=(Button& other) = delete;
		Button(Button&& other) = delete;
		Button& operator=(Button&& other) = delete;

		virtual LRESULT OnCommand(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		virtual LRESULT OnNotify(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		virtual void SetOnMouseDown(std::function<void(Button&, NMCUSTOMDRAW*)> func) final;

		virtual bool IsDown() final;

	private:

		virtual LRESULT OnCustomDraw(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam);

		std::function<void(Button&, NMCUSTOMDRAW*)> onMouseDown;
		std::function<void(Button&)> onMouseUp;
		std::function<void(Button&, NMCUSTOMDRAW*)> onMouseHover;
		std::function<void(Button&, NMCUSTOMDRAW*)> onMouseIdle;

		virtual void _onMouseDown(NMCUSTOMDRAW* customDraw);
		virtual void _onMouseUp();
		virtual void _onMouseHover(NMCUSTOMDRAW* customDraw);
		virtual void _onMouseIdle(NMCUSTOMDRAW* customDraw);

		bool isDown;

};