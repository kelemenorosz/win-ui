
#include "bitmap.h"
#include <fstream>

void WriteBitmap(const std::string& path, const DIBSECTION& dibsection) {

	BITMAPFILEHEADER bmp_file_header = {};
	bmp_file_header.bfType = 0x4D42;
	bmp_file_header.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dibsection.dsBmih.biSizeImage;
	bmp_file_header.bfReserved1 = 0;
	bmp_file_header.bfReserved2 = 0;
	bmp_file_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	std::ofstream file(path, std::ios::binary);
	file.write(reinterpret_cast<char*>(&bmp_file_header), sizeof(BITMAPFILEHEADER));
	file.write(reinterpret_cast<char*>(const_cast<BITMAPINFOHEADER*>(&dibsection.dsBmih)), sizeof(BITMAPINFOHEADER));
	file.write(reinterpret_cast<char*>(dibsection.dsBm.bmBits), dibsection.dsBmih.biSizeImage);
	file.close();

	return;

}

void WriteBitmap(const std::string& path, const HBITMAP& bmp_handle) {

	DIBSECTION dibsection = {};
	GetObject(bmp_handle, sizeof(DIBSECTION), &dibsection);
	if (dibsection.dsBm.bmBits != 0) {
		dibsection.dsBmih.biHeight *= -1;
		WriteBitmap(path, dibsection);
		return;
	}

	return;

}

std::pair<BITMAPINFOHEADER, BYTE*> ReadBitmap(const std::string& path) {

	std::ifstream file(path, std::ios::binary);
	if (file.rdstate() != 0) return std::make_pair(BITMAPINFOHEADER(), nullptr);

	int file_len = static_cast<int>(file.seekg(0, std::ios::end).tellg());
	file.seekg(0);
	
	BYTE* file_ptr = new BYTE[file_len];
	file.read(reinterpret_cast<char*>(file_ptr), file_len);

	if (!(file_ptr[0] == 0x42 && file_ptr[1] == 0x4D)) {
		delete[] file_ptr;
		return std::make_pair(BITMAPINFOHEADER(), nullptr);
	}

	BITMAPFILEHEADER* bmp_file_header = reinterpret_cast<BITMAPFILEHEADER*>(&file_ptr[0]);
	BITMAPINFOHEADER* bmp_info_header = reinterpret_cast<BITMAPINFOHEADER*>(&file_ptr[0 + sizeof(BITMAPFILEHEADER)]);

	if (bmp_info_header->biClrUsed != 0 || bmp_info_header->biPlanes != 1) {
		delete[] file_ptr;
		return std::make_pair(BITMAPINFOHEADER(), nullptr);
	}	

	if (bmp_info_header->biBitCount != 32 && bmp_info_header->biBitCount != 24) {
		delete[] file_ptr;
		return std::make_pair(BITMAPINFOHEADER(), nullptr);	
	}

	BYTE* image_ptr = new BYTE[bmp_info_header->biSizeImage];
	memcpy(&image_ptr[0], &file_ptr[0 + sizeof(BITMAPFILEHEADER) + bmp_info_header->biSize], bmp_info_header->biSizeImage);

	BITMAPINFOHEADER bmp_info = {};
	memcpy(&bmp_info, bmp_info_header, sizeof(BITMAPINFOHEADER));

	file.close();
	delete[] file_ptr;

	return std::make_pair(bmp_info, image_ptr);

}