
#include <cstdio>
#include <windows.h>
#include "main.h"
#include "threaded_window.h"

A::A() {
	printf("A::A().\n");
}

A::~A() {
	printf("A::~A().\n");
}

void A::Add(int x) {
	v.push_back(x);
	return;
}

void A::Print() {
	for (auto x : v) {
		printf("%d ", x);
	}
	printf("\n");
}

// void OnDllProcessAttach() {

// 	HMODULE hModule = GetModuleHandle(NULL);

// 	ThreadedWindow::hInstance = hModule;

// 	WNDCLASS window_class = {};
// 	window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
//   	window_class.lpfnWndProc = ThreadedWindow::s_WindProc;
//   	window_class.cbClsExtra = NULL;
//   	window_class.cbWndExtra = NULL;
//   	window_class.hInstance = ThreadedWindow::hInstance;
//   	window_class.hIcon = NULL;
//   	window_class.hCursor = NULL;
//   	window_class.hbrBackground = (HBRUSH)COLOR_GRAYTEXT;
//   	window_class.lpszMenuName = NULL;
//   	window_class.lpszClassName = L"ThreadedWindow";

// 	::RegisterClass(&window_class);

// 	return;

// }

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {

	switch(fdwReason) {

		case DLL_PROCESS_ATTACH:
			// OnDllProcessAttach();
			break;
		case DLL_PROCESS_DETACH:
			// printf("DllMain() DLL_PROCESS_DETACH.\n");
			break;
		case DLL_THREAD_ATTACH:
			// printf("DllMain() DLL_THREAD_ATTACH.\n");
			break;
		case DLL_THREAD_DETACH:
			// printf("DllMain() DLL_THREAD_DETACH.\n");
			break;
		default:
			break;

	}

	return TRUE;

}
