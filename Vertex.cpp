#include "Vertex.h"
#include "Effects.h"

#pragma region InputLayoutDesc


// 2인데 3으로 설정되어 있었음 
const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc::Basic32[2] = 
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}	
};


#pragma endregion

#pragma region InputLayouts

ID3D11InputLayout* InputLayouts::Basic32 = 0;

void InputLayouts::InitAll(ID3D11Device* device)
{
	D3DX11_PASS_DESC passDesc;

	//
	// Basic32
	//

	Effects::BasicFX->LightTech->GetPassByIndex(0)->GetDesc(&passDesc);

	// pos & normal 
	HR(device->CreateInputLayout(InputLayoutDesc::Basic32, 2, passDesc.pIAInputSignature, 
		passDesc.IAInputSignatureSize, &Basic32));
	
}

void InputLayouts::DestroyAll()
{
	ReleaseCOM(Basic32);
}

#pragma endregion
