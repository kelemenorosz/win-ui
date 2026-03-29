
#include "button.h"

LRESULT Button::OnCommand(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	WORD code = HIWORD(wParam);
	switch (code) {
		case BN_CLICKED:
			printf("[INFO] BN_CLICKED.\n");
			break;
		default:
			printf("[INFO] OTHER MESSAGE.\n");
			break;
	}

	this->onMouseUp(*this);
	isDown = false;
	printf("IsDown FALSE.\n");
	return S_OK;

}

LRESULT Button::OnNotify(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	NMHDR* notification_msg = reinterpret_cast<NMHDR*>(lParam);

	switch(notification_msg->code) {
		case NM_CUSTOMDRAW:
			return this->OnCustomDraw(windowInstance, uMsg, wParam, lParam);
			break;
		default:
			break;
	}

	return S_OK;

}

LRESULT Button::OnCustomDraw(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	NMCUSTOMDRAW* custom_draw = reinterpret_cast<NMCUSTOMDRAW*>(lParam);
	
	switch(custom_draw->dwDrawStage) {
	
		case CDDS_POSTERASE:
			break;
		case CDDS_POSTPAINT:
			break;
		case CDDS_PREERASE:
			return CDRF_NOTIFYPOSTERASE;
			break;
		case CDDS_PREPAINT:

			if (custom_draw->uItemState & CDIS_SELECTED) {
				if (isDown == false) {
					isDown = true;
					printf("IsDown TRUE.\n");
					this->onMouseDown(*this, custom_draw);
				}
			}
			else if (custom_draw->uItemState & CDIS_HOT) {
				this->onMouseHover(*this, custom_draw);
			}
			else {
				this->onMouseIdle(*this, custom_draw);
			}

			return CDRF_DOERASE;
			break;
		case CDDS_ITEMPREPAINT:
			break;
		default:
			break;
	}

	return S_OK;

}

void Button::_onMouseDown(NMCUSTOMDRAW* customDraw) {

	// printf("[INFO] _onMouseDown.\n");
	return;

}

void Button::_onMouseUp() {

	// printf("[INFO] _onMouseUp.\n");
	return;

}

void Button::_onMouseHover(NMCUSTOMDRAW* customDraw) {

	// printf("[INFO] _onMouseHover.\n");
	return;

}

void Button::_onMouseIdle(NMCUSTOMDRAW* customDraw) {

	// printf("[INFO] _onMouseIdle.\n");
	return;

}

void Button::SetOnMouseDown(std::function<void(Button&, NMCUSTOMDRAW*)> func) {

	onMouseDown = func;
	return;

}

bool Button::IsDown() {

	return this->isDown;

}
