#pragma once

#include "control.h"
#include "main.h"
#include <windows.h>
#include <unordered_map>
#include <memory>
#include <future>
#include <queue>
#include <expected>

#define AddControlMacro(clss, desc) AddControl<clss, clss##Desc>(desc)

#define THWND_DESTROY (WM_APP + 1) 
#define THWND_CUSTOMEVENT (WM_APP + 2) 
#define THWND_CUSTOMEVENT_POST (WM_APP + 3) 

typedef UINT64 TWHANDLE;
	
enum class ThWnd_CustomEvent_Type : uint8_t {

	THWND_CUSTOMEVENT_TYPE_INIT = 0,
	THWND_CUSTOMEVENT_TYPE_OTHER,

};

enum class THWND_ERROR : UINT8 {

	WND_CLOSED = 0,
	WND_NOT_FOUND,
	OTHER

};

struct ThWnd_Dim {

	ThWnd_Dim() = delete;
	ThWnd_Dim(int nWidth, int nHeight) : nWidth(nWidth), nHeight(nHeight) {}

	int nWidth;
	int nHeight;

};

struct ThWnd_CustomEvent {

	ThWnd_CustomEvent() = default;
	~ThWnd_CustomEvent() {}

	ThWnd_CustomEvent(const ThWnd_CustomEvent& other) = delete;
	ThWnd_CustomEvent& operator=(const ThWnd_CustomEvent& other) = delete;

	ThWnd_CustomEvent_Type type;
	void* data;

};

class ThreadedWindow {

	public:

		ThreadedWindow() = delete;
		ThreadedWindow(LPCWSTR windowName, int x, int y, int nWidth, int nHeight);
		virtual ~ThreadedWindow();

		ThreadedWindow(ThreadedWindow& other) = delete;
		ThreadedWindow& operator=(ThreadedWindow& other) = delete;
		
		ThreadedWindow(ThreadedWindow&& other) = delete;
		ThreadedWindow& operator=(ThreadedWindow&& other) = delete;

		static LRESULT CALLBACK s_WindProc(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static void Initialize(HINSTANCE hInstance);
		static void Shutdown();

		static LRESULT InitWnd(HWND hWnd); 
		static LRESULT CustomEventOtherWnd(HWND hWnd, void* data);
		static LRESULT CustomEventOtherWnd_Post(HWND hWnd, void* data);

		virtual LRESULT _OnQuit() final;

		HWND GetHwnd() {
			return hWnd;
		}

	protected:

		HWND hWnd;
		int nWidth;
		int nHeight;


		template<class _Ctrl, class _Dsc>
		std::shared_ptr<Control> AddControl(_Dsc& desc);

		template<class _Wnd>
		TWHANDLE StartWnd(LPCWSTR windowName, int x, int y, int nWidth, int nHeight);

		bool IsClosed();
		static std::expected<std::shared_ptr<ThreadedWindow>, THWND_ERROR> GetWndObject(TWHANDLE handle);

		ThWnd_Dim GetDim() {
			return ThWnd_Dim(nWidth, nHeight);
		}
		
		LPCWSTR windowName;

	private:

		// -- Message loop. Not overrideable. All ThreadedWindow subclasses use this callback
		virtual LRESULT CALLBACK WindProc(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;
		
		// -- Overrideable message events

		virtual LRESULT OnPaint(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnCommand(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnMouseWheel(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnMouseMove(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnMenuSelect(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnChar(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnCustomEventInit(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnCustomEventOther(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
		// -- Internal implementaion
		// -- Could be expanded to include common functionality where needed

		virtual LRESULT _OnPaint(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;
		virtual LRESULT _OnCommand(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;
		virtual LRESULT _OnMouseWheel(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;
		virtual LRESULT _OnMouseMove(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;
		virtual LRESULT _OnMenuSelect(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;
		virtual LRESULT _OnChar(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;
		virtual LRESULT _OnCustomEventInit(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;
		virtual LRESULT _OnCustomEventOther(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;

		// -- Common message events handled by the base class itself
		// -- Cannot be handled by any subclass
		// -- In this case this is for WM_COMMAND and WM_NOTIFY as ThreadedWindow takes care of control management
		// -- Added WM_CTLCOLORSTATIC; etc.

		virtual LRESULT _OnCreate(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;
		virtual LRESULT _OnNotify(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;
		virtual LRESULT _OnCtlColorStatic(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;
		virtual LRESULT _OnHscroll(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;
		virtual LRESULT _OnVscroll(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;
		virtual LRESULT _OnCustomEvent(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;
		virtual LRESULT _OnCustomEventPost(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) final;

		// -- Called after WM_QUIT is retreived from the message queue with GetMessage()
		// -- First calls _OnQuit()

		virtual LRESULT OnQuit();

		enum class TW_STATE : uint8_t {

			TW_DEFAULT = 0,
			TW_READY,
			TW_CLOSED,

		};

		static HINSTANCE hInstance;
		static TW_STATE state;
		TW_STATE wndState;

		int x;
		int y;

		std::unordered_map<HWND, std::shared_ptr<Control>> control_map;
		std::vector<std::pair<HWND, std::thread>> child_windows;

		template<class _Wnd>
		friend TWHANDLE StartWnd(LPCWSTR windowName, int x, int y, int nWidth, int nHeight);
		static std::queue<std::thread> window_threads;
		static std::mutex window_threads_mutex;
		
		static TWHANDLE twHandle_count;
		static std::mutex twHandle_mutex;
		static std::unordered_map<TWHANDLE, std::shared_ptr<ThreadedWindow>> twHandle_map;

};

template<class _Ctrl, class _Dsc>
std::shared_ptr<Control> ThreadedWindow::AddControl(_Dsc& desc) {

	std::shared_ptr<Control> ctrl = std::make_shared<_Ctrl>(desc);
	std::pair<HWND, std::shared_ptr<Control>> pair(ctrl->GetHwnd(), ctrl);
	this->control_map.insert(std::move(pair));
	return ctrl;

}

template <class _Wnd>
void WndThread(LPCWSTR windowName, int x, int y, int nWidth, int nHeight, std::promise<std::pair<HWND, std::shared_ptr<ThreadedWindow>>> prms) {

	std::shared_ptr<ThreadedWindow> pWnd = std::make_shared<_Wnd>(windowName, x, y, nWidth, nHeight);
	prms.set_value(std::make_pair(pWnd->GetHwnd(), pWnd));

	MSG msg = {};

	while (GetMessage(&msg, NULL, 0, 0) != 0) {

		TranslateMessage(&msg);
		DispatchMessage(&msg);
		
	}

	if (msg.message == WM_QUIT) {
		printf("[INFO] \"%ws\" | WM_QUIT.\n", windowName);
	}

	HRESULT quit_res = pWnd->_OnQuit();

	return;

}

template<class _Wnd>
TWHANDLE StartWnd(LPCWSTR windowName, int x, int y, int nWidth, int nHeight) {

	std::promise<std::pair<HWND, std::shared_ptr<ThreadedWindow>>> prms;
	std::future<std::pair<HWND, std::shared_ptr<ThreadedWindow>>> ft = prms.get_future();
	
	{
		std::lock_guard<std::mutex> lg(ThreadedWindow::window_threads_mutex);
		ThreadedWindow::window_threads.emplace(static_cast<void(*)(LPCWSTR, int, int, int, int, std::promise<std::pair<HWND, std::shared_ptr<ThreadedWindow>>>)>(WndThread<_Wnd>),
			windowName, x, y, nWidth, nHeight, std::move(prms));
	}

	std::pair<HWND, std::shared_ptr<ThreadedWindow>> ft_value = ft.get();

	HWND hWnd = ft_value.first;
	std::shared_ptr<ThreadedWindow> wnd_ptr = ft_value.second;

	TWHANDLE handle = 0;
	{
		std::lock_guard<std::mutex> lg(ThreadedWindow::twHandle_mutex);
		ThreadedWindow::twHandle_map.emplace(ThreadedWindow::twHandle_count, wnd_ptr);
		handle = ThreadedWindow::twHandle_count; 
		ThreadedWindow::twHandle_count++;
	}

	return handle;

}

template<class _Wnd>
TWHANDLE ThreadedWindow::StartWnd(LPCWSTR windowName, int x, int y, int nWidth, int nHeight) {

	std::promise<std::pair<HWND, std::shared_ptr<ThreadedWindow>>> prms;
	std::future<std::pair<HWND, std::shared_ptr<ThreadedWindow>>> ft = prms.get_future();
	
	std::thread th = std::thread(static_cast<void(*)(LPCWSTR, int, int, int, int, std::promise<std::pair<HWND, std::shared_ptr<ThreadedWindow>>>)>(WndThread<_Wnd>),
		windowName, x, y, nWidth, nHeight, std::move(prms));
	
	std::pair<HWND, std::shared_ptr<ThreadedWindow>> ft_value = ft.get();

	HWND hWnd = ft_value.first;
	std::shared_ptr<ThreadedWindow> wnd_ptr = ft_value.second;

	this->child_windows.emplace_back(hWnd, std::move(th));
	
	TWHANDLE handle = 0;
	{
		std::lock_guard<std::mutex> lg(ThreadedWindow::twHandle_mutex);
		ThreadedWindow::twHandle_map.emplace(ThreadedWindow::twHandle_count, wnd_ptr);
		handle = ThreadedWindow::twHandle_count; 
		ThreadedWindow::twHandle_count++;
	}

	return handle;

}
