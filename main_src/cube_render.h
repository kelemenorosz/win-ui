#pragma once

#include "rasterizer.h"
#include <random>

class CubeRender : public Rasterizer {

	public:

		CubeRender(LPCWSTR windowName, int x, int y, int nWidth, int nHeight);
		virtual ~CubeRender();

		CubeRender(CubeRender& other) = delete;
		CubeRender(CubeRender&& other) = delete;

		CubeRender& operator=(CubeRender& other) = delete;
		CubeRender& operator=(CubeRender&& other) = delete;

	private:

		virtual LRESULT OnRender() override;
		void OnVertex(rst::vertex<double>& vertex);
		void OnPixel(rst::vertex<double>& pixel);

		virtual LRESULT OnMouseWheel(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		virtual LRESULT OnChar(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		asset::Mesh cube;
		asset::Mesh triangle;
		asset::Tex depth_texture;

		double cube_position_x;
		double cube_position_y;
		double cube_position_z;

		double cube_rotation;
		double cube_distance;
		std::mt19937 mersenne_twister;
		rst::color* color_buf;
		UINT64 color_counter;
		int color_len;

		UINT32* texture;
		int texture_width;
		int texture_height;
		int checker_size;

		int vertex_buffer_len;
		int index_buffer_len;

		RASTERIZER_ALGORITHM rasterizer_algorithm;

		std::vector<rst::vertex<double>> triangle_vert = {
			rst::vertex<double>(-1.0, 1.0, 1.0, 0.0, 255.0, 0.0),
			rst::vertex<double>(-1.0, -1.0, 1.0, 255.0, 0.0, 0.0),
			rst::vertex<double>(1.0, -1.0, 1.0, 0.0, 0.0, 255.0)
		};

		rst::color triangle_color[3] = {
			{0xFF,0x00,0x00}, 
			{0x00,0xFF,0x00},
			{0x00,0x00,0xFF}, 
		};

		std::vector<int> triangle_index = { 0,1,2 };

		rst::color colorlist[8] = {
			{0xFF,0x00,0x00}, 
			{0x00,0xFF,0x00},
			{0x00,0x00,0xFF}, 
			{0xFF,0x00,0xFF},
			{0xFF,0xFF,0x00}, 
			{0x00,0xFF,0xFF},
			{0x80,0x80,0x80},
			{0x66,0xFF,0xB2},
		};

		rst::point<double> vertexlist[8] = {
			{0.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 0.0f},
			{0.0f, 0.0f, 0.0f},
			{1.0f, 0.0f, 0.0f},
			{0.0f, 1.0f, -1.0f},
			{1.0f, 1.0f, -1.0f},
			{0.0f, 0.0f, -1.0f},
			{1.0f, 0.0f, -1.0f},
		};

		int indexlist[36] = {
			2,0,1,
			2,1,3,
			
			3,1,5,
			3,5,7,
			
			6,4,0,
			6,0,2,
			
			7,5,4,
			7,4,6,
			
			0,4,5,
			0,5,1,
			
			6,2,3,
			6,3,7,
		};

};