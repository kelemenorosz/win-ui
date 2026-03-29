
#include "shader.h"
#include <fstream>
#include <sstream>

//Safe release COM pointers
template<class T> void SafeRelease(T** ptr) {

	if (*ptr != nullptr) {

		(*ptr)->Release();
		(*ptr) = nullptr;

	}
	return;

}

Shader::Shader(std::string file_name, Shader_Type t) {

	std::ifstream file_stream;
	std::stringstream file_string_stream;
	std::string shader_string;

	file_stream.open(file_name, std::ios_base::in);
	file_string_stream.str("");
	file_string_stream.clear();
	file_string_stream << file_stream.rdbuf();
	shader_string = file_string_stream.str();
	const char* shader_string_ptr = shader_string.c_str();

	ID3DBlob* err = nullptr;
	ID3DBlob* temp_blob = nullptr;
	HRESULT res = S_OK;

	switch (t) {
		case Shader_Type::SHADER_TYPE_VS:
			res = D3DCompile(static_cast<LPCVOID>(shader_string_ptr), shader_string.size(), NULL, NULL, NULL, "vertex_main", "vs_5_1", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &temp_blob, &err);
			break;
		case Shader_Type::SHADER_TYPE_PS:
			res = D3DCompile(static_cast<LPCVOID>(shader_string_ptr), shader_string.size(), NULL, NULL, NULL, "pixel_main", "ps_5_1", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &temp_blob, &err);
			break;
		default:
			printf("Shader_Type t not valid.\n");
			break;
	}

	if (res != S_OK) printf("D3DCompile() error code: %d.\n", res);
	if (err != nullptr) {
		
		LPVOID pErr = err->GetBufferPointer();
		printf("ID3DBlob error message:\n%s\n", static_cast<char*>(pErr));

	}

	SafeRelease(&err);

	blob = temp_blob;
	SafeRelease(&temp_blob);

	if (res != S_OK) return;

	bytecode.pShaderBytecode = blob->GetBufferPointer();
	bytecode.BytecodeLength = blob->GetBufferSize();

	return;

}

D3D12_SHADER_BYTECODE* Shader::GetBytecode() {

	return &bytecode;

}