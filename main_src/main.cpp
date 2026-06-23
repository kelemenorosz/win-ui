#include <cstdio>
#include <windows.h>
#include <iostream>
#include <csignal>
#include "dx12_window.h"
#include "plot_layer.h"
#include "fm_synth.h"
#include "rasterizer.h"
#include "cube_render.h"

int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR lpCmdLine, _In_ int nCmdShow) {

	AllocConsole();
	FILE* dummy_file;
	freopen_s(&dummy_file, "CONOUT$", "w", stdout);
	std::cout.clear();

	ThreadedWindow::Initialize(GetModuleHandle(NULL));
	DX12Window::InitDebug();
	
	// TWHANDLE dxHandle = StartWnd<DX12Window>(L"DX12 Window", 0, 0, 640, 640);
	// DX12Window::RegisterLayer<PlotLayer>(dxHandle);

	// TWHANDLE fmHandle = StartWnd<FMSynth>(L"FM Synth Window", 0, 0, 640, 640);

	// float* range = new float[48000];
	// for (int i = 0; i < 48000; ++i) *(range + i) = static_cast<float>(i);

	// PlotLayer::InitDesc initDesc = { reinterpret_cast<void*>(range), reinterpret_cast<void*>(range), 48000, 24000.0f, 24000.0f, -1.0f, -1.0f };
	// DX12Window::LayerMessage(dxHandle, DX12Layer_Event_Type::DX12LAYER_EVENT_TYPE_INIT, reinterpret_cast<void*>(&initDesc));

	// delete[] range;

	// TWHANDLE rstHandle = StartWnd<Rasterizer>(L"Rasterizer Window", 0, 0, 640, 640);

	TWHANDLE cubeHandle = StartWnd<CubeRender>(L"Cube Render Window", 0, 0, 640, 640);
	
	ThreadedWindow::Shutdown();
	DX12Window::ShutdownDebug();

	return 0;

}