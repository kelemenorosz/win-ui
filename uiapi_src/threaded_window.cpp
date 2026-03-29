
#include "threaded_window.h"
#include <Commctrl.h>
#include <cstdint>
#include <iostream>

HINSTANCE ThreadedWindow::hInstance = NULL;
ThreadedWindow::TW_STATE ThreadedWindow::state = TW_STATE::TW_DEFAULT;
std::queue<std::thread> ThreadedWindow::window_threads = std::queue<std::thread>();
std::mutex ThreadedWindow::window_threads_mutex = std::mutex();
TWHANDLE ThreadedWindow::twHandle_count = 0;
std::mutex ThreadedWindow::twHandle_mutex = std::mutex();
std::unordered_map<TWHANDLE, std::shared_ptr<ThreadedWindow>> ThreadedWindow::twHandle_map = std::unordered_map<TWHANDLE, std::shared_ptr<ThreadedWindow>>();

LRESULT CALLBACK ThreadedWindow::s_WindProc(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	ThreadedWindow* application = nullptr;
	if (uMsg == WM_NCCREATE) {

		LPCREATESTRUCT createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		application = static_cast<ThreadedWindow*>(createStruct->lpCreateParams);
		SetWindowLongPtr(windowInstance, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(application));

	}
	else {

		application = reinterpret_cast<ThreadedWindow*>(GetWindowLongPtr(windowInstance, GWLP_USERDATA));

	}

	if (application == nullptr) return DefWindowProc(windowInstance, uMsg, wParam, lParam);

	return application->WindProc(windowInstance, uMsg, wParam, lParam);
	
}

LRESULT CALLBACK ThreadedWindow::WindProc(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch(uMsg) {

		case WM_CREATE:
			return this->_OnCreate(windowInstance, uMsg, wParam, lParam);
			break;
		case WM_COMMAND:
			return this->_OnCommand(windowInstance, uMsg, wParam, lParam);
			break;
		case WM_NOTIFY:
			return this->_OnNotify(windowInstance, uMsg, wParam, lParam);
			break;
		case WM_PAINT:
			return this->_OnPaint(windowInstance, uMsg, wParam, lParam);
			break;
		case WM_CTLCOLORSTATIC:
			return this->_OnCtlColorStatic(windowInstance, uMsg, wParam, lParam);
			break;
		case WM_HSCROLL:
			return this->_OnHscroll(windowInstance, uMsg, wParam, lParam);
			break;
		case WM_VSCROLL:
			return this->_OnVscroll(windowInstance, uMsg, wParam, lParam);
			break;
		case WM_MOUSEWHEEL:
			return this->_OnMouseWheel(windowInstance, uMsg, wParam, lParam);
			break;
		case WM_MOUSEMOVE:
			return this->_OnMouseMove(windowInstance, uMsg, wParam, lParam);
			break;
		case WM_MENUSELECT:
			return this->_OnMenuSelect(windowInstance, uMsg, wParam, lParam);
			break; 
		case WM_CHAR:
			return this->_OnChar(windowInstance, uMsg, wParam, lParam);
			break;
		case WM_CLOSE:
			::DestroyWindow(windowInstance);
			break;
		case WM_DESTROY:
			printf("[INFO] \"%ws\" | WM_DESTROY.\n", windowName);
			for (std::pair<HWND, std::thread>& wnd_p : this->child_windows) {
				if (::IsWindow(wnd_p.first) != 0) ::PostMessage(wnd_p.first, THWND_DESTROY, NULL, NULL);
				wnd_p.second.join();
			}
			this->wndState = TW_STATE::TW_CLOSED;
			::PostQuitMessage(0);
			break;
		case THWND_DESTROY:
			printf("[INFO] \"%ws\" | THWND_DESTROY.\n", windowName);
			::DestroyWindow(this->hWnd);
			this->wndState = TW_STATE::TW_CLOSED;
			break;
		case THWND_CUSTOMEVENT:
			return this->_OnCustomEvent(windowInstance, uMsg, wParam, lParam);
			break;
		case THWND_CUSTOMEVENT_POST:
			return this->_OnCustomEventPost(windowInstance, uMsg, wParam, lParam);
			break;
		default:
			return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);
	
	}

	return S_OK;

}

ThreadedWindow::ThreadedWindow(LPCWSTR windowName, int x, int y, int nWidth, int nHeight) : windowName(windowName), x(x), y(y), nWidth(nWidth), nHeight(nHeight), wndState(TW_STATE::TW_DEFAULT) {

	if (ThreadedWindow::state == TW_STATE::TW_DEFAULT) throw;
	RECT rect = {x, y, nWidth, nHeight};
	BOOL ret = ::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	this->hWnd = ::CreateWindow(L"ThreadedWindow", this->windowName, WS_OVERLAPPEDWINDOW, this->x, this->y, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, ThreadedWindow::hInstance, this);
	::ShowWindow(this->hWnd, SW_SHOW);
		
	printf("[INFO] \"%ws\" | ThreadedWindow::ThreadedWindow() HWND: %zd.\n", windowName, reinterpret_cast<UINT64>(this->hWnd));

	return;

}

ThreadedWindow::~ThreadedWindow() {

	printf("[INFO] \"%ws\" | ThreadedWindow::~ThreadedWindow().\n", windowName);
	return;

}

void ThreadedWindow::Initialize(HINSTANCE hInstance) {

	// -- Either leave this here and call ThreadedWindow::Initialize() every time the program starts
	// -- Or
	// -- Move it into the constructor and have it run the first time ThreadedWindow::ThreadedWindow() is called

	ThreadedWindow::hInstance = hInstance;

	WNDCLASS window_class = {};
	window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  	window_class.lpfnWndProc = ThreadedWindow::s_WindProc;
  	window_class.cbClsExtra = NULL;
  	window_class.cbWndExtra = NULL;
  	window_class.hInstance = ThreadedWindow::hInstance;
  	window_class.hIcon = NULL;
  	window_class.hCursor = NULL;
  	window_class.hbrBackground = (HBRUSH)COLOR_GRAYTEXT;
  	window_class.lpszMenuName = NULL;
  	window_class.lpszClassName = L"ThreadedWindow";

	::RegisterClass(&window_class);

	ThreadedWindow::state = TW_STATE::TW_READY;

}

void ThreadedWindow::Shutdown() {

	while (window_threads.size() != 0) {

		window_threads.front().join();
		{
			std::lock_guard<std::mutex> lg(window_threads_mutex);
			window_threads.pop();
		}
	}

	for (auto& wnd : twHandle_map) {
		wnd.second.reset();
	}

	return;

}

LRESULT ThreadedWindow::OnPaint(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::OnCommand(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	// printf("[INFO] ThreadedWindow::OnCommand().\n");
	return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::OnMouseWheel(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

}		

LRESULT ThreadedWindow::OnMouseMove(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

}		

LRESULT ThreadedWindow::OnMenuSelect(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	printf("ThreadedWindow::OnMenuSelect().\n");
	return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::OnChar(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	printf("ThreadedWindow::OnChar().\n");
	return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::OnCustomEventInit(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	printf("ThreadedWindow::OnCustomEventInit().\n");
	return S_OK;

}

LRESULT ThreadedWindow::OnCustomEventOther(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	printf("ThreadedWindow::OnCustomEventOther().\n");
	return S_OK;

}

LRESULT ThreadedWindow::_OnPaint(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	return OnPaint(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::_OnMouseWheel(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	return OnMouseWheel(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::_OnMouseMove(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	return OnMouseMove(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::_OnMenuSelect(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	return OnMenuSelect(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::_OnChar(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	return OnChar(windowInstance, uMsg, wParam, lParam);
}

LRESULT ThreadedWindow::_OnCustomEventInit(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	return OnCustomEventInit(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::_OnCustomEventOther(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	return OnCustomEventOther(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::_OnCreate(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::_OnCommand(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	printf("[INFO] ThreadedWindow::_OnCommand().\n");

	// -- If lParam is != 0 then the message came from a control child

	if (lParam != 0) {

		auto control_iter = control_map.find((HWND)lParam);
		if (control_map.end() != control_iter) { 
			return control_iter->second->OnCommand(windowInstance, uMsg, wParam, lParam);
		}
		return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

	}
	else if (HIWORD(wParam) == 0) {
		return this->OnCommand(windowInstance, uMsg, wParam, lParam);
	}
	else {
		return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);
	}

}

LRESULT ThreadedWindow::_OnNotify(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	NMHDR* notification_msg = reinterpret_cast<NMHDR*>(lParam);
	auto control_iter = control_map.find(notification_msg->hwndFrom);

	if (control_map.end() != control_iter) {
		return control_iter->second->OnNotify(windowInstance, uMsg, wParam, lParam);
	}

	return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::_OnCtlColorStatic(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	// -- Device context and control handles

	HDC hdc = reinterpret_cast<HDC>(wParam);
	HWND hWnd = reinterpret_cast<HWND>(lParam);

	auto control_iter = this->control_map.find(hWnd);
	if (control_map.end() != control_iter) {
		return control_iter->second->OnCtlColorStatic(windowInstance, uMsg, wParam, lParam);
	}

	return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::_OnHscroll(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	HWND hWnd = reinterpret_cast<HWND>(lParam);
	
	auto control_iter = this->control_map.find(hWnd);
	if (control_map.end() != control_iter) {
		return control_iter->second->OnHscroll(windowInstance, uMsg, wParam, lParam);
	}

	return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::_OnVscroll(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	HWND hWnd = reinterpret_cast<HWND>(lParam);
	
	auto control_iter = this->control_map.find(hWnd);
	if (control_map.end() != control_iter) {
		return control_iter->second->OnVscroll(windowInstance, uMsg, wParam, lParam);
	}

	return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

}

LRESULT ThreadedWindow::_OnCustomEvent(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	ThWnd_CustomEvent* e = reinterpret_cast<ThWnd_CustomEvent*>(lParam);

	switch (e->type) {
		case ThWnd_CustomEvent_Type::THWND_CUSTOMEVENT_TYPE_INIT:
			return this->_OnCustomEventInit(windowInstance, uMsg, wParam, lParam);
			break;
		case ThWnd_CustomEvent_Type::THWND_CUSTOMEVENT_TYPE_OTHER:
			return this->_OnCustomEventOther(windowInstance, uMsg, wParam, reinterpret_cast<LPARAM>(e->data));
			break;
		default:
			break;
	}

	return S_OK;

}

LRESULT ThreadedWindow::_OnCustomEventPost(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	ThWnd_CustomEvent* e = reinterpret_cast<ThWnd_CustomEvent*>(lParam);
	void* data = e->data;
	delete e;

	switch (e->type) {
		case ThWnd_CustomEvent_Type::THWND_CUSTOMEVENT_TYPE_INIT:
			return this->_OnCustomEventInit(windowInstance, uMsg, wParam, lParam);
			break;
		case ThWnd_CustomEvent_Type::THWND_CUSTOMEVENT_TYPE_OTHER:
			return this->_OnCustomEventOther(windowInstance, uMsg, wParam, reinterpret_cast<LPARAM>(data));
			break;
		default:
			break;
	}

	return S_OK;

}


LRESULT ThreadedWindow::InitWnd(HWND hWnd) {

	ThWnd_CustomEvent e;
	e.type = ThWnd_CustomEvent_Type::THWND_CUSTOMEVENT_TYPE_INIT;
	e.data = nullptr;
	return SendMessage(hWnd, THWND_CUSTOMEVENT, NULL, reinterpret_cast<LPARAM>(&e));

}

LRESULT ThreadedWindow::CustomEventOtherWnd(HWND hWnd, void* data) {

	ThWnd_CustomEvent e;
	e.type = ThWnd_CustomEvent_Type::THWND_CUSTOMEVENT_TYPE_OTHER;
	e.data = data;
	return SendMessage(hWnd, THWND_CUSTOMEVENT, NULL, reinterpret_cast<LPARAM>(&e));

}

LRESULT ThreadedWindow::CustomEventOtherWnd_Post(HWND hWnd, void* data) {

	ThWnd_CustomEvent* e = new ThWnd_CustomEvent();
	e->type = ThWnd_CustomEvent_Type::THWND_CUSTOMEVENT_TYPE_OTHER;
	e->data = data;
	return PostMessage(hWnd, THWND_CUSTOMEVENT, NULL, reinterpret_cast<LPARAM>(e));

}

bool ThreadedWindow::IsClosed() {
	if (this->wndState == TW_STATE::TW_CLOSED) return true;
	return false;
}

std::expected<std::shared_ptr<ThreadedWindow>, THWND_ERROR> ThreadedWindow::GetWndObject(TWHANDLE handle) {

	if (!twHandle_map.contains(handle)){
		return std::unexpected(THWND_ERROR::WND_NOT_FOUND);
	} 
	if (twHandle_map.at(handle)->IsClosed()) {
		return std::unexpected(THWND_ERROR::WND_CLOSED);
	}
	
	return twHandle_map.at(handle);
	
}

LRESULT ThreadedWindow::_OnQuit() {

	// -- Add common code here

	// printf("[INFO] ThreadedWindow::_OnQuit().\n");

	return this->OnQuit();

}


LRESULT ThreadedWindow::OnQuit() {

	// printf("[INFO] ThreadedWindow::OnQuit().\n");

	return S_OK;

}
