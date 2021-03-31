#pragma once
#include "..\Common\DeviceResources.h"
#include "..\Common\DirectXHelper.h"
#include "..\Common\StepTimer.h"
#include "..\Content\ShaderStructures.h"
#include "DDSTextureLoader.h"

using namespace DX;
using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace JG_AdvRend_ACW_2;

class Meteors
{
public: // Structors
	Meteors(const shared_ptr<DeviceResources>&, XMFLOAT3&, XMFLOAT3&);
public: // Accessors
	XMFLOAT3 GetPosition() const;
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
	ComPtr<ID3D11HullShader> _hullShader;
	ComPtr<ID3D11DomainShader> _domainShader;
	ComPtr<ID3D11GeometryShader> _geometryShader;
	ComPtr<ID3D11PixelShader> _pixelShader;
	ComPtr<ID3D11RasterizerState> _rasterState;
	ComPtr<ID3D11Buffer> _mvpBuffer;
	ComPtr<ID3D11Buffer> _cameraBuffer;
	ComPtr<ID3D11Buffer> _timeBuffer;

	ID3D11SamplerState* _samplerState;
	ID3D11ShaderResourceView* _colourTexture;
	// Might need time variable for tail animation? Could use implicit modelling for this... like with the ray traced?

	ModelViewProjectionConstantBuffer _mvpBufferData;
	CameraPositionConstantBuffer _cameraBufferData;
	TimeConstantBuffer _timeBufferData;
	
	int _indexCount;
	bool _loadingComplete;

	XMFLOAT3 _position, _rotation, _scale;
	XMFLOAT3 _movement;

	// Variables for tail of meteor
	
};

inline XMFLOAT3 Meteors::GetPosition() const { return _position; }