
#include "rasterizer.h"
#include "bitmap.h"
#include "pix3.h"
#include <numbers>
#include <concepts>

Rasterizer::Rasterizer(LPCWSTR windowName, int x, int y, int nWidth, int nHeight) : ThreadedWindow(windowName, x, y, nWidth, nHeight),
	ticks_per_s(120), message_poster_isRunning(true), render_isRunning(true), winding_order(RASTERIZER_WINDING_ORDER::CW),
	rasterizer_algorithm(RASTERIZER_ALGORITHM::NAIVE), rasterizer_blend(RASTERIZER_BLEND::NONE), blend_coefficient(0.5), logged(true),
	default_texture("./tex/default_tex.bmp") {

	// -- Window dc
	wnd_dc = GetDC(hWnd);

	// -- Offscreen dc
	off_dc = CreateCompatibleDC(0);
	off_bitmap = CreateCompatibleBitmap(wnd_dc, nWidth, nHeight);
	off_pen = CreatePen(PS_SOLID, 2, RGB(255,0,0));
	off_brush = CreateSolidBrush(RGB(0,0,255));
	SelectObject(off_dc, off_bitmap);
	SelectObject(off_dc, off_pen);
	SelectObject(off_dc, off_brush);

	// -- Device independent bitmap dc
	dib_dc = CreateCompatibleDC(0);
	BITMAPINFO bitmapinfo = {};
	bitmapinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapinfo.bmiHeader.biWidth = nWidth;
	bitmapinfo.bmiHeader.biHeight = -nHeight;
	bitmapinfo.bmiHeader.biPlanes = 1;
	bitmapinfo.bmiHeader.biBitCount = 32;
	bitmapinfo.bmiHeader.biCompression = BI_RGB;
	bitmapinfo.bmiHeader.biSizeImage = 0;
	bitmapinfo.bmiHeader.biXPelsPerMeter = 0;
	bitmapinfo.bmiHeader.biYPelsPerMeter = 0;
	bitmapinfo.bmiHeader.biClrUsed = 0;
	bitmapinfo.bmiHeader.biClrImportant = 0;
	dib_bitmap = CreateDIBSection(wnd_dc, &bitmapinfo, DIB_RGB_COLORS, reinterpret_cast<void**>(&dib_bits), NULL, 0);
	SelectObject(dib_dc, dib_bitmap);

	// -- Save device independent bitmap dc
	save_dc = CreateCompatibleDC(0);
	save_bitmap = CreateDIBSection(wnd_dc, &bitmapinfo, DIB_RGB_COLORS, reinterpret_cast<void**>(&save_bits), NULL, 0);
	SelectObject(save_dc, save_bitmap);

	// -- Depth buffer
	depth_buffer = new double[nWidth * nHeight];
	depth_dc = CreateCompatibleDC(0);
	depth_bitmap = CreateDIBSection(wnd_dc, &bitmapinfo, DIB_RGB_COLORS, reinterpret_cast<void**>(&depth_bits), NULL, 0);
	SelectObject(depth_dc, depth_bitmap);

	// -- Clipping planes
	clipping_planes = std::vector<rst::homogenous_plane>(6);
	clipping_planes[0] = rst::homogenous_plane(0, -1); // Near Z plane
	clipping_planes[1] = rst::homogenous_plane(0, 1); // Far Z plane
	clipping_planes[2] = rst::homogenous_plane(1, -1); // Left X plane
	clipping_planes[3] = rst::homogenous_plane(1, 1); // Right X plane
	clipping_planes[4] = rst::homogenous_plane(2, -1); // Bottom Y plane
	clipping_planes[5] = rst::homogenous_plane(2, 1); // Top Y plane

	message_poster = std::thread(&Rasterizer::MessagePoster, this);
	
	return;

}

Rasterizer::~Rasterizer() {

	delete[] depth_buffer;
	return;

}

LRESULT Rasterizer::OnPaint(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	HWND hWnd = windowInstance;

	PAINTSTRUCT ps = {};
	HDC dc = BeginPaint(hWnd, &ps);
	EndPaint(hWnd, &ps);

	return S_OK;

}

LRESULT Rasterizer::OnCustomEventOther(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	PIXScopedEvent(PIX_COLOR(0xFF, 0x00, 0x00), "Rasterizer::OnCustomEventOther()");

	{
		std::lock_guard<std::mutex> lg(bitmap_mutex);
		BitBlt(wnd_dc, 0, 0, nWidth, nHeight, off_dc, 0, 0, SRCCOPY);
	}

	return S_OK;

}

LRESULT Rasterizer::OnQuit() {

	message_poster_isRunning = false;
	render_isRunning = false;

	if (message_poster.joinable()) message_poster.join();
	if (render.joinable()) render.join();

	return S_OK;

}

LRESULT Rasterizer::OnRender() {

	return S_OK;

}

void Rasterizer::StartRender() {

	render = std::thread(&Rasterizer::Render, this);

	return;

}

void Rasterizer::MessagePoster() {

	LARGE_INTEGER freq = {};
	LARGE_INTEGER count = {};
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);

	INT64 t0_i64 = count.QuadPart;
	INT64 freq_i64 = freq.QuadPart;

	double tick_duration = 1.0f / static_cast<double>(ticks_per_s);
	double sleep_time_fp = tick_duration / 3.0;
	INT64 sleep_time_microseconds = static_cast<INT64>(sleep_time_fp * 1000000);
	INT64 current_tick = 0;

	double freq_f = static_cast<double>(freq_i64);	
	double count_time_s = 1.0f / freq_f;
	
	double elapsed_time_s = 0.0f;

	while (message_poster_isRunning) {

		QueryPerformanceCounter(&count);
		INT64 t1_i64 = count.QuadPart;
		INT64 delta = t1_i64 - t0_i64;
		double delta_f = count_time_s * static_cast<double>(delta);
		elapsed_time_s += delta_f;
		t0_i64 = t1_i64;

		if ((elapsed_time_s > tick_duration)) {
			elapsed_time_s = 0;
			CustomEventOtherWnd_Post(hWnd, reinterpret_cast<void*>(0));
		}

		std::this_thread::sleep_for(std::chrono::microseconds(sleep_time_microseconds));
	}
	
}

void Rasterizer::Render() {

	LARGE_INTEGER freq = {};
	LARGE_INTEGER count = {};
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);

	INT64 t0_i64 = count.QuadPart;
	INT64 freq_i64 = freq.QuadPart;
	double freq_f = static_cast<double>(freq_i64);	
	double count_time_s = 1.0f / freq_f;

	INT64 fps_count = 0;
	double elapsed_time_s = 0.0f;

	while (render_isRunning) {

		PIXScopedEvent(PIX_COLOR(0xFF, 0xFF, 0x00), "Rasterizer::Render() render loop");
		OnRender();

		{
			PIXScopedEvent(PIX_COLOR(0xFF, 0xFF, 0x00), "Rasterizer::Render() bitmap mutex");
			
			std::lock_guard<std::mutex> lg(bitmap_mutex);
			BitBlt(off_dc, 0, 0, nWidth, nHeight, dib_dc, 0, 0, SRCCOPY);
			for (int i = 0; i < nWidth * nHeight; ++i) {
				UINT32* bitmap_ptr = &reinterpret_cast<UINT32*>(depth_bits)[i];
				UINT8 depth_u8 = (depth_buffer[i] - 1) * (-127.5);
				*bitmap_ptr = depth_u8 | (depth_u8 << 8) | (depth_u8 << 16) | (depth_u8 << 24);
			}
		}
		
		QueryPerformanceCounter(&count);
		INT64 t1_i64 = count.QuadPart;
		INT64 delta = t1_i64 - t0_i64;
		double delta_f = count_time_s * static_cast<double>(delta);
		elapsed_time_s += delta_f;
		t0_i64 = t1_i64;

		fps_count++;
		if (elapsed_time_s > 1.0) {
			printf("FPS: %I64d.\n", fps_count);
			fps_count = 0;
			elapsed_time_s = 0;
		}

	}
	
	return;
	
}

void Rasterizer::Raster(std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer) {

	FILE* log_file = nullptr;
	if (!logged) log_file = fopen("log", "wb");
	if (!logged) fprintf(log_file, "Rasterizer::Raster().\n");

	// -- Convert vertex buffer floating point values to integers
	rst::bufferlist<rst::point<int>> vertexbuffer_si = {
		position_to<double, int>(&vertexbuffer[0], vertexbuffer.size(), nWidth / 2, nHeight / 2),
		static_cast<int>(vertexbuffer.size())
	};

	if (!logged) {
		// for (int i = 0; i < vertexbuffer_si.len; ++i) {
		// 	fprintf(log_file, "Vertex %d position x: %d, y: %d, z: %d, w: %d.\n", i, vertexbuffer_si.ptr[i].x, vertexbuffer_si.ptr[i].y, vertexbuffer_si.ptr[i].z, vertexbuffer_si.ptr[i].w);
		// }
	}

	// -- For every triangle described in indexbuffer
	for (int k = 0; k < indexbuffer.size(); k += 3) {

		// -- Triangle bounding box
		rst::rectangle<int> rect = triangle_bounding_box(
			vertexbuffer_si.ptr[indexbuffer[k+0]],
			vertexbuffer_si.ptr[indexbuffer[k+1]],
			vertexbuffer_si.ptr[indexbuffer[k+2]]);

		if (!logged) {
			fprintf(log_file, "Bounding box: %d %d %d %d.\n", rect.left, rect.top, rect.right, rect.bottom);
			
			INT64 A01 = vertexbuffer_si.ptr[indexbuffer[k+0]].y - vertexbuffer_si.ptr[indexbuffer[k+1]].y; 
			INT64 A12 = vertexbuffer_si.ptr[indexbuffer[k+1]].y - vertexbuffer_si.ptr[indexbuffer[k+2]].y; 
			INT64 A20 = vertexbuffer_si.ptr[indexbuffer[k+2]].y - vertexbuffer_si.ptr[indexbuffer[k+0]].y; 

			INT64 B01 = vertexbuffer_si.ptr[indexbuffer[k+1]].x - vertexbuffer_si.ptr[indexbuffer[k+0]].x;
			INT64 B12 = vertexbuffer_si.ptr[indexbuffer[k+2]].x - vertexbuffer_si.ptr[indexbuffer[k+1]].x;
			INT64 B20 = vertexbuffer_si.ptr[indexbuffer[k+0]].x - vertexbuffer_si.ptr[indexbuffer[k+2]].x;

			fprintf(log_file, "Barycentric coordinates: A01: %I64d, A12: %I64d, A20: %I64d, B01: %I64d, B12: %I64d, B20: %I64d.\n",
			 A01, A12, A20, B01, B12, B20);
		}

		// -- Fill triangle
		for (int i = rect.top; i <= rect.bottom; ++i) {
			for (int j = rect.left; j <= rect.right; ++j) {

				
				if (i >= 0 && j >= 0 && i < nWidth && j < nHeight) {

					rst::point<int> pixel = {j, i, 0, 0};
					bool isPoint = false;

					if (!logged) {
						fprintf(log_file, "Candidate pixel: i: %d, j: %d, ", i, j);
						isPoint = is_point_in_triangle_verbose(
							log_file,
							vertexbuffer_si.ptr[indexbuffer[k+0]],
							vertexbuffer_si.ptr[indexbuffer[k+1]], 
							vertexbuffer_si.ptr[indexbuffer[k+2]],
							pixel);
					}
					else {

						isPoint = is_point_in_triangle(
							vertexbuffer_si.ptr[indexbuffer[k+0]],
							vertexbuffer_si.ptr[indexbuffer[k+1]], 
							vertexbuffer_si.ptr[indexbuffer[k+2]],
							pixel);
					
					}

					// -- If bitmap pixel is inside triangle
					if (isPoint) {
						
						if (!logged) fprintf(log_file, "Rendered pixel i: %d j: %d.\n", i, j);

						// -- Interpolating colors, uv coords, etc.
						rst::point<double> coefs = rst::barycentric_coefficients<int, double>(
							vertexbuffer_si.ptr[indexbuffer[k+0]],
							vertexbuffer_si.ptr[indexbuffer[k+1]],
							vertexbuffer_si.ptr[indexbuffer[k+2]],
							pixel);

						double w0 = vertexbuffer[indexbuffer[k+0]].position.w;
						double w1 = vertexbuffer[indexbuffer[k+1]].position.w;
						double w2 = vertexbuffer[indexbuffer[k+2]].position.w;
						
						double l0 = coefs.x / w0;
						double l1 = coefs.y / w1;
						double l2 = coefs.z / w2;

						double lsum = l0 + l1 + l2;

						l0 /= lsum;
						l1 /= lsum;
						l2 /= lsum;

						double z = l0 * vertexbuffer[indexbuffer[k+0]].position.z + l1 * vertexbuffer[indexbuffer[k+1]].position.z + l2 * vertexbuffer[indexbuffer[k+2]].position.z; 

						// -- Compare with depth buffer
						if (depth_buffer[i * nWidth + j] < z) continue;
						depth_buffer[i * nWidth + j] = z;

						rst::vertex<double> pixel = {};
						pixel.texture = 
							l0 * vertexbuffer[indexbuffer[k+0]].texture + 
							l1 * vertexbuffer[indexbuffer[k+1]].texture + 
							l2 * vertexbuffer[indexbuffer[k+2]].texture;
						pixel.color = 
							l0 * vertexbuffer[indexbuffer[k+0]].color + 
							l1 * vertexbuffer[indexbuffer[k+1]].color + 
							l2 * vertexbuffer[indexbuffer[k+2]].color;

						this->onpixel(pixel);

						BYTE r_byte = pixel.color.x;
						BYTE g_byte = pixel.color.y;
						BYTE b_byte = pixel.color.z;
						UINT32 color_u32 = (((UINT32)r_byte) << 0x10) + (((UINT32)g_byte) << 0x8) + (UINT32)b_byte;							

						dib_bits[i * nWidth + j] = color_u32;

					}
				}
			}
		}
	
	}

	delete[] vertexbuffer_si.ptr;

	if (!logged) fclose(log_file);
	if (!logged) logged = true;

	return;

}

void Rasterizer::Raster_Barycentric(std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer) {

	PIXScopedEvent(PIX_COLOR(0xFF, 0x00, 0xFF), "Rasterizer::Raster_Barycentric()");

	FILE* log_file = nullptr;
	if (!logged) log_file = fopen("Raster_Barycentric.log", "wb");
	if (!logged) fprintf(log_file, "Rasterizer::Raster_Barycentric().\n");

	// -- Convert to int screen size
	std::vector<rst::point<INT64>> positionbuffer;
	for (rst::vertex<double>& vertex : vertexbuffer) {
		rst::point<INT64> p;
		de_normalize(vertex.position, p, nWidth / 2, nHeight / 2);
		positionbuffer.push_back(p);
	}

	INT64 subpixel_precision = 0;
	INT64 subpixel_precision_step = pow(2, subpixel_precision);
	INT64 subpixel_mask = pow(2, subpixel_precision) - 1;

	// -- Convert to subpixel precision
	for (rst::point<INT64>& position : positionbuffer) {
		position.x *= subpixel_precision_step;
		position.y *= subpixel_precision_step;
		position.z *= subpixel_precision_step;
		position.w *= subpixel_precision_step;
	}

	if (!logged) {
		// for (int i = 0; i < positionbuffer.size(); ++i) {
		// 	fprintf(log_file, "Vertex %d position x: %I64d, y: %I64d, z: %I64d, w: %I64d.\n", i, positionbuffer[i].x, positionbuffer[i].y, positionbuffer[i].z, positionbuffer[i].w);
		// }
	}

	// -- Loop triangles
	for (int k = 0; k < indexbuffer.size(); k += 3) {

		rst::point<INT64>& A_pos = positionbuffer[indexbuffer[k+0]];
		rst::point<INT64>& B_pos = positionbuffer[indexbuffer[k+1]];
		rst::point<INT64>& C_pos = positionbuffer[indexbuffer[k+2]];

		rst::rectangle<INT64> bounding_box = triangle_bounding_box(A_pos, B_pos, C_pos);

		// -- Edge function
		// -- E_xy(p) = (vx_y - vy_y)*p_x + (vy_x - vx_x)*p_y + (vx_x*vy_y - vx_y*vy_x)
		// -- E_xy(p) = A_xy*p_x + B_xy*p_y + C_xy

		INT64 A01 = A_pos.y - B_pos.y; 
		INT64 A12 = B_pos.y - C_pos.y; 
		INT64 A20 = C_pos.y - A_pos.y; 

		INT64 B01 = B_pos.x - A_pos.x;
		INT64 B12 = C_pos.x - B_pos.x;
		INT64 B20 = A_pos.x - C_pos.x;

		// -- Barycentric coordinates at min/min
		// -- Min/Min at rounded up coordinates
		rst::point<INT64> min_point = {bounding_box.left, bounding_box.top, 0, 0};
		// min_point.x = (min_point.x + subpixel_mask) & ~subpixel_mask;
		// min_point.y = (min_point.y + subpixel_mask) & ~subpixel_mask;

		if (!logged) {
			fprintf(log_file, "Bounding box: %I64d %I64d %I64d %I64d.\n", bounding_box.left, bounding_box.top, bounding_box.right, bounding_box.bottom);
			fprintf(log_file, "Barycentric coordinates: A01: %I64d, A12: %I64d, A20: %I64d, B01: %I64d, B12: %I64d, B20: %I64d.\n",
			 A01, A12, A20, B01, B12, B20);
		}

		INT64 edge_min_0 = rst::edge(B_pos, C_pos, min_point);
		INT64 edge_min_1 = rst::edge(C_pos, A_pos, min_point);
		INT64 edge_min_2 = rst::edge(A_pos, B_pos, min_point);
		INT64 area = rst::edge(A_pos, B_pos, C_pos);

		INT64 bias0 = rst::is_top_left(B_pos, C_pos) ? 0 : -(1 << subpixel_precision);
		INT64 bias1 = rst::is_top_left(C_pos, A_pos) ? 0 : -(1 << subpixel_precision);
		INT64 bias2 = rst::is_top_left(A_pos, B_pos) ? 0 : -(1 << subpixel_precision);
		// INT64 bias0 = 0;
		// INT64 bias1 = 0;
		// INT64 bias2 = 0;

		if (!logged) fprintf(log_file, "Bias: %I64d, %I64d, %I64d.\n", bias0, bias1, bias2);

		for (INT64 i = min_point.y; i <= bounding_box.bottom; i += subpixel_precision_step) {

			INT64 edge_0 = edge_min_0;
			INT64 edge_1 = edge_min_1;
			INT64 edge_2 = edge_min_2;

			for (INT64 j = min_point.x; j <= bounding_box.right; j += subpixel_precision_step) {

				if (!logged) {
					fprintf(log_file, "Candidate pixel: i: %I64d, j: %I64d, edge0: %I64d, edge1: %I64d, edge2: %I64d.\n", i, j, edge_0, edge_1, edge_2);
					{
						rst::point<double> barycentric_coefs(edge_0, edge_1, edge_2, 0);
						if (area != 0) {
							barycentric_coefs.x /= 2.0 * static_cast<double>(area);
							barycentric_coefs.y /= 2.0 * static_cast<double>(area);
							barycentric_coefs.z /= 2.0 * static_cast<double>(area);							
						}
						else {
							barycentric_coefs.x = 1.0;	
							barycentric_coefs.y = 0.0;	
							barycentric_coefs.z = 0.0;	
						}
						// fprintf(log_file, "Barycentric coords: %f, %f, %f.\n", barycentric_coefs.x, barycentric_coefs.y, barycentric_coefs.z);
					}
				}

				if (i >= 0 && j >= 0 && (i / subpixel_precision_step) < nWidth && (j / subpixel_precision_step) < nHeight) {
					
					if ((edge_0 + bias0) >= 0 && (edge_1 + bias1) >= 0 && (edge_2 + bias2) >= 0) {

						if (!logged) fprintf(log_file, "Rendered pixel i: %I64d j: %I64d.\n", i, j);

						rst::point<double> barycentric_coefs(edge_0, edge_1, edge_2, 0);
						if (area != 0) {
							barycentric_coefs.x /= 2.0 * static_cast<double>(area);
							barycentric_coefs.y /= 2.0 * static_cast<double>(area);
							barycentric_coefs.z /= 2.0 * static_cast<double>(area);							
						}
						else {
							barycentric_coefs.x = 1.0;	
							barycentric_coefs.y = 0.0;	
							barycentric_coefs.z = 0.0;	
						}

						double w0 = vertexbuffer[indexbuffer[k+0]].position.w;
						double w1 = vertexbuffer[indexbuffer[k+1]].position.w;
						double w2 = vertexbuffer[indexbuffer[k+2]].position.w;
						
						double l0 = barycentric_coefs.x / w0;
						double l1 = barycentric_coefs.y / w1;
						double l2 = barycentric_coefs.z / w2;

						double lsum = l0 + l1 + l2;

						l0 /= lsum;
						l1 /= lsum;
						l2 /= lsum;

						double z = l0 * vertexbuffer[indexbuffer[k+0]].position.z + l1 * vertexbuffer[indexbuffer[k+1]].position.z + l2 * vertexbuffer[indexbuffer[k+2]].position.z; 

						// -- Compare with depth buffer
						if (depth_buffer[(i / subpixel_precision_step) * nWidth + (j / subpixel_precision_step)] < z) {
						
						}
						else {

							depth_buffer[(i / subpixel_precision_step) * nWidth + (j / subpixel_precision_step)] = z;

							rst::vertex<double> pixel = {};
							pixel.texture = 
								l0 * vertexbuffer[indexbuffer[k+0]].texture + 
								l1 * vertexbuffer[indexbuffer[k+1]].texture + 
								l2 * vertexbuffer[indexbuffer[k+2]].texture;
							pixel.color = 
								l0 * vertexbuffer[indexbuffer[k+0]].color + 
								l1 * vertexbuffer[indexbuffer[k+1]].color + 
								l2 * vertexbuffer[indexbuffer[k+2]].color;

							this->onpixel(pixel);

							if (rasterizer_blend == RASTERIZER_BLEND::NONE) {
								BYTE r_byte = pixel.color.x;
								BYTE g_byte = pixel.color.y;
								BYTE b_byte = pixel.color.z;
								UINT32 color_u32 = (((UINT32)r_byte) << 0x10) + (((UINT32)g_byte) << 0x8) + (UINT32)b_byte;							

								dib_bits[(i / subpixel_precision_step) * nWidth + (j / subpixel_precision_step)] = color_u32;
							}
							else if (rasterizer_blend == RASTERIZER_BLEND::ENABLED) {

								double r_src = pixel.color.x;
								double g_src = pixel.color.y;
								double b_src = pixel.color.z;
								r_src *= blend_coefficient;
								g_src *= blend_coefficient;
								b_src *= blend_coefficient;

								UINT32 dest_color_u32 = dib_bits[(i / subpixel_precision_step) * nWidth + (j / subpixel_precision_step)];

								double r_dest = static_cast<UINT8>(dest_color_u32 >> 0x10);
								double g_dest = static_cast<UINT8>(dest_color_u32 >> 0x08);
								double b_dest = static_cast<UINT8>(dest_color_u32 >> 0x00);
								r_dest *= (1.0 - blend_coefficient);
								g_dest *= (1.0 - blend_coefficient);
								b_dest *= (1.0 - blend_coefficient);

								double r_dp = r_src + r_dest;
								double g_dp = g_src + g_dest;
								double b_dp = b_src + b_dest;
								BYTE r_byte = static_cast<UINT8>(r_dp);
								BYTE g_byte = static_cast<UINT8>(g_dp);
								BYTE b_byte = static_cast<UINT8>(b_dp);
								UINT32 color_u32 = (((UINT32)r_byte) << 0x10) + (((UINT32)g_byte) << 0x8) + (UINT32)b_byte;
								dib_bits[(i / subpixel_precision_step) * nWidth + (j / subpixel_precision_step)] = color_u32;

							}

						}

					}
						
				}

				edge_0 += A12 * subpixel_precision_step;
				edge_1 += A20 * subpixel_precision_step;
				edge_2 += A01 * subpixel_precision_step;
			
			}

			edge_min_0 += B12 * subpixel_precision_step;
			edge_min_1 += B20 * subpixel_precision_step;
			edge_min_2 += B01 * subpixel_precision_step;
		}

	}

	if (!logged) fclose(log_file);
	if (!logged) logged = true;

	return;

}

void Rasterizer::Raster_SIMD_Integer(std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer) {

	PIXScopedEvent(PIX_COLOR(0xFF, 0x00, 0xFF), "Rasterizer::Raster_SIMD_Integer()");

	FILE* log_file = nullptr;
	if (!logged) log_file = fopen("Raster_SIMD_Integer.log", "wb");
	if (!logged) fprintf(log_file, "Rasterizer::Raster_SIMD_Integer().\n");

	std::vector<rst::point<INT32>> positionbuffer;
	for (rst::vertex<double>& vertex : vertexbuffer) {
		rst::point<INT32> p;
		de_normalize(vertex.position, p, nWidth / 2, nHeight / 2);
		positionbuffer.push_back(p);
	}

	for (int k = 0; k < indexbuffer.size(); k += 3) {

		rst::point<INT32>& A_pos = positionbuffer[indexbuffer[k+0]];
		rst::point<INT32>& B_pos = positionbuffer[indexbuffer[k+1]];
		rst::point<INT32>& C_pos = positionbuffer[indexbuffer[k+2]];
		rst::rectangle<INT32> bounding_box = triangle_bounding_box(A_pos, B_pos, C_pos);
		rst::point<INT32> min_point = {bounding_box.left, bounding_box.top, 0, 0};
		INT32 area = rst::edge(A_pos, B_pos, C_pos);

		rst::edge_simd_integer AB_edge(A_pos, B_pos, min_point);
		rst::edge_simd_integer BC_edge(B_pos, C_pos, min_point);
		rst::edge_simd_integer CA_edge(C_pos, A_pos, min_point);

		__m128i AB_row = AB_edge.get_initial();
		__m128i BC_row = BC_edge.get_initial();
		__m128i CA_row = CA_edge.get_initial();

		int step_x = 4;
		int step_y = 1;

		for (INT32 i = min_point.y; i <= bounding_box.bottom; i += step_y) {

			__m128i AB_edge_val = AB_row;
			__m128i BC_edge_val = BC_row;
			__m128i CA_edge_val = CA_row;

			for (INT32 j = min_point.x; j <= bounding_box.right; j += step_x) {


				__m128i or_edge_val;
				or_edge_val = _mm_or_si128(AB_edge_val, BC_edge_val);
				or_edge_val = _mm_or_si128(or_edge_val, CA_edge_val);

				if (!logged) {
					INT32 or_buf[4];
					INT32 w0_buf[4];
					INT32 w1_buf[4];
					INT32 w2_buf[4];
					_mm_storeu_si128(reinterpret_cast<__m128i*>(&or_buf[0]), or_edge_val);	
					_mm_storeu_si128(reinterpret_cast<__m128i*>(&w0_buf[0]), AB_edge_val);
					_mm_storeu_si128(reinterpret_cast<__m128i*>(&w1_buf[0]), BC_edge_val);
					_mm_storeu_si128(reinterpret_cast<__m128i*>(&w2_buf[0]), CA_edge_val);
					for (int x = 0; x < 4; x++) {
						fprintf(log_file, "Candidate pixel: i: %d, j: %d, edge0: %d, edge1: %d, edge2: %d, or: %d.\n", i, j + x, w1_buf[x], w2_buf[x], w0_buf[x], or_buf[x]);
					}
				}

				__m128i positive_bitmask = rst::edge_simd_integer::load_si32(0);
				positive_bitmask = _mm_cmpgt_epi32(or_edge_val, positive_bitmask); 		// -- Sets 0xFFFFFFFF if value is positive, 0x00 if negative
				int positive = _mm_testz_si128(positive_bitmask, positive_bitmask); // -- returns 0 if all values are negative

				if (!positive) {

					if (!logged) {
						INT32 or_buf[4];
						INT32 w0_buf[4];
						INT32 w1_buf[4];
						INT32 w2_buf[4];
						_mm_storeu_si128(reinterpret_cast<__m128i*>(&or_buf[0]), or_edge_val);
						_mm_storeu_si128(reinterpret_cast<__m128i*>(&w0_buf[0]), AB_edge_val);
						_mm_storeu_si128(reinterpret_cast<__m128i*>(&w1_buf[0]), BC_edge_val);
						_mm_storeu_si128(reinterpret_cast<__m128i*>(&w2_buf[0]), CA_edge_val);
						for (int x = 0; x < 4; x++) {
							fprintf(log_file, "Rendered pixel: i: %d, j: %d, edge0: %d, edge1: %d, edge2: %d, or: %d.\n", i, j + x, w1_buf[x], w2_buf[x], w0_buf[x], or_buf[x]);
						}
					}
					Raster_SIMD_Render_Group(vertexbuffer, indexbuffer, positionbuffer, i, j, or_edge_val, BC_edge_val, CA_edge_val, AB_edge_val, area, k);

				}

				AB_edge_val = _mm_add_epi32(AB_edge_val, AB_edge.diff_x);
				BC_edge_val = _mm_add_epi32(BC_edge_val, BC_edge.diff_x);
				CA_edge_val = _mm_add_epi32(CA_edge_val, CA_edge.diff_x);
			
			}

			AB_row = _mm_add_epi32(AB_row, AB_edge.diff_y); 
			BC_row = _mm_add_epi32(BC_row, BC_edge.diff_y); 
			CA_row = _mm_add_epi32(CA_row, CA_edge.diff_y); 

		}


	}

	if (!logged) fclose(log_file);
	if (!logged) logged = true;

	return;

}

void Rasterizer::Raster_SIMD_Render_Group(std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer, std::vector<rst::point<INT32>>& positionbuffer, int i, int j,
	__m128i or_edge_val, __m128i w0, __m128i w1, __m128i w2, INT32 area, int k) {

	INT32 or_buf[4];
	INT32 w0_buf[4];
	INT32 w1_buf[4];
	INT32 w2_buf[4];
	_mm_storeu_si128(reinterpret_cast<__m128i*>(&or_buf[0]), or_edge_val);
	_mm_storeu_si128(reinterpret_cast<__m128i*>(&w0_buf[0]), w0);
	_mm_storeu_si128(reinterpret_cast<__m128i*>(&w1_buf[0]), w1);
	_mm_storeu_si128(reinterpret_cast<__m128i*>(&w2_buf[0]), w2);

	for (int x = 0; x < 4; x++) {
		if (or_buf[x] > 0) {
			Raster_SIMD_Render_Pixel(vertexbuffer, indexbuffer, positionbuffer, i, j + x, w0_buf[x], w1_buf[x], w2_buf[x], area, k);
		}
	}

	return;

}

void Rasterizer::Raster_SIMD_Render_Pixel(std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer, std::vector<rst::point<INT32>>& positionbuffer, int i, int j,
	INT32 w0_edge, INT32 w1_edge, INT32 w2_edge, INT32 area, int k) {

	PIXScopedEvent(PIX_COLOR(0xFF, 0x00, 0xFF), "Rasterizer::Raster_SIMD_Render_Pixel()");

	if (!(i >= 0 && j >= 0 && i < nHeight && j < nWidth)) {
		return;
	}

	rst::point<double> barycentric_coefs(w0_edge, w1_edge, w2_edge, 0);
	if (area != 0) {
		barycentric_coefs.x /= 2.0 * static_cast<double>(area);
		barycentric_coefs.y /= 2.0 * static_cast<double>(area);
		barycentric_coefs.z /= 2.0 * static_cast<double>(area);							
	}
	else {
		barycentric_coefs.x = 1.0;	
		barycentric_coefs.y = 0.0;	
		barycentric_coefs.z = 0.0;	
	}

	double w0 = vertexbuffer[indexbuffer[k+0]].position.w;
	double w1 = vertexbuffer[indexbuffer[k+1]].position.w;
	double w2 = vertexbuffer[indexbuffer[k+2]].position.w;
	
	double l0 = barycentric_coefs.x / w0;
	double l1 = barycentric_coefs.y / w1;
	double l2 = barycentric_coefs.z / w2;

	double lsum = l0 + l1 + l2;

	l0 /= lsum;
	l1 /= lsum;
	l2 /= lsum;

	double z = l0 * vertexbuffer[indexbuffer[k+0]].position.z + l1 * vertexbuffer[indexbuffer[k+1]].position.z + l2 * vertexbuffer[indexbuffer[k+2]].position.z; 

	// -- Compare with depth buffer
	if (depth_buffer[i * nWidth + j] < z) {
	
	}
	else {

		depth_buffer[i * nWidth + j] = z;

		rst::vertex<double> pixel = {};
		pixel.texture = 
			l0 * vertexbuffer[indexbuffer[k+0]].texture + 
			l1 * vertexbuffer[indexbuffer[k+1]].texture + 
			l2 * vertexbuffer[indexbuffer[k+2]].texture;
		pixel.color = 
			l0 * vertexbuffer[indexbuffer[k+0]].color + 
			l1 * vertexbuffer[indexbuffer[k+1]].color + 
			l2 * vertexbuffer[indexbuffer[k+2]].color;

		this->onpixel(pixel);

		if (rasterizer_blend == RASTERIZER_BLEND::NONE) {
			BYTE r_byte = pixel.color.x;
			BYTE g_byte = pixel.color.y;
			BYTE b_byte = pixel.color.z;
			UINT32 color_u32 = (((UINT32)r_byte) << 0x10) + (((UINT32)g_byte) << 0x8) + (UINT32)b_byte;							

			dib_bits[i * nWidth + j] = color_u32;
		}
		else if (rasterizer_blend == RASTERIZER_BLEND::ENABLED) {

			double r_src = pixel.color.x;
			double g_src = pixel.color.y;
			double b_src = pixel.color.z;
			r_src *= blend_coefficient;
			g_src *= blend_coefficient;
			b_src *= blend_coefficient;

			UINT32 dest_color_u32 = dib_bits[i * nWidth + j];

			double r_dest = static_cast<UINT8>(dest_color_u32 >> 0x10);
			double g_dest = static_cast<UINT8>(dest_color_u32 >> 0x08);
			double b_dest = static_cast<UINT8>(dest_color_u32 >> 0x00);
			r_dest *= (1.0 - blend_coefficient);
			g_dest *= (1.0 - blend_coefficient);
			b_dest *= (1.0 - blend_coefficient);

			double r_dp = r_src + r_dest;
			double g_dp = g_src + g_dest;
			double b_dp = b_src + b_dest;
			BYTE r_byte = static_cast<UINT8>(r_dp);
			BYTE g_byte = static_cast<UINT8>(g_dp);
			BYTE b_byte = static_cast<UINT8>(b_dp);
			UINT32 color_u32 = (((UINT32)r_byte) << 0x10) + (((UINT32)g_byte) << 0x8) + (UINT32)b_byte;
			dib_bits[i * nWidth + j] = color_u32;

		}

	}

	return;

}

void Rasterizer::Raster_Barycentric_Floating_Point(std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer) {

	FILE* log_file = nullptr;
	if (!logged) log_file = fopen("Raster_Barycentric_Floating_Point.log", "wb");
	// if (!logged) fprintf(log_file, "Rasterizer::Raster_Barycentric_Floating_Point().\n");

	for (int k = 0; k < indexbuffer.size(); k += 3) {

		rst::point<double>& A_pos = vertexbuffer[indexbuffer[k+0]].position;
		rst::point<double>& B_pos = vertexbuffer[indexbuffer[k+1]].position;
		rst::point<double>& C_pos = vertexbuffer[indexbuffer[k+2]].position;

		rst::rectangle<double> bounding_box = triangle_bounding_box(A_pos, B_pos, C_pos);

		// -- Edge function
		// -- E_xy(p) = (vx_y - vy_y)*p_x + (vy_x - vx_x)*p_y + (vx_x*vy_y - vx_y*vy_x)
		// -- E_xy(p) = A_xy*p_x + B_xy*p_y + C_xy

		// double A01 = A_pos.y - B_pos.y; 
		// double A12 = B_pos.y - C_pos.y; 
		// double A20 = C_pos.y - A_pos.y; 

		// double B01 = B_pos.x - A_pos.x;
		// double B12 = C_pos.x - B_pos.x;
		// double B20 = A_pos.x - C_pos.x;

		double A01 = B_pos.y - A_pos.y; 
		double A12 = C_pos.y - B_pos.y; 
		double A20 = A_pos.y - C_pos.y; 

		double B01 = A_pos.x - B_pos.x;
		double B12 = B_pos.x - C_pos.x;
		double B20 = C_pos.x - A_pos.x;

		// -- Barycentric coordinates at min/min
		// -- Min/Min at rounded up coordinates
		rst::point<double> min_point = {bounding_box.left, bounding_box.top, 0, 0};

		INT64 bounding_box_int_i = 0;
		INT64 bounding_box_int_j = 0;
		{		
			rst::point<INT64> de_norm_bounding_box = {};
			rst::point<double> norm_bounding_box = { bounding_box.left, bounding_box.top, 0, 0 };
			de_normalize(norm_bounding_box, de_norm_bounding_box, nWidth / 2, nHeight / 2);
			bounding_box_int_j = de_norm_bounding_box.x;
			bounding_box_int_i = de_norm_bounding_box.y;
		}

		if (!logged) {
			// fprintf(log_file, "Bounding box: %f %f %f %f.\n", bounding_box.left, bounding_box.top, bounding_box.right, bounding_box.bottom);
			{
				rst::point<INT64> de_norm_bounding_box = {};
				rst::point<double> norm_bounding_box = { bounding_box.left, bounding_box.top, 0, 0 };
				de_normalize(norm_bounding_box, de_norm_bounding_box, nWidth / 2, nHeight / 2);
				fprintf(log_file, "Bounding box: %I64d %I64d", de_norm_bounding_box.x, de_norm_bounding_box.y);
			}
			{
				rst::point<INT64> de_norm_bounding_box = {};
				rst::point<double> norm_bounding_box = { bounding_box.right, bounding_box.bottom, 0, 0 };
				de_normalize(norm_bounding_box, de_norm_bounding_box, nWidth / 2, nHeight / 2);
				fprintf(log_file, " %I64d %I64d.\n", de_norm_bounding_box.x, de_norm_bounding_box.y);
			}
			fprintf(log_file, "Barycentric coordinates: A01: %f, A12: %f, A20: %f, B01: %f, B12: %f, B20: %f.\n",
			 A01, A12, A20, B01, B12, B20);
		}

		double pixel_len = 2.0 / static_cast<double>(nWidth);
		double pixel_centre_offset = pixel_len / 2;
		double pixel_centre_point_x = ((double)(int)(bounding_box.left / pixel_centre_offset) * pixel_centre_offset);
		double pixel_centre_point_y = ((double)(int)(bounding_box.top / pixel_centre_offset) * pixel_centre_offset);
		// rst::point<double> min_point_centre = {bounding_box.left, bounding_box.top, 0, 0};
		rst::point<double> min_point_centre = {pixel_centre_point_x, pixel_centre_point_y, 0, 0};
		double bounding_box_width = bounding_box.right - bounding_box.left;
		double bounding_box_height = bounding_box.top - bounding_box.bottom;	

		double edge_min_0 = rst::edge(C_pos, B_pos, min_point);
		double edge_min_1 = rst::edge(A_pos, C_pos, min_point);
		double edge_min_2 = rst::edge(B_pos, A_pos, min_point);
		double area = rst::edge(A_pos, C_pos, B_pos);
		// double bias0 = rst::is_top_left(B_pos, C_pos) ? 0 : -pixel_centre_offset;
		// double bias1 = rst::is_top_left(C_pos, A_pos) ? 0 : -pixel_centre_offset;
		// double bias2 = rst::is_top_left(A_pos, B_pos) ? 0 : -pixel_centre_offset;
		double bias0 = 0;
		double bias1 = 0;
		double bias2 = 0;

		if (!logged) fprintf(log_file, "Bias: %f, %f, %f.\n", bias0, bias1, bias2);

		for (double i = min_point_centre.y; i >= min_point_centre.y - bounding_box_height; i -= pixel_len) {

			double edge_0 = edge_min_0;
			double edge_1 = edge_min_1;
			double edge_2 = edge_min_2;

			{		
				rst::point<INT64> de_norm_bounding_box = {};
				rst::point<double> norm_bounding_box = { bounding_box.left, bounding_box.top, 0, 0 };
				de_normalize(norm_bounding_box, de_norm_bounding_box, nWidth / 2, nHeight / 2);
				bounding_box_int_j = de_norm_bounding_box.x;
			}

			for (double j = min_point_centre.x; j <= min_point_centre.x + bounding_box_width; j += pixel_len) {

				if (!logged) {
					{
						rst::point<INT64> de_norm_bounding_box = {};
						rst::point<double> norm_bounding_box = { j, i, 0, 0 };
						de_normalize(norm_bounding_box, de_norm_bounding_box, nWidth / 2, nHeight / 2);
						// fprintf(log_file, "Candidate pixel: i: %I64d, j: %I64d, edge0: %f, edge1: %f, edge2: %f.\n", de_norm_bounding_box.y, de_norm_bounding_box.x, edge_0, edge_1, edge_2); 
						// fprintf(log_file, "Int pixel i: %I64d j: %I64d.\n", bounding_box_int_i, bounding_box_int_j);	
						// fprintf(log_file, "Candidate pixel: i: %f, j: %f, edge0: %f, edge1: %f, edge2: %f.\n", i, j, edge_0, edge_1, edge_2); 
						{
							rst::point<double> barycentric_coefs(edge_0, edge_1, edge_2, 0);
							if (area != 0) {
								barycentric_coefs.x /= 2.0 * static_cast<double>(area);
								barycentric_coefs.y /= 2.0 * static_cast<double>(area);
								barycentric_coefs.z /= 2.0 * static_cast<double>(area);							
							}
							else {
								barycentric_coefs.x = 1.0;	
								barycentric_coefs.y = 0.0;	
								barycentric_coefs.z = 0.0;	
							}
							// fprintf(log_file, "Barycentric coords: %f, %f, %f.\n", barycentric_coefs.x, barycentric_coefs.y, barycentric_coefs.z);
						}
					}
				}
				if ((edge_0 + bias0) >= -0.0001101111 && (edge_1 + bias1) >= -0.0001101111 && (edge_2 + bias2) >= -0.0001101111) {

					if (!logged) {
						{
							rst::point<INT64> de_norm_bounding_box = {};
							rst::point<double> norm_bounding_box = { j, i, 0, 0 };
							de_normalize(norm_bounding_box, de_norm_bounding_box, nWidth / 2, nHeight / 2);
							// fprintf(log_file, "Rendered pixel i: %I64d j: %I64d.\n", de_norm_bounding_box.y, de_norm_bounding_box.x);

						}
					}

					rst::point<double> barycentric_coefs(edge_0, edge_1, edge_2, 0);
					if (area != 0) {
						barycentric_coefs.x /= 2.0 * static_cast<double>(area);
						barycentric_coefs.y /= 2.0 * static_cast<double>(area);
						barycentric_coefs.z /= 2.0 * static_cast<double>(area);							
					}
					else {
						barycentric_coefs.x = 1.0;	
						barycentric_coefs.y = 0.0;	
						barycentric_coefs.z = 0.0;	
					}	

					double w0 = vertexbuffer[indexbuffer[k+0]].position.w;
					double w1 = vertexbuffer[indexbuffer[k+1]].position.w;
					double w2 = vertexbuffer[indexbuffer[k+2]].position.w;
					
					double l0 = barycentric_coefs.x / w0;
					double l1 = barycentric_coefs.y / w1;
					double l2 = barycentric_coefs.z / w2;

					double lsum = l0 + l1 + l2;

					l0 /= lsum;
					l1 /= lsum;
					l2 /= lsum;

					double z = l0 * vertexbuffer[indexbuffer[k+0]].position.z + l1 * vertexbuffer[indexbuffer[k+1]].position.z + l2 * vertexbuffer[indexbuffer[k+2]].position.z; 

					rst::point<INT64> de_norm_bounding_box = {};
					rst::point<double> norm_bounding_box = { j, i, 0, 0 };
					de_normalize(norm_bounding_box, de_norm_bounding_box, nWidth / 2, nHeight / 2);
					// -- Compare with depth buffer
					if (depth_buffer[bounding_box_int_i * nWidth + bounding_box_int_j] <= z) {
					
					}
					else {

						depth_buffer[bounding_box_int_i * nWidth + bounding_box_int_j] = z;

						rst::vertex<double> pixel = {};
						pixel.texture = 
							l0 * vertexbuffer[indexbuffer[k+0]].texture + 
							l1 * vertexbuffer[indexbuffer[k+1]].texture + 
							l2 * vertexbuffer[indexbuffer[k+2]].texture;
						pixel.color = 
							l0 * vertexbuffer[indexbuffer[k+0]].color + 
							l1 * vertexbuffer[indexbuffer[k+1]].color + 
							l2 * vertexbuffer[indexbuffer[k+2]].color;

						this->onpixel(pixel);

						BYTE r_byte = pixel.color.x;
						BYTE g_byte = pixel.color.y;
						BYTE b_byte = pixel.color.z;
						UINT32 color_u32 = (((UINT32)r_byte) << 0x10) + (((UINT32)g_byte) << 0x8) + (UINT32)b_byte;							

						dib_bits[bounding_box_int_i * nWidth + bounding_box_int_j] = color_u32;
					}

				}
			
				edge_0 += A12 * pixel_len;
				edge_1 += A20 * pixel_len;
				edge_2 += A01 * pixel_len;
				bounding_box_int_j++;

			}

			edge_min_0 -= B12 * pixel_len;
			edge_min_1 -= B20 * pixel_len;
			edge_min_2 -= B01 * pixel_len;
			bounding_box_int_i++;

		}

	}

	if (!logged) fclose(log_file);
	if (!logged) logged = true;

	return;

}

void Rasterizer::Raster_SIMD(std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer) {

	FILE* log_file = nullptr;
	if (!logged) log_file = fopen("log", "wb");
	if (!logged) fprintf(log_file, "Rasterizer::Raster_SIMD().\n");

	// -- Loop triangles
	for (int k = 0; k < indexbuffer.size(); k += 3) {
	
		rst::point<double>& A_pos = vertexbuffer[indexbuffer[k+0]].position;
		rst::point<double>& B_pos = vertexbuffer[indexbuffer[k+2]].position;
		rst::point<double>& C_pos = vertexbuffer[indexbuffer[k+1]].position;
		rst::rectangle<double> bounding_box = triangle_bounding_box(A_pos, B_pos, C_pos);
		rst::point<double> min_point = {bounding_box.left, bounding_box.top, 0, 0};

		// -- Find pixel center less than top-left of bounding box
		double pixel_len = 2.0 / static_cast<double>(nWidth);
		double pixel_centre_offset = pixel_len / 2;
		double pixel_centre_point_x = ((double)(int)(bounding_box.left / pixel_centre_offset) * pixel_centre_offset);
		double pixel_centre_point_y = ((double)(int)(bounding_box.top / pixel_centre_offset) * pixel_centre_offset);
		rst::point<double> min_point_centre = {pixel_centre_point_x, pixel_centre_point_y, 0, 0};

		double step_rate_x = 4.0;
		double step_rate_y = 1.0;

		rst::edge_simd AB_edge(A_pos, B_pos, min_point_centre, step_rate_x, step_rate_y);
		rst::edge_simd BC_edge(B_pos, C_pos, min_point_centre, step_rate_x, step_rate_y);
		rst::edge_simd CA_edge(C_pos, A_pos, min_point_centre, step_rate_x, step_rate_y);

		double bounding_box_width = bounding_box.right - bounding_box.left;
		double bounding_box_height = bounding_box.top - bounding_box.bottom;

		if (!logged) {
			fprintf(log_file, "Bounding box: %f %f %f %f.\n", bounding_box.left, bounding_box.top, bounding_box.right, bounding_box.bottom);
			{
				rst::point<INT64> de_norm_bounding_box = {};
				rst::point<double> norm_bounding_box = { bounding_box.left, bounding_box.top, 0, 0 };
				de_normalize(norm_bounding_box, de_norm_bounding_box, nWidth / 2, nHeight / 2);
				fprintf(log_file, "Bounding box: %I64d %I64d", de_norm_bounding_box.x, de_norm_bounding_box.y);
			}
			{
				rst::point<INT64> de_norm_bounding_box = {};
				rst::point<double> norm_bounding_box = { bounding_box.right, bounding_box.bottom, 0, 0 };
				de_normalize(norm_bounding_box, de_norm_bounding_box, nWidth / 2, nHeight / 2);
				fprintf(log_file, " %I64d %I64d.\n", de_norm_bounding_box.x, de_norm_bounding_box.y);
			}
			
			// fprintf(log_file, "Barycentric coordinates: A01: %I64d, A12: %I64d, A20: %I64d, B01: %I64d, B12: %I64d, B20: %I64d.\n",
			//  A01, A12, A20, B01, B12, B20);
		
			// fprintf(log_file, "Pixel len: %f.\n", pixel_len);
			// fprintf(log_file, "Pixel centre offset: %f.\n", pixel_centre_offset);
			// fprintf(log_file, "Bounding box pixel centre offset: %f %f.\n", (double)(int)(bounding_box.left / pixel_centre_offset), bounding_box.top / pixel_centre_offset);
			// fprintf(log_file, "Min point centre: %f, %f.\n", min_point_centre.x, min_point_centre.y);
			fprintf(log_file, "Bounding box height: %f.\n", bounding_box_height);
			fprintf(log_file, "Bounding box width: %f.\n", bounding_box_width);
		}

		for (double i = min_point_centre.y; i <= min_point_centre.y + bounding_box_height; i += step_rate_y * pixel_len) {
			for (double j = min_point_centre.x; j <= min_point_centre.x + bounding_box_width; j += step_rate_x * pixel_len) {

				__m256d AB_edge_val = AB_edge.get();
				__m256d BC_edge_val = BC_edge.get();
				__m256d CA_edge_val = CA_edge.get();

				AB_edge_val = _mm256_or_pd(AB_edge_val, BC_edge_val);
				AB_edge_val = _mm256_or_pd(AB_edge_val, CA_edge_val);

				__m256d positive_bitmask = _mm256_cmp_pd(AB_edge_val, rst::edge_simd::load_dp(0), _CMP_GE_OQ);
				UINT32 moved_positive_bitmask = _mm256_movemask_pd(positive_bitmask);
				if (moved_positive_bitmask) {
					if (!logged) {fprintf(log_file, "boom.\n");}
					double w0[4];
					double w1[4];
					double w2[4];
					_mm256_storeu_pd(&w0[0], AB_edge_val);
					_mm256_storeu_pd(&w1[0], BC_edge_val);
					_mm256_storeu_pd(&w2[0], CA_edge_val);
					if (moved_positive_bitmask & 0x1) {
						Raster_SIMD_Pixel(i + pixel_len * 0 - pixel_centre_offset, j - pixel_centre_offset, vertexbuffer, indexbuffer, k, w0[0], w1[0], w2[0]);
					}
					if (moved_positive_bitmask & 0x2) {
						Raster_SIMD_Pixel(i + pixel_len * 1 - pixel_centre_offset, j - pixel_centre_offset, vertexbuffer, indexbuffer, k, w0[1], w1[1], w2[1]);
					}
					if (moved_positive_bitmask & 0x4) {
						Raster_SIMD_Pixel(i + pixel_len * 2 - pixel_centre_offset, j - pixel_centre_offset, vertexbuffer, indexbuffer, k, w0[2], w1[2], w2[2]);
					}
					if (moved_positive_bitmask & 0x8) {
						Raster_SIMD_Pixel(i + pixel_len * 3 - pixel_centre_offset, j - pixel_centre_offset, vertexbuffer, indexbuffer, k, w0[3], w1[3], w2[3]);
					}
				}

				AB_edge.step_x();
				BC_edge.step_x();
				CA_edge.step_x();
			}

			AB_edge.step_y();
			BC_edge.step_y();
			CA_edge.step_y();
		}
	}

	if (!logged) fclose(log_file);
	if (!logged) logged = true;

	return;

}

void Rasterizer::Raster_SIMD_Pixel(double i, double j, std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer, int k, double w0, double w1, double w2) {

	double area = rst::edge(vertexbuffer[indexbuffer[k+0]].position, vertexbuffer[indexbuffer[k+1]].position, vertexbuffer[indexbuffer[k+2]].position);
	rst::point<double> barycentric_coefs(w0, w1, w2, 0);
	if (area != 0) {
		barycentric_coefs.x /= 2.0 * static_cast<double>(area);
		barycentric_coefs.y /= 2.0 * static_cast<double>(area);
		barycentric_coefs.z /= 2.0 * static_cast<double>(area);							
	}
	else {
		barycentric_coefs.x = 1.0;	
		barycentric_coefs.y = 0.0;	
		barycentric_coefs.z = 0.0;	
	}

	double w_0 = vertexbuffer[indexbuffer[k+0]].position.w;
	double w_1 = vertexbuffer[indexbuffer[k+1]].position.w;
	double w_2 = vertexbuffer[indexbuffer[k+2]].position.w;
	
	double l0 = barycentric_coefs.x / w_0;
	double l1 = barycentric_coefs.y / w_1;
	double l2 = barycentric_coefs.z / w_2;

	double lsum = l0 + l1 + l2;

	l0 /= lsum;
	l1 /= lsum;
	l2 /= lsum;

	rst::point<INT64> position = {};
	rst::point<double> position_dp = {j, i, 0.0, 0.0};

	de_normalize(position_dp, position, nWidth / 2, nHeight / 2);

	double z = l0 * vertexbuffer[indexbuffer[k+0]].position.z + l1 * vertexbuffer[indexbuffer[k+1]].position.z + l2 * vertexbuffer[indexbuffer[k+2]].position.z; 

	// -- Compare with depth buffer
	if (depth_buffer[position.y * nWidth + position.x] < z) {
	
	}
	else {

		depth_buffer[position.y * nWidth + position.x] = z;

		rst::vertex<double> pixel = {};
		pixel.texture = 
			l0 * vertexbuffer[indexbuffer[k+0]].texture + 
			l1 * vertexbuffer[indexbuffer[k+1]].texture + 
			l2 * vertexbuffer[indexbuffer[k+2]].texture;
		pixel.color = 
			l0 * vertexbuffer[indexbuffer[k+0]].color + 
			l1 * vertexbuffer[indexbuffer[k+1]].color + 
			l2 * vertexbuffer[indexbuffer[k+2]].color;

		this->onpixel(pixel);

		BYTE r_byte = pixel.color.x;
		BYTE g_byte = pixel.color.y;
		BYTE b_byte = pixel.color.z;
		UINT32 color_u32 = (((UINT32)r_byte) << 0x10) + (((UINT32)g_byte) << 0x8) + (UINT32)b_byte;							

		dib_bits[position.y * nWidth + position.x] = color_u32;

	}

	return;

}

void Rasterizer::Clip(std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer) {

	for (rst::homogenous_plane& plane : clipping_planes) {

		std::vector<int> clipped_indexbuffer;

		for (int i = 0; i < indexbuffer.size(); i += 3) {

			bool isInside0 = distance_to_plane(plane, vertexbuffer[indexbuffer[i+0]].position);			
			bool isInside1 = distance_to_plane(plane, vertexbuffer[indexbuffer[i+1]].position);
			bool isInside2 = distance_to_plane(plane, vertexbuffer[indexbuffer[i+2]].position);	
	
			// -- Triangle is inside the bouding box
			if (isInside0 == true && isInside1 == true && isInside2 == true) {
				clipped_indexbuffer.push_back(indexbuffer[i+0]);
				clipped_indexbuffer.push_back(indexbuffer[i+1]);
				clipped_indexbuffer.push_back(indexbuffer[i+2]);
				continue;
			}

			// -- Triangle is outside the bounding box 
			if (isInside0 == false && isInside1 == false && isInside2 == false) {
				continue;
			}

			// -- One of the triangles vertices is inside the bounding box
			if ((isInside0 == true && isInside1 == false && isInside2 == false) ||
				(isInside1 == true && isInside0 == false && isInside2 == false) ||
				(isInside2 == true && isInside0 == false && isInside1 == false)) {

				double t1 = 0.0;
				double t2 = 0.0;
				rst::vertex<double> A;
				rst::vertex<double> B;

				if (isInside0 == true) {
					t1 = signed_distance_ratio(vertexbuffer[indexbuffer[i+1]].position, vertexbuffer[indexbuffer[i+0]].position, plane);
					t2 = signed_distance_ratio(vertexbuffer[indexbuffer[i+2]].position, vertexbuffer[indexbuffer[i+0]].position, plane);
					B = larp(vertexbuffer[indexbuffer[i+1]], vertexbuffer[indexbuffer[i+0]], t1);
					A = larp(vertexbuffer[indexbuffer[i+2]], vertexbuffer[indexbuffer[i+0]], t2);
				}
				else if (isInside1 == true) {
					t1 = signed_distance_ratio(vertexbuffer[indexbuffer[i+2]].position, vertexbuffer[indexbuffer[i+1]].position, plane);
					t2 = signed_distance_ratio(vertexbuffer[indexbuffer[i+0]].position, vertexbuffer[indexbuffer[i+1]].position, plane);
					B = larp(vertexbuffer[indexbuffer[i+2]], vertexbuffer[indexbuffer[i+1]], t1);
					A = larp(vertexbuffer[indexbuffer[i+0]], vertexbuffer[indexbuffer[i+1]], t2);
				}
				else if (isInside2 == true) {
					t1 = signed_distance_ratio(vertexbuffer[indexbuffer[i+0]].position, vertexbuffer[indexbuffer[i+2]].position, plane);
					t2 = signed_distance_ratio(vertexbuffer[indexbuffer[i+1]].position, vertexbuffer[indexbuffer[i+2]].position, plane);
					B = larp(vertexbuffer[indexbuffer[i+0]], vertexbuffer[indexbuffer[i+2]], t1);
					A = larp(vertexbuffer[indexbuffer[i+1]], vertexbuffer[indexbuffer[i+2]], t2);
				}

				vertexbuffer.push_back(B);
				vertexbuffer.push_back(A);
				
				if (isInside0 == true) clipped_indexbuffer.push_back(indexbuffer[i+0]);
				if (isInside1 == true) clipped_indexbuffer.push_back(indexbuffer[i+1]);
				if (isInside2 == true) clipped_indexbuffer.push_back(indexbuffer[i+2]);
				
				clipped_indexbuffer.push_back(vertexbuffer.size() - 2);
				clipped_indexbuffer.push_back(vertexbuffer.size() - 1);
				continue;
			}

			// -- Two of the triangles vertices is inside the bounding box
			if ((isInside0 == true && isInside1 == true && isInside2 == false) ||
				(isInside0 == true && isInside2 == true && isInside1 == false) ||
				(isInside1 == true && isInside2 == true && isInside0 == false)) {

				double t1 = 0.0;
				double t2 = 0.0;
				rst::vertex<double> A;
				rst::vertex<double> B;

				if (isInside0 == false) {
					t1 = signed_distance_ratio(vertexbuffer[indexbuffer[i+0]].position, vertexbuffer[indexbuffer[i+1]].position, plane);
					t2 = signed_distance_ratio(vertexbuffer[indexbuffer[i+0]].position, vertexbuffer[indexbuffer[i+2]].position, plane);
					A = larp(vertexbuffer[indexbuffer[i+0]], vertexbuffer[indexbuffer[i+1]], t1);
					B = larp(vertexbuffer[indexbuffer[i+0]], vertexbuffer[indexbuffer[i+2]], t2);
				}
				else if (isInside1 == false) {
					t1 = signed_distance_ratio(vertexbuffer[indexbuffer[i+1]].position, vertexbuffer[indexbuffer[i+2]].position, plane);
					t2 = signed_distance_ratio(vertexbuffer[indexbuffer[i+1]].position, vertexbuffer[indexbuffer[i+0]].position, plane);
					A = larp(vertexbuffer[indexbuffer[i+1]], vertexbuffer[indexbuffer[i+2]], t1);
					B = larp(vertexbuffer[indexbuffer[i+1]], vertexbuffer[indexbuffer[i+0]], t2);
				}
				else if (isInside2 == false) {
					t1 = signed_distance_ratio(vertexbuffer[indexbuffer[i+2]].position, vertexbuffer[indexbuffer[i+0]].position, plane);
					t2 = signed_distance_ratio(vertexbuffer[indexbuffer[i+2]].position, vertexbuffer[indexbuffer[i+1]].position, plane);
					A = larp(vertexbuffer[indexbuffer[i+2]], vertexbuffer[indexbuffer[i+0]], t1);
					B = larp(vertexbuffer[indexbuffer[i+2]], vertexbuffer[indexbuffer[i+1]], t2);
				}

				vertexbuffer.push_back(A);
				vertexbuffer.push_back(B);

				// -- Triangle 1: OLD1, OLD2, A
				// -- Triangle 2: OLD2, B, A

				if (isInside0 == false) {
					clipped_indexbuffer.push_back(indexbuffer[i+1]);
					clipped_indexbuffer.push_back(indexbuffer[i+2]);
					clipped_indexbuffer.push_back(vertexbuffer.size() - 2);
					
					clipped_indexbuffer.push_back(vertexbuffer.size() - 2);
					clipped_indexbuffer.push_back(indexbuffer[i+2]);
					clipped_indexbuffer.push_back(vertexbuffer.size() - 1);
				}
				else if (isInside1 == false) {				
					clipped_indexbuffer.push_back(indexbuffer[i+2]);
					clipped_indexbuffer.push_back(indexbuffer[i+0]);
					clipped_indexbuffer.push_back(vertexbuffer.size() - 2);
					
					clipped_indexbuffer.push_back(vertexbuffer.size() - 2);
					clipped_indexbuffer.push_back(indexbuffer[i+0]);
					clipped_indexbuffer.push_back(vertexbuffer.size() - 1);
				}
				else if (isInside2 == false) {
					clipped_indexbuffer.push_back(indexbuffer[i+0]);
					clipped_indexbuffer.push_back(indexbuffer[i+1]);
					clipped_indexbuffer.push_back(vertexbuffer.size() - 2);
					
					clipped_indexbuffer.push_back(vertexbuffer.size() - 2);
					clipped_indexbuffer.push_back(indexbuffer[i+1]);
					clipped_indexbuffer.push_back(vertexbuffer.size() - 1);
				}

				continue;
			}
		
		}

		indexbuffer = std::move(clipped_indexbuffer);
	
	}

	return;

}

void Rasterizer::ClearBackground() {

	memset(dib_bits, 0x00, nHeight * nWidth * 4);
	return;

}

void Rasterizer::ClearDepthBuffer() {

	for (int i = 0; i < nHeight * nWidth; ++i) depth_buffer[i] = 1.0;
	return;

}

void Rasterizer::AddMesh(asset::Mesh* mesh) {

	pass_list.push_back(mesh);	
	return;

}

void Rasterizer::AddOnVertex(std::function<void(rst::vertex<double>&)> onvertex) {

	this->onvertex = onvertex;
	return;

}

void Rasterizer::AddOnVertexInstanced(std::function<void(rst::vertex<double>&, void*)> onvertexinstanced) {

	this->onvertexinstanced = onvertexinstanced;
	return;

}


void Rasterizer::AddOnPixel(std::function<void(rst::vertex<double>&)> onpixel) {
	
	this->onpixel = onpixel;
	return;

}

void Rasterizer::SetWindingOrder(RASTERIZER_WINDING_ORDER winding_order) {

	this->winding_order = winding_order;
	return;

}

void Rasterizer::SetRasterizerAlgorithm(RASTERIZER_ALGORITHM rasterizer_algorithm) {

	this->rasterizer_algorithm = rasterizer_algorithm;
	return;

}

void Rasterizer::Draw() {

	PIXScopedEvent(PIX_COLOR(0x00, 0xFF, 0xFF), "Rasterizer::Draw()");
	
	for (asset::Mesh* pass : pass_list) {

		std::vector<rst::vertex<double>> vertexbuffer = pass->CloneVertexBuffer_vec();
		std::vector<int> indexbuffer = pass->CloneIndexBuffer_vec();

		for (int i = 0; i < vertexbuffer.size(); ++i) {
			this->onvertex(vertexbuffer[i]);
		}

		Clip(vertexbuffer, indexbuffer);

		// -- Divide by w
		for (rst::vertex<double>& vertex : vertexbuffer) {
			vertex.position.x /= vertex.position.w;
			vertex.position.y /= vertex.position.w;
			vertex.position.z /= vertex.position.w;
		}

		// -- Rasterizer renders triangles with positive determinant
		// -- Left-handed coordinate system -> Clockwise rotation
		// -- Right-handed coordinate system -> Counter-clockwise rotation
		// -- After divide by w the coordinate system is left handed -> clockwise triangles will be drawn

		// -- Clockwise
		if (winding_order == RASTERIZER_WINDING_ORDER::CW) {

		}
		// -- Counter-clockwise
		if (winding_order == RASTERIZER_WINDING_ORDER::CCW) {
			for (int i = 0; i < indexbuffer.size(); i += 3) {
				std::swap(indexbuffer[i+1], indexbuffer[i+2]);
			}
		}
		// -- All winding orders allowed
		else if (winding_order == RASTERIZER_WINDING_ORDER::NONE) {
			
			for (int i = 0; i < indexbuffer.size(); i += 3) {
				if (edge(vertexbuffer[indexbuffer[i+0]].position, vertexbuffer[indexbuffer[i+1]].position, vertexbuffer[indexbuffer[i+2]].position) > 0) {
					std::swap(indexbuffer[i+1], indexbuffer[i+2]);
				}			
			}
		
		}

		{
			std::lock_guard<std::mutex> lg(log_mutex);
			
			if (rasterizer_algorithm == RASTERIZER_ALGORITHM::NAIVE) {
				Raster(vertexbuffer, indexbuffer);
			}
			else if (rasterizer_algorithm == RASTERIZER_ALGORITHM::BARYCENTRIC) {
				Raster_Barycentric(vertexbuffer, indexbuffer);
			}
			else if (rasterizer_algorithm == RASTERIZER_ALGORITHM::SIMD) {
				Raster_SIMD(vertexbuffer, indexbuffer);
			}
			else if (rasterizer_algorithm == RASTERIZER_ALGORITHM::BARYCENTRIC_FLOATING_POINT) {
				Raster_Barycentric_Floating_Point(vertexbuffer, indexbuffer);
			}
			else if (rasterizer_algorithm == RASTERIZER_ALGORITHM::SIMD_INTEGER) {
				Raster_SIMD_Integer(vertexbuffer, indexbuffer);
			}

		}


	}

	pass_list.clear();

	return;

}

void Rasterizer::DrawInstanced(size_t count) {
	
	PIXScopedEvent(PIX_COLOR(0x00, 0xFF, 0xFF), "Rasterizer::DrawInstanced()");
	
	for (asset::Mesh* pass : pass_list) {

		for (size_t k = 0; k < count; k++) {

			std::vector<rst::vertex<double>> vertexbuffer = pass->CloneVertexBuffer_vec();
			std::vector<int> indexbuffer = pass->CloneIndexBuffer_vec();

			for (int i = 0; i < vertexbuffer.size(); ++i) {
				this->onvertexinstanced(vertexbuffer[i], reinterpret_cast<char*>(instance_buffer) + (instance_buffer_alignment * k));
			}

			Clip(vertexbuffer, indexbuffer);

			// -- Divide by w
			for (rst::vertex<double>& vertex : vertexbuffer) {
				vertex.position.x /= vertex.position.w;
				vertex.position.y /= vertex.position.w;
				vertex.position.z /= vertex.position.w;
			}

			// -- Rasterizer renders triangles with positive determinant
			// -- Left-handed coordinate system -> Clockwise rotation
			// -- Right-handed coordinate system -> Counter-clockwise rotation
			// -- After divide by w the coordinate system is left handed -> clockwise triangles will be drawn

			// -- Clockwise
			if (winding_order == RASTERIZER_WINDING_ORDER::CW) {

			}
			// -- Counter-clockwise
			if (winding_order == RASTERIZER_WINDING_ORDER::CCW) {
				for (int i = 0; i < indexbuffer.size(); i += 3) {
					std::swap(indexbuffer[i+1], indexbuffer[i+2]);
				}
			}
			// -- All winding orders allowed
			else if (winding_order == RASTERIZER_WINDING_ORDER::NONE) {
				
				for (int i = 0; i < indexbuffer.size(); i += 3) {
					if (edge(vertexbuffer[indexbuffer[i+0]].position, vertexbuffer[indexbuffer[i+1]].position, vertexbuffer[indexbuffer[i+2]].position) > 0) {
						std::swap(indexbuffer[i+1], indexbuffer[i+2]);
					}			
				}
			
			}

			{
				std::lock_guard<std::mutex> lg(log_mutex);
				
				if (rasterizer_algorithm == RASTERIZER_ALGORITHM::NAIVE) {
					Raster(vertexbuffer, indexbuffer);
				}
				else if (rasterizer_algorithm == RASTERIZER_ALGORITHM::BARYCENTRIC) {
					Raster_Barycentric(vertexbuffer, indexbuffer);
				}
				else if (rasterizer_algorithm == RASTERIZER_ALGORITHM::SIMD) {
					Raster_SIMD(vertexbuffer, indexbuffer);
				}
				else if (rasterizer_algorithm == RASTERIZER_ALGORITHM::BARYCENTRIC_FLOATING_POINT) {
					Raster_Barycentric_Floating_Point(vertexbuffer, indexbuffer);
				}	
				else if (rasterizer_algorithm == RASTERIZER_ALGORITHM::SIMD_INTEGER) {
					Raster_SIMD_Integer(vertexbuffer, indexbuffer);
				}

			}

		}

	}

	pass_list.clear();

	return;

}

void Rasterizer::SaveRender(const std::string& path) {

	{
		std::lock_guard<std::mutex> lg(bitmap_mutex);
		BitBlt(save_dc, 0, 0, nWidth, nHeight, off_dc, 0, 0, SRCCOPY);		
		WriteBitmap(path, save_bitmap);		
	}

	return;

}

void Rasterizer::SaveDepth(const std::string& path) {

	{
		std::lock_guard<std::mutex> lg(bitmap_mutex);
		WriteBitmap(path, depth_bitmap);		
	}

}

UINT32 Rasterizer::SampleTexture(double u, double v, asset::Tex& texture) {

	rst::point<int> uv_screen_coords = {0, 0, 0, 0};
	rst::point<double> uv_double = {u, v, 0.0, 0.0};
	rst::de_normalize_tex(uv_double, uv_screen_coords, texture.GetWidth(), texture.GetHeight());

	UINT32 sample = texture.GetValue(uv_screen_coords.y * texture.GetWidth() + uv_screen_coords.x);
	
	return sample; 

}

UINT32 Rasterizer::SampleTexture(double u, double v, RASTERIZER_TEXTURE texture) {

	rst::point<int> uv_screen_coords = {0, 0, 0, 0};
	rst::point<double> uv_double = {u, v, 0.0, 0.0};
	rst::de_normalize_tex(uv_double, uv_screen_coords, this->default_texture.GetWidth(), this->default_texture.GetHeight());

	UINT32 sample = this->default_texture.GetValue(uv_screen_coords.y * this->default_texture.GetWidth() + uv_screen_coords.x);

	return sample;

}

void Rasterizer::LogRasterizer() {

	{
		std::lock_guard<std::mutex> lg(log_mutex);
		logged = false;
	}

	return;

}

void Rasterizer::SetBlend(RASTERIZER_BLEND rasterizer_blend) {

	this->rasterizer_blend = rasterizer_blend;
	return;

}

void Rasterizer::SetBlendCoefficient(double bc) {

	this->blend_coefficient = bc;
	return;

}

void Rasterizer::AddInstanceBuffer(void* instance_ptr, size_t alignment) {

	this->instance_buffer = instance_ptr;
	this->instance_buffer_alignment = alignment;
	return;

}
		
