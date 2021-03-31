#pragma once
#include "..\Common\DeviceResources.h"
#include "..\Common\DirectXHelper.h"
#include "..\Common\StepTimer.h"
#include "..\Content\ShaderStructures.h"

using namespace DX;
using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace JG_AdvRend_ACW_2;

class RayMarchObjects
{
public: // Structors
	RayMarchObjects(const shared_ptr<DeviceResources>&);
public: // Accessors

public: // Functions
	void CreateDeviceDependentResources();
	void SetViewProjectionMatrixCB(XMMATRIX&, XMMATRIX&);
	void SetCameraPositionCB(XMFLOAT3&);
	void ReleaseDeviceDependentResources();
	void Update(StepTimer const&);
	void Render();

private: // Data
	shared_ptr<DeviceResources> _device;
	ComPtr<ID3D11InputLayout> _inputLayout;
	ComPtr<ID3D11Buffer> _vertexBuffer;
	ComPtr<ID3D11Buffer> _indexBuffer;
	ComPtr<ID3D11VertexShader> _vertexShader;
	ComPtr<ID3D11PixelShader> _pixelShader;
	ComPtr<ID3D11RasterizerState> _rasterState;
	ComPtr<ID3D11Buffer> _mvpBuffer;
	ComPtr<ID3D11Buffer> _cameraBuffer;

	ModelViewProjectionConstantBuffer _mvpBufferData;
	CameraPositionConstantBuffer _cameraBufferData;

	int _indexCount;
	bool _loadingComplete;
};

