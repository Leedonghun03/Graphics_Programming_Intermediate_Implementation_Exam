#include "framework.h"
#include "Effect_Direct3D_Lighting_00_RenderSphere.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	CKDX_LightingGeo theApp(hInstance);

	if (!theApp.Init())
		return 0;

	return theApp.Run();
}

CKDX_LightingGeo::CKDX_LightingGeo(HINSTANCE hInstance)
	: D3DApp(hInstance),
	mBoxAVB(0), mBoxAIB(0),
	mBoxBVB(0), mBoxBIB(0),
	mInputLayout(0),
	mEyePosW(0.0f, 0.0f, 0.0f),
	mTheta(1.5f * MathHelper::Pi),
	mPhi(0.25f * MathHelper::Pi),
	mRadius(150.0f),
	mBoxAngle(0.0f),
	mBoxBOrbitAngle(0.0f),
	mBoxBRotateAngle(0.0f)
{
	mMainWndCaption = L"Crate Demo";

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mBoxAWorld, I);
	XMStoreFloat4x4(&mBoxBWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);
}

CKDX_LightingGeo::~CKDX_LightingGeo()
{
	ReleaseCOM(mBoxAVB);
	ReleaseCOM(mBoxAIB);
	ReleaseCOM(mBoxBVB);
	ReleaseCOM(mBoxBIB);

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
}

bool CKDX_LightingGeo::Init()
{
	if (!D3DApp::Init())
		return false;

	BuildGeometryBuffers();
	BuildFX();
	BuildVertexLayout();

	return true;
}

void CKDX_LightingGeo::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void CKDX_LightingGeo::UpdateScene(float dt)
{
	// 카메라 설정
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	mEyePosW = XMFLOAT3(x, y, z);

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);

	// ========= 원 궤도, 자전, 공전 구하는 부분 =========
	// Box A 부분
	mBoxAngle += XMConvertToRadians(3.0f) * dt;
	if (mBoxAngle >= XM_2PI)
		mBoxAngle -= XM_2PI;

	XMMATRIX boxARotate = XMMatrixRotationY(mBoxAngle);
	XMStoreFloat4x4(&mBoxAWorld, boxARotate);

	// Box B 부분
	mBoxBOrbitAngle += XMConvertToRadians(5.0f) * dt;
	if (mBoxBOrbitAngle >= XM_2PI)
		mBoxBOrbitAngle -= XM_2PI;

	mBoxBRotateAngle += XMConvertToRadians(3.0f) * dt;
	if (mBoxBRotateAngle >= XM_2PI)
		mBoxBRotateAngle -= XM_2PI;

	float radius = 50.0f;
	float bX = radius * cosf(mBoxBOrbitAngle);
	float bZ = radius * sinf(mBoxBOrbitAngle);

	XMMATRIX boxBRotate = XMMatrixRotationY(mBoxBRotateAngle);
	XMMATRIX boxBTranslate = XMMatrixTranslation(bX, 0.0f, bZ);
	XMMATRIX boxBWorld = boxBRotate * boxBTranslate;
	XMStoreFloat4x4(&mBoxBWorld, boxBWorld);
}

void CKDX_LightingGeo::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(InputLayouts::Basic32);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	// 자연광이 있어야 보이니 자연광은 냅두기
	dummyLight.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	dummyLight.Diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	dummyLight.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	Effects::BasicFX->SetDirLights(dummyLight);
	Effects::BasicFX->SetEyePosW(mEyePosW);

	ID3DX11EffectTechnique* activeTech = Effects::BasicFX->LightTech;
	D3DX11_TECHNIQUE_DESC techDesc;
	activeTech->GetDesc(&techDesc);

	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		// ========= A Box =========
		// Box A 렌더링
		md3dImmediateContext->IASetVertexBuffers(0, 1, &mBoxAVB, &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mBoxAIB, DXGI_FORMAT_R32_UINT, 0);

		XMMATRIX boxAWorld = XMLoadFloat4x4(&mBoxAWorld);
		XMMATRIX boxAWorldInvTranspose = MathHelper::InverseTranspose(boxAWorld);
		XMMATRIX boxAWorldViewProj = boxAWorld * view * proj;

		Effects::BasicFX->SetWorld(boxAWorld);
		Effects::BasicFX->SetWorldInvTranspose(boxAWorldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(boxAWorldViewProj);

		// Box A 공전에 따른 색상 변경
		Material boxAMat;
		float boxADegree = XMConvertToDegrees(mBoxAngle);

		if (boxADegree < 180.0f)
		{
			boxAMat.Ambient = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
			boxAMat.Diffuse = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
		}
		else
		{
			boxAMat.Ambient = XMFLOAT4(1.0f, 0.5f, 0.2f, 1.0f);
			boxAMat.Diffuse = XMFLOAT4(1.0f, 0.5f, 0.2f, 1.0f);
		}
		boxAMat.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		Effects::BasicFX->SetMaterial(boxAMat);

		activeTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
		md3dImmediateContext->DrawIndexed(mBoxAIndexCount, 0, 0);


		// ========= B Box =========
		// Box B 렌더링
		md3dImmediateContext->IASetVertexBuffers(0, 1, &mBoxBVB, &stride, &offset);
		md3dImmediateContext->IASetIndexBuffer(mBoxBIB, DXGI_FORMAT_R32_UINT, 0);

		XMMATRIX boxBWorld = XMLoadFloat4x4(&mBoxBWorld);
		XMMATRIX boxBWorldInvTranspose = MathHelper::InverseTranspose(boxBWorld);
		XMMATRIX boxBWorldViewProj = boxBWorld * view * proj;

		Effects::BasicFX->SetWorld(boxBWorld);
		Effects::BasicFX->SetWorldInvTranspose(boxBWorldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(boxBWorldViewProj);

		// Box B 공전에 따른 색상 변경
		Material boxBMat;
		float boxBDegree = XMConvertToDegrees(mBoxBOrbitAngle);

		if (boxBDegree < 180.0f)
		{
			boxBMat.Ambient = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
			boxBMat.Diffuse = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
		}
		else
		{
			boxBMat.Ambient = XMFLOAT4(0.5f, 0.8f, 1.0f, 1.0f);
			boxBMat.Diffuse = XMFLOAT4(0.5f, 0.8f, 1.0f, 1.0f);
		}
		boxBMat.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		Effects::BasicFX->SetMaterial(boxBMat);

		activeTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
		md3dImmediateContext->DrawIndexed(mBoxBIndexCount, 0, 0);
	}

	HR(mSwapChain->Present(0, 0));
}

void CKDX_LightingGeo::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void CKDX_LightingGeo::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CKDX_LightingGeo::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;

		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		float dx = 0.2f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.2f * static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;

		mRadius = MathHelper::Clamp(mRadius, 50.0f, 500.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void CKDX_LightingGeo::BuildGeometryBuffers()
{
	GeometryGenerator geoGen;

	// ========= Box A 생성 =========
	GeometryGenerator::MeshData boxAGeo;
	geoGen.CreateBox(10.0f, 10.0f, 10.0f, boxAGeo);

	mBoxAIndexCount = (UINT)boxAGeo.Indices.size();

	std::vector<Vertex::Basic32> boxAVertices(boxAGeo.Vertices.size());
	for (size_t i = 0; i < boxAGeo.Vertices.size(); ++i)
	{
		boxAVertices[i].Pos = boxAGeo.Vertices[i].Position;
		boxAVertices[i].Normal = boxAGeo.Vertices[i].Normal;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * boxAVertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &boxAVertices[0];
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxAVB));

	std::vector<UINT> boxAIndices;
	boxAIndices.insert(boxAIndices.end(), boxAGeo.Indices.begin(), boxAGeo.Indices.end());

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * boxAIndices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &boxAIndices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxAIB));

	// ========= Box B 생성 =========
	GeometryGenerator::MeshData boxBGeo;
	geoGen.CreateBox(2.0f, 2.0f, 2.0f, boxBGeo);

	mBoxBIndexCount = (UINT)boxBGeo.Indices.size();

	std::vector<Vertex::Basic32> boxBVertices(boxBGeo.Vertices.size());
	for (size_t i = 0; i < boxBGeo.Vertices.size(); ++i)
	{
		boxBVertices[i].Pos = boxBGeo.Vertices[i].Position;
		boxBVertices[i].Normal = boxBGeo.Vertices[i].Normal;
	}

	vbd.ByteWidth = sizeof(Vertex::Basic32) * boxBVertices.size();
	vinitData.pSysMem = &boxBVertices[0];
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxBVB));

	std::vector<UINT> boxBIndices;
	boxBIndices.insert(boxBIndices.end(), boxBGeo.Indices.begin(), boxBGeo.Indices.end());

	ibd.ByteWidth = sizeof(UINT) * boxBIndices.size();
	iinitData.pSysMem = &boxBIndices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxBIB));
}

void CKDX_LightingGeo::BuildFX()
{
	Effects::InitAll(md3dDevice);
}

void CKDX_LightingGeo::BuildVertexLayout()
{
	InputLayouts::InitAll(md3dDevice);
}