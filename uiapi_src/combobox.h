#pragma once

#include "control.h"
#include <string>
#include <commctrl.h>
#include <functional>

struct ComboboxDesc {

	ComboboxDesc() = default;
	ComboboxDesc(HINSTANCE hInstance, HWND hWnd) : controlDesc(hInstance, hWnd) {}
	ComboboxDesc(HINSTANCE hInstance, HWND hWnd, int x, int y, int nWidth, int nHeight) : controlDesc(hInstance, hWnd, x, y, nWidth, nHeight) {}
	~ComboboxDesc() {}

	ComboboxDesc(ComboboxDesc& other) {
		memcpy(this, &other, sizeof(ComboboxDesc));
	}
	ComboboxDesc& operator=(ComboboxDesc& other) = delete;

	ComboboxDesc(ComboboxDesc&& other) = delete;
	ComboboxDesc& operator=(ComboboxDesc&& other) = delete;

	ControlDesc controlDesc;

};

class ComboBox : public Control {

	public:

		ComboBox() = delete;
		ComboBox(ComboboxDesc& desc) : Control(desc.controlDesc, WC_COMBOBOX, L"", CBS_DROPDOWNLIST | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE), isEmpty(TRUE), onSelChange(&ComboBox::_onSelChange) {}
		~ComboBox() {}

		ComboBox(ComboBox& other) = delete;
		ComboBox& operator=(ComboBox& other) = delete;
		ComboBox(ComboBox&& other) = delete;
		ComboBox& operator=(ComboBox&& other) = delete;

		void AddString(std::wstring string);
		std::wstring GetString();

		virtual LRESULT OnCommand(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		virtual void SetOnSelChange(std::function<void(ComboBox&)> func) final;

	private:

		BOOL isEmpty;
		std::function<void(ComboBox&)> onSelChange;

		virtual void _onSelChange();

};