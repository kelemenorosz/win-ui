
#include "wavefront_loader.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <map>

wavefront::obj* wavefront::Load(const std::wstring& path) {

	// -- Create wavefront object

 	wavefront::obj* object = new wavefront::obj();

	// -- Open mesh file

 	std::ifstream wavefront;
 	wavefront.open(path.c_str(), std::ios_base::in);

 	// -- Temporary vectors
 	
 	std::vector<double> uv(0);
 	std::vector<double> position(0);
 	std::vector<int> index_position(0);
 	std::vector<int> index_uv(0);

 	std::map<std::pair<int, int>, rst::vertex<double>> map_temp;

 	// -- Parse mesh file

 	char* strtok_context = nullptr;
 	char buffer[1024];
 	while (!wavefront.eof()) {

	 	wavefront.getline(&buffer[0], 1024, '\n');
	 	char* wavefront_type = strtok_s(buffer, " ", &strtok_context);

	 	if (wavefront_type != nullptr && !strcmp(wavefront_type, "f")) {

	 		char* token = strtok_s(wavefront_type + strlen(wavefront_type) + 1, "/ ", &strtok_context);
	 		
	 		while (token != nullptr) {

	 			// -- Vertex index

				index_position.push_back(static_cast<int>(std::atoi(token) - 1));
				token = strtok_s(NULL, "/ ", &strtok_context);

				// -- Texture index

				index_uv.push_back(static_cast<int>(std::atoi(token) - 1));
				token = strtok_s(NULL, "/ ", &strtok_context);
	 			
				// -- Normal index

	 			token = strtok_s(NULL, "/ ", &strtok_context);

	 		}

	 	}
	 	else if (wavefront_type != nullptr && !strcmp(wavefront_type, "v")) {

	 		char* token = strtok_s(wavefront_type + strlen(wavefront_type) + 1, " ", &strtok_context);
	 		
	 		while (token != nullptr) {

	 			// -- Vertex

				position.push_back(static_cast<double>(std::atof(token)));
				token = strtok_s(NULL, "/ ", &strtok_context);

	 		}

	 	}
	 	else if (wavefront_type != nullptr && !strcmp(wavefront_type, "vt")) {

	 		char* token = strtok_s(wavefront_type + strlen(wavefront_type) + 1, " ", &strtok_context);
	 		
	 		while (token != nullptr) {

	 			// -- UV

				uv.push_back(static_cast<double>(std::atof(token)));
				token = strtok_s(NULL, "/ ", &strtok_context);

	 		}

	 	}

 	}

 	// std::cout << "[VERTEX POSITION] Vertex position: ";
	// for (const auto& i : position) {
	// 	std::cout << i << " ";
	// }
	// std::cout << std::endl;

	// std::cout << "[VERTEX TEXTURE] Vertex texture: ";
	// for (const auto& i : uv) {
	// 	std::cout << i << " ";
	// }
	// std::cout << std::endl;

	// std::cout << "[INDEX POSITION] Vertex position index: ";
	// for (const auto& i : index_position) {
	// 	std::cout << i << " ";
	// }
	// std::cout << std::endl;

	// std::cout << "[INDEX TEXTURE] Vertex texture index: ";
	// for (const auto& i : index_uv) {
	// 	std::cout << i << " ";
	// }
	// std::cout << std::endl;

	// -- Build index buffer

	for (int i = 0; i < index_position.size(); ++i) {

		// -- Check if vertex exists

		bool vertex_exists = false;
		int vertex_position = -1;
		for (int j = 0; j < object->vertex_list.size(); ++j) {

			if (object->vertex_list[j].position.x == position[index_position[i] * 3] && 
				object->vertex_list[j].position.y == position[index_position[i] * 3 + 1] &&
				object->vertex_list[j].position.z == position[index_position[i] * 3 + 2] &&
				object->vertex_list[j].texture.x == uv[index_uv[i] * 2] &&
				object->vertex_list[j].texture.y == uv[index_uv[i] * 2 + 1]) {
				vertex_exists = true;
				vertex_position = j;
			}

		}

		if (vertex_exists) {
			object->index_list.push_back(vertex_position);
		}
		else {
			object->vertex_list.emplace_back(
				position[index_position[i] * 3], 
				position[index_position[i] * 3 + 1], 
				position[index_position[i] * 3 + 2], 
				uv[index_uv[i] * 2], 
				uv[index_uv[i] * 2 + 1]);
			object->index_list.push_back(static_cast<int>(object->vertex_list.size() - 1));
		}

	}

	// -- Print

	// for (const auto& i : object->vertex_list) {

	// 	std::cout << i.position[0] << " " << i.position[1] << " " << i.position[2] << " " << i.texture[0] << " " << i.texture[1] << " " << std::endl;

	// }

	// for (const auto& i : object->index_list) {

	// 	std::cout << i << " ";

	// }
	// std::cout << std::endl;

 	// -- Return waveform object

	return object;

}