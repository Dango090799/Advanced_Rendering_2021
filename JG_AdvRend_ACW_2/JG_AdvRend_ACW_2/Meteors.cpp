#include "pch.h"
#include "Meteors.h"

Meteors::Meteors(const shared_ptr<DeviceResources>& device, XMFLOAT3& position, XMFLOAT3& movement)
	: _device(device), _position(position), _rotation(0.0f, 0.0f, 0.0f), _scale(0.05f, 0.05f, 0.05f), _movement(movement), _loadingComplete(false), _indexCount(0)
{
	CreateDeviceDependentResources();
}

void Meteors::CreateDeviceDependentResources()
{
	// Load shaders asynchronously.
	auto loadVSTask = ReadDataAsync(L"VS_Meteors.cso");
	auto loadPSTask = ReadDataAsync(L"PS_Meteors.cso");
	auto loadHSTask = ReadDataAsync(L"HS_Meteors.cso");
	auto loadDSTask = ReadDataAsync(L"DS_Meteors.cso");
	auto loadGSTask = ReadDataAsync(L"GS_Meteors.cso");

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
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

	auto createGSTask = loadGSTask.then([this](const vector<byte>& fileData)
		{
			DX::ThrowIfFailed(
				_device->GetD3DDevice()->CreateGeometryShader(
					&fileData[0],
					fileData.size(),
					nullptr,
					&_geometryShader
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

		D3D11_RASTERIZER_DESC raster = CD3D11_RASTERIZER_DESC(D3D11_DEFAULT);
		raster.CullMode = D3D11_CULL_NONE;
		raster.FillMode = D3D11_FILL_SOLID;
		ThrowIfFailed(
			_device->GetD3DDevice()->CreateRasterizerState(
				&raster,
				_rasterState.GetAddressOf()
			)
		);

		D3D11_SAMPLER_DESC sampDesc = CD3D11_SAMPLER_DESC(D3D11_DEFAULT);
		DX::ThrowIfFailed(_device->GetD3DDevice()->CreateSamplerState(&sampDesc, &_samplerState));
		});

	// Once both shaders are loaded, create the mesh.
	auto createSphere = (createPSTask && createVSTask && createHSTask && createDSTask && createGSTask).then([this]() {

		// Load mesh vertices. Each vertex has a position and a color.
		static const VertexPositionColor sphereVertices[] =
		{
			{XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f)},
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = sphereVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(sphereVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			_device->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&_vertexBuffer
			)
		);

		static const unsigned short sphereIndices[] = {
			0,2,1, // -x
			1,
		};
		_indexCount = ARRAYSIZE(sphereIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = sphereIndices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(sphereIndices), D3D11_BIND_INDEX_BUFFER);
		ThrowIfFailed(
			_device->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&_indexBuffer
			)
		);

		DX::ThrowIfFailed(CreateDDSTextureFromFile(_device->GetD3DDevice(), L"lava_rock.dds", nullptr, &_colourTexture));
		});

	(createSphere).then([this]() {
		_loadingComplete = true;
		});
}

void Meteors::SetViewProjectionMatrixCB(XMMATRIX& view, XMMATRIX& projection)
{
	XMStoreFloat4x4(&_mvpBufferData.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&_mvpBufferData.projection, XMMatrixTranspose(projection));
}

void Meteors::SetCameraPositionCB(XMFLOAT3& position)
{
	_cameraBufferData.position = position;
}

void Meteors::ReleaseDeviceDependentResources()
{
	_loadingComplete = false;
	if (_vertexShader) _vertexShader.Reset();
	if (_pixelShader) _pixelShader.Reset();
	if (_geometryShader) _geometryShader.Reset();
	if (_hullShader) _hullShader.Reset();
	if (_domainShader) _domainShader.Reset();
	if (_inputLayout) _inputLayout.Reset();
	if (_mvpBuffer) _mvpBuffer.Reset();
	if (_vertexBuffer) _vertexBuffer.Reset();
	if (_indexBuffer) _indexBuffer.Reset();
	if (_rasterState) _rasterState.Reset();
	if (_timeBuffer) _timeBuffer.Reset();
	if (_cameraBuffer) _cameraBuffer.Reset();
}

void Meteors::Update(StepTimer const& timer)
{
	auto worldMatrix = XMMatrixIdentity();

	_position.x += _movement.x * timer.GetElapsedSeconds();
	_position.y += _movement.y * timer.GetElapsedSeconds();
	_position.z += _movement.z * timer.GetElapsedSeconds();

	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixScaling(_scale.x, _scale.y, _scale.z));
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(_rotation.x, _rotation.y, _rotation.z)));
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixTranslation(_position.x, _position.y, _position.z));

	XMStoreFloat4x4(&_mvpBufferData.model, XMMatrixTranspose(worldMatrix));

	_timeBufferData.time = timer.GetTotalSeconds();
}

void Meteors::Render()
{
	if (!_loadingComplete) return;

	auto context = _device->GetD3DDeviceContext();

	// METEOR SPHERE
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
	context->RSSetState(_rasterState.Get());

	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(_mvpBuffer.Get(), 0, NULL, &_mvpBufferData, 0, 0, 0);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPosition);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetInputLayout(_inputLayout.Get());
	// Attach our vertex shader.
	context->VSSetShader(_vertexShader.Get(), nullptr, 0);
	
	// Attach our pixel shader.
	context->PSSetShader(_pixelShader.Get(), nullptr, 0);
	context->PSSetConstantBuffers1(0, 1, _cameraBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetSamplers(0, 1, &_samplerState);
	context->PSSetShaderResources(0, 1, &_colourTexture);

	context->GSSetShader(_geometryShader.Get(), nullptr, 0);

	context->HSSetShader(_hullShader.Get(), nullptr, 0);

	context->DSSetShader(_domainShader.Get(), nullptr, 0);
	context->DSSetConstantBuffers1(0, 1, _mvpBuffer.GetAddressOf(), nullptr, nullptr);
	context->DSSetConstantBuffers1(1, 1, _cameraBuffer.GetAddressOf(), nullptr, nullptr);

	// Draw the objects.
	context->DrawIndexed(_indexCount, 0, 0);


	// METEOR TAIL
	
}
