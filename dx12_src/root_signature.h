#pragma once

#include "directx/d3dx12.h"
#include <d3d12.h>
#include <wrl.h>

class RootSignature {

	public:

		RootSignature(Microsoft::WRL::ComPtr<ID3D12Device2>, D3D12_ROOT_PARAMETER1*, UINT, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
		~RootSignature();
		static void SetRootSignatureVer(Microsoft::WRL::ComPtr<ID3D12Device2> dev);

		Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature();


	private:

		enum class RS_STATE : uint8_t {

			RS_DEFAULT = 0,
			RS_READY,

		};


		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
		static D3D12_FEATURE_DATA_ROOT_SIGNATURE ver;
		static RS_STATE state;

};
