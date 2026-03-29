#pragma once

#include "threaded_window.h"
#include "auxiliary.h"
#include "asset.h"

enum class RASTERIZER_WINDING_ORDER : uint8_t {

	CW = 0,
	CCW = 1,
	NONE = 2

};

enum class RASTERIZER_ALGORITHM : uint8_t {

	NAIVE = 0,
	BARYCENTRIC = 1,
	SIMD = 2

};

class Rasterizer : public ThreadedWindow {

	public:

		Rasterizer(LPCWSTR windowName, int x, int y, int nWidth, int nHeight);
		virtual ~Rasterizer();

		Rasterizer(Rasterizer& other) = delete;
		Rasterizer(Rasterizer&& other) = delete;

		Rasterizer& operator=(Rasterizer& other) = delete;
		Rasterizer& operator=(Rasterizer&& other) = delete;

	protected:

		std::vector<asset::Mesh*> pass_list;

		virtual void StartRender() final;
		virtual void ClearBackground() final;
		virtual void ClearDepthBuffer() final;
		virtual void AddMesh(asset::Mesh* mesh) final;
		virtual void AddOnVertex(std::function<void(rst::vertex<double>&)> onvertex) final;
		virtual void AddOnPixel(std::function<void(rst::vertex<double>&)> onpixel) final;
		virtual void SetWindingOrder(RASTERIZER_WINDING_ORDER winding_order) final;
		virtual void SetRasterizerAlgorithm(RASTERIZER_ALGORITHM rasterizer_algorithm) final;
		virtual void Draw() final;
		virtual UINT32 SampleTexture(double u, double v, asset::Tex& texture) final;

		virtual void SaveRender(const std::string& path) final;
		virtual void SaveDepth(const std::string& path) final;
		virtual void LogRasterizer() final;

	private:

		virtual LRESULT OnPaint(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		virtual LRESULT OnCustomEventOther(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		virtual LRESULT OnQuit() override;
		
		int ticks_per_s;

		std::thread message_poster;
		std::thread render;
		std::mutex bitmap_mutex;

		virtual void MessagePoster() final;
		bool message_poster_isRunning;

		virtual void Render() final;
		virtual void Raster(std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer) final;
		virtual void Raster_Barycentric(std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer) final;
		virtual void Raster_SIMD(std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer) final;
		virtual void Raster_SIMD_Pixel(double i, double j, std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer, int k, double w0, double w1, double w2) final;
		virtual void Clip(std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer) final;
		bool render_isRunning;

		virtual LRESULT OnRender();

		std::function<void(rst::vertex<double>&)> onvertex;
		std::function<void(rst::vertex<double>&)> onpixel;

		HDC wnd_dc;
		
		HDC off_dc;
		HBITMAP off_bitmap;
		HPEN off_pen;
		HBRUSH off_brush;

		HDC dib_dc;
		HBITMAP dib_bitmap;
		UINT32* dib_bits;

		HDC depth_dc;
		HBITMAP depth_bitmap;
		UINT32* depth_bits;
		double* depth_buffer;

		HDC save_dc;
		HBITMAP save_bitmap;
		UINT32* save_bits;

		std::vector<rst::homogenous_plane> clipping_planes;
		RASTERIZER_WINDING_ORDER winding_order;
		RASTERIZER_ALGORITHM rasterizer_algorithm;

		std::mutex log_mutex;
		bool logged;

};
