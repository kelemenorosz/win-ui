
#include "cube_render.h"
#include "asset.h"
#include <numbers>

CubeRender::CubeRender(LPCWSTR windowName, int x, int y, int nWidth, int nHeight) : Rasterizer(windowName, x, y, nWidth, nHeight),
	cube(L"./mesh/sphere.obj"), cube_rotation(0.0), cube_distance(0.0), depth_texture("./tex/butterfly_tex.bmp"), rasterizer_algorithm(RASTERIZER_ALGORITHM::NAIVE) {

	cube_position_x = 0.0;
	cube_position_y = 0.0;
	cube_position_z = 5.0;

	rst::point<double> test_p = {1.0, 1.0, -1.1, 0.0};
	printf("Point before perspective projection: %f %f %f %f.\n", test_p.x, test_p.y, test_p.z, test_p.w);
	rst::perspective_fix(test_p, 90, 1.0, 1.0, 50.0);
	printf("Point after perspective projection: %f %f %f %f.\n", test_p.x, test_p.y, test_p.z, test_p.w);

	std::random_device rd;
	mersenne_twister.seed(rd());

	color_len = cube.GetVertexBufferLen();
	color_buf = new rst::color[color_len];
	for (int i = 0; i < color_len; ++i) {
		color_buf[i + 0] = rst::color(mersenne_twister() % 256, mersenne_twister() % 256, mersenne_twister() % 256);
	}

	// pass_list.push_back(&cube);

	// triangle_vert.push_back(rst::vertex<double>(-1.0, 1.0, 1.0, 0.0, 255.0, 0.0));
	// triangle_vert.push_back(rst::vertex<double>(-1.0, -1.0, 1.0, 255.0, 0.0, 0.0));
	// triangle_vert.push_back(rst::vertex<double>(1.0, -1.0, 1.0, 0.0, 0.0, 255.0));
	triangle = asset::Mesh(triangle_vert, triangle_index);

	texture_width = 64;
	texture_height = 64;
	checker_size = 2;
	texture = new UINT32[texture_width * texture_height]; 
	for (int i = 0; i < texture_height; i++) {
		for (int j = 0; j < texture_width; j++) {
			bool isWhite = (i / checker_size + j / checker_size) % 2 ? true : false;
			if (!isWhite) {
				texture[i * texture_width + j] = 0xFF00FF00;
			} 
			else {
				texture[i * texture_width + j] = 0x00FF00FF;
			}
		}
	}

	StartRender();

	return;

}

CubeRender::~CubeRender() {

	delete[] texture;
	delete[] color_buf;

	return;

}

LRESULT CubeRender::OnRender() {

	ClearBackground();
	ClearDepthBuffer();
	AddMesh(&cube);
	// AddMesh(&triangle);
	AddOnVertex(std::bind(&CubeRender::OnVertex, this, std::placeholders::_1));
	AddOnPixel(std::bind(&CubeRender::OnPixel, this, std::placeholders::_1));
	SetWindingOrder(RASTERIZER_WINDING_ORDER::NONE);
	SetRasterizerAlgorithm(rasterizer_algorithm);
	Draw();

	cube_rotation += 0.05;

	return S_OK;

}

void CubeRender::OnVertex(rst::vertex<double>& vertex) {

	// vertex.color.x = color_buf[color_counter % color_len].r;
	// vertex.color.y = color_buf[color_counter % color_len].g;
	// vertex.color.z = color_buf[color_counter % color_len].b;
	// color_counter++;

	// rst::rotate_x(vertex.position, cube_position_z);
	// rst::rotate_y(vertex.position, cube_position_z);
	// rst::rotate_z(vertex.position, -cube_position_z);
	
	// -- Initial position z = 0 
	// vertex.position.x *= 2;
	// vertex.position.y *= 2;
	// vertex.position.z *= 2;
	vertex.position.x += cube_position_x;
	vertex.position.y += cube_position_y;
	vertex.position.z -= cube_position_z;

	// vertex.position.z -= cube_distance - 3 - 1.3; // At -0.3 - 1.7
	// vertex.position.z -= cube_distance - 3; // At 1-3
	// vertex.position.z -= cube_distance + 6; // At 9-11
	rst::perspective_fix(vertex.position, 90, 1.0f, 1.0f, 4.0f);
	// printf("vert: %f, %f, %f, %f.\n", vertex.position.x, vertex.position.y, vertex.position.z, vertex.position.w);

	return;

}

void CubeRender::OnPixel(rst::vertex<double>& pixel) {

	UINT32 sample = SampleTexture(pixel.texture.x, pixel.texture.y, depth_texture);

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

LRESULT CubeRender::OnChar(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (wParam) {
		case 'w':
			cube_position_y += 0.1;
			break;
		case 's':
			cube_position_y -= 0.1;
			break;
		case 'a':
			cube_position_x -= 0.1;
			break;
		case 'd':
			cube_position_x += 0.1;
			break;
		case VK_SPACE:
			SaveRender("./render.bmp");
			SaveDepth("./depth.bmp");
			LogRasterizer();
			break;
		case VK_TAB:
			// if (rasterizer_algorithm == RASTERIZER_ALGORITHM::NAIVE) {
			// 	rasterizer_algorithm = RASTERIZER_ALGORITHM::BARYCENTRIC;
			// }
			// else {
			// 	rasterizer_algorithm = RASTERIZER_ALGORITHM::NAIVE;
			// }
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
		default:
			break;
	}

	return S_OK;

}
