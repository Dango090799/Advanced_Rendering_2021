#include "pch.h"
#include "RayMarchObjects.h"

RayMarchObjects::RayMarchObjects(const shared_ptr<DeviceResources>& device)
	: _device(device), _loadingComplete(false), _indexCount(0)
{
	CreateDeviceDependentResources();
}

void RayMarchObjects::CreateDeviceDependentResources()
{
	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(L"VS_RayMarchObjects.cso");
	auto loadPSTask = DX::ReadDataAsync(L"PS_RayMarchObjects.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			_device->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&_vertexShader
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		DX::ThrowIfFailed(
			_device->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&_inputLayout
			)
		);
		});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			_device->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&_pixelShader
			)
		);

		CD3D11_BUFFER_DESC MVPBufferDescription(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);

		DX::ThrowIfFailed(_device->GetD3DDevice()->CreateBuffer(&MVPBufferDescription, nullptr, &_mvpBuffer));

		CD3D11_BUFFER_DESC cameraBufferDescription(sizeof(CameraPositionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);

		DX::ThrowIfFailed(_device->GetD3DDevice()->CreateBuffer(&cameraBufferDescription, nullptr, &_cameraBuffer));

		D3D11_RASTERIZER_DESC raster = CD3D11_RASTERIZER_DESC(D3D11_DEFAULT);
		raster.CullMode = D3D11_CULL_NONE;
		raster.FillMode = D3D11_FILL_SOLID;
		DX::ThrowIfFailed(_device->GetD3DDevice()->CreateRasterizerState(&raster, _rasterState.GetAddressOf()));
		});

	// Once both shaders are loaded, create the mesh.
	auto createGrassPoints = (createPSTask && createVSTask).then([this]() {

		// Load mesh vertices. Each vertex has a position and a color.
		static const VertexPosition quadVertices[] =
		{
			{DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f)},
			{DirectX::XMFLOAT3(-0.5f, 0.5f,  0.0f)},
			{DirectX::XMFLOAT3(0.5f,  -0.5f, 0.0f)},
			{DirectX::XMFLOAT3(0.5f,  0.5f,  0.0f)},
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = quadVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(quadVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			_device->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&_vertexBuffer
			)
		);

		static const unsigned short quadIndices[] =
		{
			0, 1, 2,
			3, 2, 1
		};

		_indexCount = ARRAYSIZE(quadIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = quadIndices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(quadIndices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			_device->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&_indexBuffer
			)
		);
		});

	// Once the cube is loaded, the object is ready to be rendered.
	createGrassPoints.then([this]() {
		_loadingComplete = true;
		});
}

void RayMarchObjects::SetViewProjectionMatrixCB(XMMATRIX& view, XMMATRIX& projection)
{
	XMStoreFloat4x4(&_mvpBufferData.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&_mvpBufferData.projection, XMMatrixTranspose(projection));
}

void RayMarchObjects::SetCameraPositionCB(XMFLOAT3& position)
{
	_cameraBufferData.position = position;
}

void RayMarchObjects::ReleaseDeviceDependentResources()
{
	_loadingComplete = false;
	if (_vertexShader) _vertexShader.Reset();
	if (_pixelShader) _pixelShader.Reset();
	if (_inputLayout) _inputLayout.Reset();
	if (_mvpBuffer) _mvpBuffer.Reset();
	if (_vertexBuffer) _vertexBuffer.Reset();
	if (_indexBuffer) _indexBuffer.Reset();
	if (_rasterState) _rasterState.Reset();
}

void RayMarchObjects::Update(StepTimer const& timer)
{
	XMStoreFloat4x4(&_mvpBufferData.model, XMMatrixTranspose(XMMatrixIdentity()));
}

void RayMarchObjects::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!_loadingComplete)
	{
		return;
	}

	auto context = _device->GetD3DDeviceContext();

	// Prepare constant buffers to send it to the graphics device.
	context->UpdateSubresource1(_mvpBuffer.Get(), 0, NULL, &_mvpBufferData, 0, 0, 0);
	context->UpdateSubresource1(_cameraBuffer.Get(), 0, NULL, &_cameraBufferData, 0, 0, 0);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPosition);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(_inputLayout.Get());

	// Attach our vertex shader.
	context->VSSetShader(_vertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, _mvpBuffer.GetAddressOf(), nullptr, nullptr);

	context->HSSetShader(nullptr, nullptr, 0);

	context->DSSetShader(nullptr, nullptr, 0);

	context->GSSetShader(nullptr, nullptr, 0);

	context->RSSetState(_rasterState.Get());

	context->PSSetConstantBuffers1(0, 1, _mvpBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetConstantBuffers1(1, 1, _cameraBuffer.GetAddressOf(), nullptr, nullptr);
	// Attach our pixel shader.
	context->PSSetShader(_pixelShader.Get(), nullptr, 0);

	// Draw the objects.
	context->DrawIndexed(_indexCount, 0, 0);
}
