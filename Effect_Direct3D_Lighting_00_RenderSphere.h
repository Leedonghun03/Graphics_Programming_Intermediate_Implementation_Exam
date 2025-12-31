#pragma once

#include "resource.h"
#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "Effects.h"
#include "Vertex.h"

class CKDX_LightingGeo : public D3DApp
{
public:
	CKDX_LightingGeo(HINSTANCE hInstance);
	~CKDX_LightingGeo();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void BuildGeometryBuffers();
	void BuildFX();
	void BuildVertexLayout();

private:
	// ÀÚ¿¬±¤
	DirectionalLight dummyLight;

	// ========== Box A (»¡°­, Å©±â 10) ==========
	ID3D11Buffer* mBoxAVB;
	ID3D11Buffer* mBoxAIB;
	UINT mBoxAIndexCount;
	XMFLOAT4X4 mBoxAWorld;
	float mBoxAngle;

	// ========== Box B (ÆÄ¶û, Å©±â 2) ==========
	ID3D11Buffer* mBoxBVB;
	ID3D11Buffer* mBoxBIB;
	UINT mBoxBIndexCount;
	XMFLOAT4X4 mBoxBWorld;
	float mBoxBOrbitAngle;
	float mBoxBRotateAngle;

	ID3D11InputLayout* mInputLayout;

	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	XMFLOAT3 mEyePosW;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;
};