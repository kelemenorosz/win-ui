#pragma once

#include "directx/d3dx12.h"
#include <d3d12.h>
#include <string>
#include <d3dcompiler.h>
#include <wrl.h>

enum class Shader_Type : uint8_t {

	SHADER_TYPE_VS = 0,
	SHADER_TYPE_PS,

};

class Shader {

	public:

		Shader(std::string file_name, Shader_Type t);
		~Shader() {}

		Shader(const Shader&& other) = delete;

		D3D12_SHADER_BYTECODE* GetBytecode();

	private:

		Microsoft::WRL::ComPtr<ID3DBlob> blob;
		D3D12_SHADER_BYTECODE bytecode;

};