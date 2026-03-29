#pragma once

#include <string>
#include <windows.h>
#include <utility>

void WriteBitmap(const std::string& path, const DIBSECTION& dibsection);
void WriteBitmap(const std::string& path, const HBITMAP& bmp_handle);

std::pair<BITMAPINFOHEADER, BYTE*> ReadBitmap(const std::string& path);