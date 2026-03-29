#pragma once

#include "auxiliary.h"
#include "wavefront_loader.h"
#include <string>

namespace asset {

	class Mesh {

		public:

			Mesh();
			Mesh(const std::wstring& path);
			Mesh(std::vector<rst::vertex<double>>& vertexbuffer, std::vector<int>& indexbuffer);
			~Mesh();

			Mesh(Mesh& other);
			Mesh& operator=(Mesh& other);

			Mesh(Mesh&& other);
			Mesh& operator=(Mesh&& other);

			rst::bufferlist<rst::vertex<double>> CloneVertexBuffer();
			std::vector<rst::vertex<double>> CloneVertexBuffer_vec();
			rst::bufferlist<int> CloneIndexBuffer();
			std::vector<int> CloneIndexBuffer_vec();
			rst::bufferlist<int> GetIndexBuffer();

			int GetVertexBufferLen();

		private:

			wavefront::obj* wavefront_obj;

	};

	class Tex {

		public:

			Tex() = delete;
			Tex(const std::string& path);
			~Tex();

			Tex(Tex& other) = delete;
			Tex& operator=(Tex& other) = delete;

			Tex(Tex&& other) = delete;
			Tex& operator=(Tex&& other) = delete;

			int GetWidth();
			int GetHeight();
			UINT32 GetValue(int pos);

		private:

			int width;
			int height;
			int pixel_size;
			BYTE* ptr;

	};

}
