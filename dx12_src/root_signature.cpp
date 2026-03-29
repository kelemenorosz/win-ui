
#include "root_signature.h"
#include "directx/d3dx12.h"
#include <d3d12.h>
#include <wrl.h>

namespace mWRL = Microsoft::WRL;

//Safe release COM pointers
template<class T> void SafeRelease(T** ptr) {

	if (*ptr != nullptr) {

		(*ptr)->Release();
		(*ptr) = nullptr;

	}
	return;

}

D3D12_FEATURE_DATA_ROOT_SIGNATURE RootSignature::ver = {};
RootSignature::RS_STATE RootSignature::state = RS_STATE::RS_DEFAULT;

RootSignature::RootSignature(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_ROOT_PARAMETER1* rootParameter, UINT parameterCount, D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags) {

	if (state == RS_STATE::RS_DEFAULT) {
		printf("[ERROR] RootSignature::version not set.\n");
		return;
	}

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription = {};
	rootSignatureDescription.Init_1_1(parameterCount, rootParameter, 0, NULL, rootSignatureFlags);

	ID3D10Blob* serializedRootSignatureBlob = nullptr;
	ID3D10Blob* errorBlob = nullptr;
	HRESULT hr = S_OK;
	hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, this->ver.HighestVersion, &serializedRootSignatureBlob, &errorBlob);
	if (hr != S_OK) {
		printf("[ERROR] D3DX12SerializeVersionedRootSignature() failed with error code %d.\n", hr);
		return;
	}

	hr = device->CreateRootSignature(0, serializedRootSignatureBlob->GetBufferPointer(), serializedRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
	if (hr != S_OK) {
		printf("[ERROR] CreateRootSignature() failed with error code %d.\n", hr);
		return;
	}

	SafeRelease(&serializedRootSignatureBlob);
	SafeRelease(&errorBlob);

}

RootSignature::~RootSignature() {



}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature::GetRootSignature() {

	return m_rootSignature;

}

void RootSignature::SetRootSignatureVer(Microsoft::WRL::ComPtr<ID3D12Device2> dev) {

	ver.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(dev->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &ver, sizeof(D3D12_FEATURE_DATA_ROOT_SIGNATURE)))) {
		ver.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	state = RS_STATE::RS_READY;

	return;

}
