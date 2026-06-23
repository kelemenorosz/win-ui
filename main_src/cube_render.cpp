
#include "cube_render.h"
#include "asset.h"
#include <numbers>
#include "bitmap.h"

CubeRender::CubeRender(LPCWSTR windowName, int x, int y, int nWidth, int nHeight) : Rasterizer(windowName, x, y, nWidth, nHeight),
	cube(L"./mesh/crate.obj"), cube_rotation(0.0), cube_distance(0.0), depth_texture("./tex/earthmap10k.bmp"),
	sphere_cylindrical(L"./mesh/sphere_cylinder.obj"), white_texture("./tex/white_tex.bmp"), rasterizer_algorithm(RASTERIZER_ALGORITHM::SIMD_INTEGER),
	crate_texture("./tex/crate_tex.bmp"), camera_panning_running(false), cursor_clipped(false) {

	cube_position_x = 0.0;
	cube_position_y = 0.0;
	cube_position_z = 0.0;
	camera_rotation = 0.0;
	camera_position_x = 0.0;
	camera_position_y = 0.0;
	camera_position_z = 10.0;
	camera_target_x = 0.0;
	camera_target_y = 0.0;
	camera_target_z = -1.0;
	mouse_x = -1;
	mouse_y = -1;
	camera_target.set({0.0, 0.0, -1.0});
	camera_position.set({0.0, 0.0, 10.0});
	camera_up.set({0.0, 1.0, 0.0});

	rst::point<double> test_p = {1.0, 1.0, -1.1, 0.0};
	printf("Point before perspective projection: %f %f %f %f.\n", test_p.x, test_p.y, test_p.z, test_p.w);
	rst::perspective_fix(test_p, 90, 1.0, 1.0, 50.0);
	printf("Point after perspective projection: %f %f %f %f.\n", test_p.x, test_p.y, test_p.z, test_p.w);

	// std::random_device rd;
	// mersenne_twister.seed(rd());

	// color_len = cube.GetVertexBufferLen();
	// color_buf = new rst::color[color_len];
	// for (int i = 0; i < color_len; ++i) {
	// 	color_buf[i + 0] = rst::color(mersenne_twister() % 256, mersenne_twister() % 256, mersenne_twister() % 256);
	// }

	// pass_list.push_back(&cube);

	// triangle_vert.push_back(rst::vertex<double>(-1.0, 1.0, 1.0, 0.0, 255.0, 0.0));
	// triangle_vert.push_back(rst::vertex<double>(-1.0, -1.0, 1.0, 255.0, 0.0, 0.0));
	// triangle_vert.push_back(rst::vertex<double>(1.0, -1.0, 1.0, 0.0, 0.0, 255.0));
	triangle = asset::Mesh(triangle_vert, triangle_index);

	// texture_width = nWidth;
	// texture_height = nHeight;
	// checker_size = nHeight / 4;
	// texture = new UINT32[texture_width * texture_height]; 
	// for (int i = 0; i < texture_height; i++) {
	// 	for (int j = 0; j < texture_width; j++) {
	// 		bool isWhite = (i / checker_size + j / checker_size) % 2 ? true : false;
	// 		if (!isWhite) {
	// 			texture[i * texture_width + j] = 0xFF000000;
	// 		} 
	// 		else {
	// 			texture[i * texture_width + j] = 0x00FF00FF;
	// 		}
	// 	}
	// }

	// BITMAPINFO bitmapinfo = {};
	// bitmapinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	// bitmapinfo.bmiHeader.biWidth = nWidth;
	// bitmapinfo.bmiHeader.biHeight = -nHeight;
	// bitmapinfo.bmiHeader.biPlanes = 1;
	// bitmapinfo.bmiHeader.biBitCount = 32;
	// bitmapinfo.bmiHeader.biCompression = BI_RGB;
	// bitmapinfo.bmiHeader.biSizeImage = 0;
	// bitmapinfo.bmiHeader.biXPelsPerMeter = 0;
	// bitmapinfo.bmiHeader.biYPelsPerMeter = 0;
	// bitmapinfo.bmiHeader.biClrUsed = 0;
	// bitmapinfo.bmiHeader.biClrImportant = 0;
	// HDC window_dc = GetDC(hWnd);
	// HDC teture_dc = CreateCompatibleDC(0);
	// uint8_t* texture_bits = nullptr;
	// HBITMAP texture_bitmap = CreateDIBSection(window_dc, &bitmapinfo, DIB_RGB_COLORS, reinterpret_cast<void**>(&texture_bits), NULL, 0);
	// memcpy(texture_bits, texture, nWidth * nHeight * 4);
	// WriteBitmap("./default_tex.bmp", texture_bitmap);

	RAWINPUTDEVICE raw_input_device = {};
	raw_input_device.usUsagePage = 0x1;
	raw_input_device.usUsage = 0x2;
	raw_input_device.dwFlags = 0x0;
	raw_input_device.hwndTarget = NULL;

	BOOL ret = RegisterRawInputDevices(&raw_input_device, 1, sizeof(RAWINPUTDEVICE));
	if (ret == TRUE) {
		printf("RegisterRawInputDevices() success.\n");
	}

	StartRender();

	return;

}

CubeRender::~CubeRender() {

	// delete[] texture;
	// delete[] color_buf;

	return;

}

LRESULT CubeRender::OnRender() {

	// camera_position.set({camera_position_x, camera_position_y, camera_position_z});
	rst::vector<3> up;
	up.set({0.0, 1.0, 0.0});

	// rst::vector<3> temp_target;
	// temp_target.set({camera_target_x, camera_target_y, camera_target_z});

	// temp_target.print();
	// camera_target.print();

	// camera_target.normalize
	camera_view_matrix = rst::matrix<1, 1>::lookAt(camera_up, camera_position + camera_target, camera_position);
	// camera_rotation_matrix = rst::matrix<1, 1>::rotationX(rst::degree_to_radian(camera_rotation));
	// camera_view_matrix = camera_rotation_matrix * camera_view_matrix;
	// camera_view_matrix = camera_view_matrix;

	rst::vector<3> world_pos;
	world_pos.set({0.0, 0.0, 0.0});

	// rst::matrix<4, 4> rotation_matrix = rst::matrix<1, 1>::rotationZ(cube_position_x) * rst::matrix<1, 1>::rotationX(cube_position_y);
	rst::matrix<4, 4> rotation_matrix = rst::matrix<1, 1>::identity<4>();
	vertex_desc.world_matrix = rotation_matrix * rst::matrix<1, 1>::translation(world_pos);

	ClearBackground();
	ClearDepthBuffer();
	AddMesh(&sphere_cylindrical);
	AddOnVertex(std::bind(&CubeRender::OnVertex, this, reinterpret_cast<void*>(&vertex_desc), std::placeholders::_1));
	AddOnPixel(std::bind(&CubeRender::OnPixel, this, std::placeholders::_1));
	SetWindingOrder(RASTERIZER_WINDING_ORDER::NONE);
	SetRasterizerAlgorithm(rasterizer_algorithm);
	SetBlend(RASTERIZER_BLEND::NONE);
	Draw();

	vertex_desc.world_matrix = rst::matrix<1, 1>::identity<4>();
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++)
		instance_buffer[i * 8 + j].pos.set({-2.0 * j + 8, -2.0, -2.0 * i});
	}
	// instance_buffer[1].pos.set({-2.0, -2.0, 0.0});

	AddMesh(&cube);
	AddInstanceBuffer(reinterpret_cast<void*>(&instance_buffer), 64);
	AddOnVertexInstanced(std::bind(&CubeRender::OnVertexSecond, this, reinterpret_cast<void*>(&vertex_desc), std::placeholders::_1, std::placeholders::_2));
	AddOnPixel(std::bind(&CubeRender::OnPixelSecond, this, std::placeholders::_1));
	SetWindingOrder(RASTERIZER_WINDING_ORDER::NONE);
	SetRasterizerAlgorithm(rasterizer_algorithm);
	SetBlend(RASTERIZER_BLEND::NONE);
	DrawInstanced(64);

	// cube_rotation += 0.05;

	return S_OK;

}

void CubeRender::OnVertex(void* ptr, rst::vertex<double>& vertex) {
	
	Vertex_Desc* vertex_desc = reinterpret_cast<Vertex_Desc*>(ptr);

	rst::vector<4> final_position;
	final_position.set({vertex.position.x, vertex.position.y, vertex.position.z, 1.0});
	final_position = final_position * vertex_desc->world_matrix;
	final_position = final_position * camera_view_matrix;

	vertex.position.x = final_position._vector[0];
	vertex.position.y = final_position._vector[1];
	vertex.position.z = final_position._vector[2];

	rst::perspective_fix(vertex.position, 90, 1.0f, 0.1f, 10.0f);

	return;

}

void CubeRender::OnVertexSecond(void* ptr, rst::vertex<double>& vertex, void* instance_ptr) {

	Vertex_Desc* vertex_desc = reinterpret_cast<Vertex_Desc*>(ptr);
	Instance_Desc* instance_desc = reinterpret_cast<Instance_Desc*>(instance_ptr);

	rst::matrix<4, 4> translation_matrix = rst::matrix<1, 1>::translation(instance_desc->pos);

	rst::vector<4> final_position;
	final_position.set({vertex.position.x, vertex.position.y, vertex.position.z, 1.0});
	final_position = final_position * vertex_desc->world_matrix * translation_matrix;
	final_position = final_position * camera_view_matrix;

	vertex.position.x = final_position._vector[0];
	vertex.position.y = final_position._vector[1];
	vertex.position.z = final_position._vector[2];

	rst::perspective_fix(vertex.position, 90, 1.0f, 0.1f, 10.0f);

	return;

}

void CubeRender::OnPixel(rst::vertex<double>& pixel) {

	// UINT32 sample = SampleTexture(pixel.texture.x, pixel.texture.y, RASTERIZER_TEXTURE::NONE);
	UINT32 sample = SampleTexture(pixel.texture.x, pixel.texture.y, depth_texture);

	UINT8 red = static_cast<UINT8>(sample >> 16);
	UINT8 green = static_cast<UINT8>(sample >> 8);
	UINT8 blue = static_cast<UINT8>(sample);

	pixel.color.x = red;
	pixel.color.y = green;
	pixel.color.z = blue;

	return;

}

void CubeRender::OnPixelSecond(rst::vertex<double>& pixel) {

	// UINT32 sample = SampleTexture(pixel.texture.x, pixel.texture.y, white_texture);
	UINT32 sample = SampleTexture(pixel.texture.x, pixel.texture.y, RASTERIZER_TEXTURE::NONE);
	// UINT32 sample = SampleTexture(pixel.texture.x, pixel.texture.y, crate_texture);

	UINT8 red = static_cast<UINT8>(sample >> 16);
	UINT8 green = static_cast<UINT8>(sample >> 8);
	UINT8 blue = static_cast<UINT8>(sample);

	pixel.color.x = red;
	pixel.color.y = green;
	pixel.color.z = blue;

	return;

}

LRESULT CubeRender::OnMouseWheel(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	double delta = ((SHORT) HIWORD(wParam)) / (FLOAT) WHEEL_DELTA;

	if (delta > 0) {
		cube_position_z -= 0.1;
	}
	else {
		cube_position_z += 0.1;
	}

	return S_OK;

}

LRESULT CubeRender::OnMouseMove(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	int mouse_x = LOWORD(lParam);
	int mouse_y = HIWORD(lParam);

	// printf("CubeRender::OnMouseMove() mouse_x: %d, mouse_y: %d.\n", mouse_x, mouse_y);

	if (camera_panning_running == true) {

		if (this->mouse_x < 0 && this->mouse_y < 0) {
			this->mouse_x = mouse_x;
			this->mouse_y = mouse_y;
		}
		else {
			
			if (this->mouse_y - mouse_y > 0) {

				rst::vector<4> target = camera_target.extend<4>({1.0});
				rst::vector<3> temp_pos = camera_position;
				temp_pos.negate();
				// target = target * rst::matrix<1, 1>::translation(temp_pos);
				target = target * rst::matrix<1, 1>::rotationX(-0.001);
				// target = target * rst::matrix<1, 1>::translation(camera_position);
				camera_target._vector[0] = target._vector[0];
				camera_target._vector[1] = target._vector[1];
				camera_target._vector[2] = target._vector[2];

			}
			else if (this->mouse_y - mouse_y < 0) {

				rst::vector<4> target = camera_target.extend<4>({1.0});
				rst::vector<3> temp_pos = camera_position;
				temp_pos.negate();
				// target = target * rst::matrix<1, 1>::translation(temp_pos);
				target = target * rst::matrix<1, 1>::rotationX(0.001);
				// target = target * rst::matrix<1, 1>::translation(camera_position);
				camera_target._vector[0] = target._vector[0];
				camera_target._vector[1] = target._vector[1];
				camera_target._vector[2] = target._vector[2];
			
			}

			if (this->mouse_x - mouse_x > 0) {

			rst::vector<4> target = camera_target.extend<4>({1.0});
			rst::vector<3> temp_pos = camera_position;
			temp_pos.negate();
			// target = target * rst::matrix<1, 1>::translation(temp_pos);
			target = target * rst::matrix<1, 1>::rotationY(0.001);
			// target = target * rst::matrix<1, 1>::translation(camera_position);
			camera_target._vector[0] = target._vector[0];
			camera_target._vector[1] = target._vector[1];
			camera_target._vector[2] = target._vector[2];

			}
			else if (this->mouse_x - mouse_x < 0) {
			
			rst::vector<4> target = camera_target.extend<4>({1.0});
			rst::vector<3> temp_pos = camera_position;
			temp_pos.negate();
			// target = target * rst::matrix<1, 1>::translation(temp_pos);
			target = target * rst::matrix<1, 1>::rotationY(-0.001);
			// target = target * rst::matrix<1, 1>::translation(camera_position);
			camera_target._vector[0] = target._vector[0];
			camera_target._vector[1] = target._vector[1];
			camera_target._vector[2] = target._vector[2];
			
			}

			this->mouse_x = mouse_x;
			this->mouse_y = mouse_y;
		
		}

	}
	else {
		this->mouse_x = -1; 
		this->mouse_y = -1; 
	}

	return S_OK;

}

LRESULT CubeRender::OnInput(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	WPARAM real_wParam = GET_RAWINPUT_CODE_WPARAM(wParam);
	
	switch (real_wParam) {
		case RIM_INPUT:
			{
				RAWINPUT raw_input = {};
				UINT raw_input_size = sizeof(raw_input);
				UINT ret = GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw_input, &raw_input_size, sizeof(RAWINPUTHEADER));
				if (ret > 0) {
					if (raw_input.header.dwType == RIM_TYPEMOUSE) {
						if (!(raw_input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) {
							// printf("lLastX: %d, lLastY: %d.\n", raw_input.data.mouse.lLastX, raw_input.data.mouse.lLastY);
							MouseCameraMovement(raw_input.data.mouse.lLastX, raw_input.data.mouse.lLastY);
						}
					}
					// DrainRawInputQueue();
				}
			}
			return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);
			break;
		case RIM_INPUTSINK:
			break;
		default:
			break;
	}

	return S_OK;

}

void CubeRender::MouseCameraMovement(LONG lLastX, LONG lLastY) {

	camera_target.print();

	if (lLastY < 0) {

		rst::vector<4> target = camera_target.extend<4>({1.0});
		rst::vector<3> temp_pos = camera_position;
		temp_pos.negate();
		// target = target * rst::matrix<1, 1>::translation(temp_pos);
		target = target * rst::matrix<1, 1>::rotationX(-0.001);
		// target = target * rst::matrix<1, 1>::translation(camera_position);
		camera_target._vector[0] = target._vector[0];
		camera_target._vector[1] = target._vector[1];
		camera_target._vector[2] = target._vector[2];

	}
	else if (lLastY > 0) {

		rst::vector<4> target = camera_target.extend<4>({1.0});
		rst::vector<3> temp_pos = camera_position;
		temp_pos.negate();
		// target = target * rst::matrix<1, 1>::translation(temp_pos);
		target = target * rst::matrix<1, 1>::rotationX(0.001);
		// target = target * rst::matrix<1, 1>::translation(camera_position);
		camera_target._vector[0] = target._vector[0];
		camera_target._vector[1] = target._vector[1];
		camera_target._vector[2] = target._vector[2];
	
	}

	if (lLastX < 0) {

	rst::vector<4> target = camera_target.extend<4>({1.0});
	rst::vector<3> temp_pos = camera_position;
	temp_pos.negate();
	// target = target * rst::matrix<1, 1>::translation(temp_pos);
	target = target * rst::matrix<1, 1>::rotationY(0.001);
	// target = target * rst::matrix<1, 1>::translation(camera_position);
	camera_target._vector[0] = target._vector[0];
	camera_target._vector[1] = target._vector[1];
	camera_target._vector[2] = target._vector[2];

	}
	else if (lLastX > 0) {
	
	rst::vector<4> target = camera_target.extend<4>({1.0});
	rst::vector<3> temp_pos = camera_position;
	temp_pos.negate();
	// target = target * rst::matrix<1, 1>::translation(temp_pos);
	target = target * rst::matrix<1, 1>::rotationY(-0.001);
	// target = target * rst::matrix<1, 1>::translation(camera_position);
	camera_target._vector[0] = target._vector[0];
	camera_target._vector[1] = target._vector[1];
	camera_target._vector[2] = target._vector[2];
	
	}

}

LRESULT CubeRender::OnActivate(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (wParam) {
		case WA_ACTIVE:
			printf("CubeRender::OnActivate() WA_ACTIVE.\n");
			cursor_clipped = true;

			GetClipCursor(&old_clip_rect);
			printf("old_clip_rect: %d, %d, %d, %d.\n", old_clip_rect.left, old_clip_rect.top, old_clip_rect.right, old_clip_rect.bottom);
			GetWindowRect(hWnd, &window_rect);
			printf("window_rect: %d, %d, %d, %d.\n", window_rect.left, window_rect.top, window_rect.right, window_rect.bottom);
			{
				int clip_x = (window_rect.right - window_rect.left) / 2;
				int clip_y = (window_rect.bottom - window_rect.top) / 2;
				RECT new_clip_rect = {clip_x, clip_y, clip_x, clip_y};
				ClipCursor(&new_clip_rect);
				ShowCursor(FALSE);
				// ClipCursor(&window_rect);
			}

			break;
		case WA_CLICKACTIVE:
			printf("CubeRender::OnActivate() WA_CLICKACTIVE.\n");
			camera_panning_running = true;
			cursor_clipped = true;

			GetClipCursor(&old_clip_rect);
			printf("old_clip_rect: %d, %d, %d, %d.\n", old_clip_rect.left, old_clip_rect.top, old_clip_rect.right, old_clip_rect.bottom);
			GetWindowRect(hWnd, &window_rect);
			printf("window_rect: %d, %d, %d, %d.\n", window_rect.left, window_rect.top, window_rect.right, window_rect.bottom);
			{
				int clip_x = (window_rect.right - window_rect.left) / 2;
				int clip_y = (window_rect.bottom - window_rect.top) / 2;
				RECT new_clip_rect = {clip_x, clip_y, clip_x, clip_y};
				ClipCursor(&new_clip_rect);
				ShowCursor(FALSE);
				// ClipCursor(&window_rect);
			}

			break;
		case WA_INACTIVE:
			printf("CubeRender::OnActivate() WA_INACTIVE.\n");
			camera_panning_running = false;
			if (cursor_clipped) {
				ClipCursor(&old_clip_rect);
				ShowCursor(TRUE);
				cursor_clipped = false;
			}
			break;
		default:
			break;
	}

	return S_OK;
}

LRESULT CubeRender::OnChar(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	rst::vector<3> temp_target = camera_target;
	temp_target._vector[1] = 0.0;
	temp_target.normalize();

	switch (wParam) {
		case 'w':
			// printf("W\n");
			// cube_position_y += 0.1;
			// camera_position_z -= 0.1;
			// camera_target_y -= 0.1;
			// camera_position = camera_position + camera_target * 0.05;
			camera_position = camera_position + temp_target * 0.05;
			break;
		case 's':
			// printf("S\n");
			// cube_position_y -= 0.1;
			// camera_position_z += 0.1;
			// camera_position = camera_position - camera_target * 0.05;
			camera_position = camera_position - temp_target * 0.05;
			// camera_target_y += 0.1;
			break;
		case 'a':
			// printf("A\n");
			// cube_position_x -= 0.1;
			camera_position_x -= 0.1;
			{
				// rst::vector<3> temp_direction = rst::vector<1>::cross(camera_up, camera_target);
				rst::vector<3> temp_direction = rst::vector<1>::cross(camera_up, temp_target);
				temp_direction.normalize();
				camera_position = camera_position + temp_direction * 0.05;
			}
			break;
		case 'd':
			// printf("D\n");
			// cube_position_x += 0.1;
			camera_position_x += 0.1;
			{
				// rst::vector<3> temp_direction = rst::vector<1>::cross(camera_up, camera_target);
				rst::vector<3> temp_direction = rst::vector<1>::cross(camera_up, temp_target);
				temp_direction.normalize();
				camera_position = camera_position - temp_direction * 0.05;
			}
			// camera_target_x += 0.1;
			break;
		case 'z':
			camera_rotation += 3.0;
			break;
		case 'x':
			camera_rotation -= 3.0;
			break;
		case VK_SPACE:
			SaveRender("./render.bmp");
			SaveDepth("./depth.bmp");
			LogRasterizer();
			break;
		case VK_TAB:
			break;
		case '1':
			printf("Rendering algorithm set to RASTERIZER_ALGORITHM::NAIVE.\n");
			rasterizer_algorithm = RASTERIZER_ALGORITHM::NAIVE;
			break;
		case '2':
			printf("Rendering algorithm set to RASTERIZER_ALGORITHM::BARYCENTRIC.\n");
			rasterizer_algorithm = RASTERIZER_ALGORITHM::BARYCENTRIC;
			break;
		case '3':
			printf("Rendering algorithm set to RASTERIZER_ALGORITHM::SIMD.\n");
			rasterizer_algorithm = RASTERIZER_ALGORITHM::SIMD;
			break;
		case '4':
			printf("Rendering algorithm set to RASTERIZER_ALGORITHM::BARYCENTRIC_FLOATING_POINT.\n");
			rasterizer_algorithm = RASTERIZER_ALGORITHM::BARYCENTRIC_FLOATING_POINT;
			break;
		case '5':
			printf("Rendering algorithm set to RASTERIZER_ALGORITHM::SIMD_INTEGER.\n");
			rasterizer_algorithm = RASTERIZER_ALGORITHM::SIMD_INTEGER;
			break;
		default:
			break;
	}

	return S_OK;

}
