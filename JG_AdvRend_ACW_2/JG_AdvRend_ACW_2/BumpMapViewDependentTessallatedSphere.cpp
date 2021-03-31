#include "pch.h"
#include "BumpMapViewDependentTessallatedSphere.h"

BumpMapViewDependentTessallatedSphere::BumpMapViewDependentTessallatedSphere(const shared_ptr<DeviceResources>& device)
	: _device(device), _position(-3.0f, 3.0f, 5.0f), _rotation(0.0f, 0.0f, 0.0f), _scale(1/6000.0f, 1 / 6000.0f, 1 / 6000.0f), _loadingComplete(false), _indexCount(0)
{
	CreateDeviceDependentResources();
}

void BumpMapViewDependentTessallatedSphere::CreateDeviceDependentResources()
{
	// Load shaders asynchronously.
	auto loadVSTask = ReadDataAsync(L"VS_BumpMapViewDependentTessallatedSphere.cso");
	auto loadPSTask = ReadDataAsync(L"PS_BumpMapViewDependentTessellatedSphere.cso");
	auto loadHSTask = ReadDataAsync(L"HS_BumpMapViewDependentTessellatedSphere.cso");
	auto loadDSTask = ReadDataAsync(L"DS_BumpMapViewDependentTessellatedSphere.cso");
	auto loadGSTask = ReadDataAsync(L"GS_BumpMapViewDependentTessellatedSphere.cso");

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
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

		CD3D11_BUFFER_DESC cameraBufferDesc(sizeof(CameraPositionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		ThrowIfFailed(_device->GetD3DDevice()->CreateBuffer(&cameraBufferDesc, nullptr, &_cameraBuffer));

		CD3D11_BUFFER_DESC displacementBufferDesc(sizeof(DisplacementPowerConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		ThrowIfFailed(_device->GetD3DDevice()->CreateBuffer(&displacementBufferDesc, nullptr, &_displacementBuffer));
		
		D3D11_RASTERIZER_DESC raster = CD3D11_RASTERIZER_DESC(D3D11_DEFAULT);
		raster.CullMode = D3D11_CULL_NONE;
		raster.FillMode = D3D11_FILL_SOLID;
		ThrowIfFailed(_device->GetD3DDevice()->CreateRasterizerState(&raster, _rasterState.GetAddressOf()));

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
			0,2,1,1,
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

		DX::ThrowIfFailed(CreateDDSTextureFromFile(_device->GetD3DDevice(), L"earth.dds", nullptr, &_colourTexture));
	});

	(createSphere).then([this]() {
		_loadingComplete = true;
		});
}

void BumpMapViewDependentTessallatedSphere::SetViewProjectionMatrixCB(XMMATRIX& view, XMMATRIX& projection)
{
	XMStoreFloat4x4(&_mvpBufferData.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&_mvpBufferData.projection, XMMatrixTranspose(projection));
}

void BumpMapViewDependentTessallatedSphere::SetCameraPositionCB(XMFLOAT3& position)
{
	_cameraBufferData.position = position;
}

void BumpMapViewDependentTessallatedSphere::SetDisplacementPowerCB(const float displacement)
{
	_displacementBufferData.displacementPower = displacement;
}

void BumpMapViewDependentTessallatedSphere::ReleaseDeviceDependentResources()
{
	_loadingComplete = false;
	if (_vertexShader) _vertexShader.Reset();
	if (_hullShader) _hullShader.Reset();
	if (_domainShader) _domainShader.Reset();
	if (_geometryShader) _geometryShader.Reset();
	if (_pixelShader) _pixelShader.Reset();
	if (_inputLayout) _inputLayout.Reset();
	if (_mvpBuffer) _mvpBuffer.Reset();
	if (_cameraBuffer) _cameraBuffer.Reset();
	if (_vertexBuffer) _vertexBuffer.Reset();
	if (_indexBuffer) _indexBuffer.Reset();
	if (_rasterState) _rasterState.Reset();
	if (_displacementBuffer) _displacementBuffer.Reset();
}

void BumpMapViewDependentTessallatedSphere::Update(StepTimer const& timer)
{
	auto worldMatrix = XMMatrixIdentity();

	_rotation.x += -0.1f * timer.GetElapsedSeconds();
	_rotation.y += 0.1f * timer.GetElapsedSeconds();

	worldMatrix = XMMatrixMultiply(worldMatrix, DirectX::XMMatrixScaling(_scale.x, _scale.y, _scale.z));
	worldMatrix = XMMatrixMultiply(worldMatrix, DirectX::XMMatrixRotationQuaternion(DirectX::XMQuaternionRotationRollPitchYaw(_rotation.x, _rotation.y, _rotation.z)));
	worldMatrix = XMMatrixMultiply(worldMatrix, DirectX::XMMatrixTranslation(_position.x, _position.y, _position.z));

	XMStoreFloat4x4(&_mvpBufferData.model, XMMatrixTranspose(worldMatrix));

	// Update light location if needed
}

void BumpMapViewDependentTessallatedSphere::Render()
{
	if (!_loadingComplete) return;

	auto context = _device->GetD3DDeviceContext();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
	context->RSSetState(_rasterState.Get());
	
	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(_mvpBuffer.Get(), 0, NULL, &_mvpBufferData, 0, 0, 0);
	context->UpdateSubresource1(_cameraBuffer.Get(), 0, NULL, &_cameraBufferData, 0, 0, 0);
	context->UpdateSubresource1(_displacementBuffer.Get(), 0, NULL, &_displacementBufferData, 0, 0, 0);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionColor);
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
	context->DSSetConstantBuffers1(1, 1, _displacementBuffer.GetAddressOf(), nullptr, nullptr);
	
	// Draw the objects.
	context->DrawIndexed(_indexCount, 0, 0);
}
