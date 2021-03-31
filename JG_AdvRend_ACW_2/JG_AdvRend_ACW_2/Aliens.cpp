#include "pch.h"
#include "Aliens.h"

Aliens::Aliens(const shared_ptr<DeviceResources>& device, const shared_ptr<ResourceManager>& resource, XMFLOAT3& position, XMFLOAT3& rotation)
	: _device(device), _resourceManager(resource), _position(position), _rotation(rotation), _scale(1.0f, 1.0f, 1.0f), _loadingComplete(false), _indexCount(0)
{
	CreateDeviceDependentResources();
}

void Aliens::CreateDeviceDependentResources()
{
	auto loadVSTask = ReadDataAsync(L"VS_Aliens.cso");
	auto loadPSTask = ReadDataAsync(L"PS_Aliens.cso");
	auto loadHSTask = ReadDataAsync(L"HS_Aliens.cso");
	auto loadDSTask = ReadDataAsync(L"DS_Aliens.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const vector<byte>& fileData) {
		ThrowIfFailed(
			_device->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&_vertexShader
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		ThrowIfFailed(
			_device->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&_inputLayout
			)
		);
		});

	auto createHSTask = loadHSTask.then([this](const vector<byte>& fileData)
		{
			DX::ThrowIfFailed(
				_device->GetD3DDevice()->CreateHullShader(
					&fileData[0],
					fileData.size(),
					nullptr,
					&_hullShader
				)
			);
		});

	auto createDSTask = loadDSTask.then([this](const vector<byte>& fileData)
		{
			DX::ThrowIfFailed(
				_device->GetD3DDevice()->CreateDomainShader(
					&fileData[0],
					fileData.size(),
					nullptr,
					&_domainShader
				)
			);
		});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const vector<byte>& fileData) {
		DX::ThrowIfFailed(
			_device->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&_pixelShader
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			_device->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&_mvpBuffer
			)
		);

		CD3D11_BUFFER_DESC cameraBufferDesc(sizeof(CameraPositionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		ThrowIfFailed(_device->GetD3DDevice()->CreateBuffer(&cameraBufferDesc, nullptr, &_cameraBuffer));

		CD3D11_BUFFER_DESC timeBufferDesc(sizeof(TimeConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		ThrowIfFailed(_device->GetD3DDevice()->CreateBuffer(&timeBufferDesc, nullptr, &_timeBuffer));

		CD3D11_BUFFER_DESC offsetBufferDesc(sizeof(OffsetConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		ThrowIfFailed(_device->GetD3DDevice()->CreateBuffer(&offsetBufferDesc, nullptr, &_offsetBuffer));
		_offsetBufferData.position = _position;
		_offsetBufferData.rotation = _rotation;
		_offsetBufferData.scale = _scale;
		
		D3D11_RASTERIZER_DESC raster = CD3D11_RASTERIZER_DESC(D3D11_DEFAULT);
		raster.CullMode = D3D11_CULL_NONE;
		raster.FillMode = D3D11_FILL_SOLID;
		ThrowIfFailed(_device->GetD3DDevice()->CreateRasterizerState(&raster, _rasterState.GetAddressOf()));
		});

	// Once both shaders are loaded, create the mesh.
	auto createSphere = (createPSTask && createVSTask && createHSTask && createDSTask).then([this]() {
		ThrowIfFailed(
			_resourceManager->LoadModel(
				_device,
				"plane.obj",
				_indexCount,
				_vertexBuffer,
				_indexBuffer
			)
		);
	});

	(createSphere).then([this]() {
		_loadingComplete = true;
		});
}

void Aliens::SetViewProjectionMatrixCB(XMMATRIX& view, XMMATRIX& projection)
{
	XMStoreFloat4x4(&_mvpBufferData.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&_mvpBufferData.projection, XMMatrixTranspose(projection));
}

void Aliens::SetCameraPositionCB(XMFLOAT3& position)
{
	_cameraBufferData.position = position;
}

void Aliens::ReleaseDeviceDependentResources()
{
	_loadingComplete = false;
	if (_vertexShader) _vertexShader.Reset();
	if (_pixelShader) _pixelShader.Reset();
	if (_inputLayout) _inputLayout.Reset();
	if (_mvpBuffer) _mvpBuffer.Reset();
	if (_vertexBuffer) _vertexBuffer.Reset();
	if (_indexBuffer) _indexBuffer.Reset();
	if (_rasterState) _rasterState.Reset();
	if (_cameraBuffer) _cameraBuffer.Reset();
	if (_timeBuffer) _timeBuffer.Reset();
	if (_offsetBuffer) _offsetBuffer.Reset();
}

void Aliens::Update(StepTimer const& timer)
{
	auto worldMatrix = XMMatrixIdentity();
	
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixScaling(_scale.x, _scale.y, _scale.z));
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(_rotation.x, _rotation.y, _rotation.z)));
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixTranslation(_position.x, _position.y, _position.z));

	XMStoreFloat4x4(&_mvpBufferData.model, XMMatrixTranspose(worldMatrix));

	_timeBufferData.time = timer.GetTotalSeconds();
}

void Aliens::Render()
{
	if (!_loadingComplete) return;

	auto context = _device->GetD3DDeviceContext();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	context->RSSetState(_rasterState.Get());

	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(_mvpBuffer.Get(), 0, NULL, &_mvpBufferData, 0, 0, 0);
	context->UpdateSubresource1(_cameraBuffer.Get(), 0, NULL, &_cameraBufferData, 0, 0, 0);
	context->UpdateSubresource1(_timeBuffer.Get(), 0, NULL, &_timeBufferData, 0, 0, 0);
	context->UpdateSubresource1(_offsetBuffer.Get(), 0, NULL, &_offsetBufferData, 0, 0, 0);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionTexcoordNormalTangentBinormal);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetInputLayout(_inputLayout.Get());
	// Attach our vertex shader.
	context->VSSetShader(_vertexShader.Get(), nullptr, 0);
	
	// Attach our pixel shader.
	context->PSSetShader(_pixelShader.Get(), nullptr, 0);
	context->PSSetConstantBuffers1(0, 1, _mvpBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetConstantBuffers1(1, 1, _cameraBuffer.GetAddressOf(), nullptr, nullptr);
	
	context->GSSetShader(nullptr, nullptr, 0);

	context->HSSetShader(_hullShader.Get(), nullptr, 0);

	context->DSSetShader(_domainShader.Get(), nullptr, 0);
	context->DSSetConstantBuffers1(0, 1, _mvpBuffer.GetAddressOf(), nullptr, nullptr);
	context->DSSetConstantBuffers1(1, 1, _cameraBuffer.GetAddressOf(), nullptr, nullptr);
	context->DSSetConstantBuffers1(2, 1, _timeBuffer.GetAddressOf(), nullptr, nullptr);
	context->DSSetConstantBuffers1(3, 1, _offsetBuffer.GetAddressOf(), nullptr, nullptr);
	
	// Draw the objects.
	context->DrawIndexed(_indexCount, 0, 0);
}
