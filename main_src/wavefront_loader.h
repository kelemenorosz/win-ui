#pragma once

#include <string>
#include <vector>
#include "auxiliary.h"

namespace wavefront {

	struct obj {

		std::vector<int> index_list;
		std::vector<rst::vertex<double>> vertex_list;

	};

	obj* Load(const std::wstring& path);

}