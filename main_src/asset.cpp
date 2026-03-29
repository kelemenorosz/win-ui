
#include "asset.h"
#include "bitmap.h"
#include <fstream>

asset::Mesh::Mesh() : wavefront_obj(nullptr) {
	
	wavefront_obj = new wavefront::obj();

	return;
}

asset::Mesh::Mesh(const std::wstring& path) : wavefront_obj(nullptr) {

	wavefront_obj = wavefront::Load(path);

	return;

}

asset::Mesh::Mesh(std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer) : wavefront_obj(nullptr) {

	wavefront_obj = new wavefront::obj();
	wavefront_obj->vertex_list = vertexbuffer;
	wavefront_obj->index_list = indexbuffer;

	return;

}

asset::Mesh::Mesh(asset::Mesh& other) : wavefront_obj(nullptr) {

	return;

}

asset::Mesh::Mesh(asset::Mesh&& other) : wavefront_obj(nullptr) {

	return;

}

asset::Mesh& asset::Mesh::operator=(asset::Mesh& other) {

	this->wavefront_obj->vertex_list = other.wavefront_obj->vertex_list;
	this->wavefront_obj->index_list = other.wavefront_obj->index_list;

	return *this;

}

asset::Mesh& asset::Mesh::operator=(asset::Mesh&& other) {

	this->wavefront_obj = other.wavefront_obj;
	other.wavefront_obj = nullptr;
	return *this;

}

asset::Mesh::~Mesh() {

	if (wavefront_obj != nullptr) delete wavefront_obj;

	return;

}

rst::bufferlist<rst::vertex<double>> asset::Mesh::CloneVertexBuffer() {

	rst::bufferlist<rst::vertex<double>> bufferlist = { nullptr, static_cast<int>(wavefront_obj->vertex_list.size()) };
	bufferlist.ptr = new rst::vertex<double>[wavefront_obj->vertex_list.size()];
	for (int i = 0; i < bufferlist.len; ++i) {
		bufferlist.ptr[i] = wavefront_obj->vertex_list[i];
	}

	return bufferlist;

}

std::vector<rst::vertex<double>> asset::Mesh::CloneVertexBuffer_vec() {

	std::vector<rst::vertex<double>> bufferlist = wavefront_obj->vertex_list;

	return bufferlist;

}

rst::bufferlist<int> asset::Mesh::CloneIndexBuffer() {

	rst::bufferlist<int> bufferlist = { nullptr, static_cast<int>(wavefront_obj->index_list.size()) };
	bufferlist.ptr = new int[wavefront_obj->index_list.size()];
	for (int i = 0; i < bufferlist.len; ++i) {
		bufferlist.ptr[i] = wavefront_obj->index_list[i];
	}

	return bufferlist;

}

std::vector<int> asset::Mesh::CloneIndexBuffer_vec() {

	std::vector<int> bufferlist = wavefront_obj->index_list;

	return bufferlist;

}

rst::bufferlist<int> asset::Mesh::GetIndexBuffer() {

	rst::bufferlist<int> bufferlist = { nullptr, static_cast<int>(wavefront_obj->index_list.size()) };
	bufferlist.ptr = &wavefront_obj->index_list[0];

	return bufferlist;

}

int asset::Mesh::GetVertexBufferLen() {

	return static_cast<int>(wavefront_obj->vertex_list.size());

}

asset::Tex::Tex(const std::string& path) : width(0), height(0), pixel_size(0), ptr(nullptr) {

	if (path.find(".bmp") == std::string::npos) return;

	std::pair<BITMAPINFOHEADER, BYTE*> ret = ReadBitmap(path);
	if (ret.second == nullptr) return;

	width = ret.first.biWidth;
	height = abs(ret.first.biHeight);
	pixel_size = ret.first.biBitCount;

	ptr = reinterpret_cast<BYTE*>(ret.second);

	return;

}

asset::Tex::~Tex() {

	if (ptr != nullptr) delete[] ptr;

	return;

}

int asset::Tex::GetWidth() {

	return width;

}

int asset::Tex::GetHeight() {

	return height;

}

UINT32 asset::Tex::GetValue(int pos) {

	if (pixel_size == 32) return reinterpret_cast<UINT32*>(ptr)[pos];

	if (pixel_size == 24) {

		UINT8* pixel_ptr = &reinterpret_cast<UINT8*>(ptr)[pos * 3];
		UINT32 ret = pixel_ptr[0] | (pixel_ptr[1] << 8) | (pixel_ptr[2] << 16);
		return ret;

	}

	return 0;

}