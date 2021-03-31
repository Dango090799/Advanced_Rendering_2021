#include "pch.h"
#include "StarySky.h"

StarySky::StarySky(const shared_ptr<DeviceResources>& device)
	: _device(device), _loadingComplete(false), _indexCount(0)
{
	CreateDeviceDependentResources();
}

void StarySky::CreateDeviceDependentResources()
{
	// Load shaders asynchronously.
	auto loadVSTask = ReadDataAsync(L"VS_StarySky.cso");
	auto loadPSTask = ReadDataAsync(L"PS_StarySky.cso");
	auto loadGSTask = ReadDataAsync(L"GS_StarySky.cso");
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
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const vector<byte>& fileData) {
		ThrowIfFailed(
			_device->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&_pixelShader
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		ThrowIfFailed(
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
	});
	auto createGSTask = loadGSTask.then([this](const vector<byte>& fileData)
		{
			ThrowIfFailed(
				_device->GetD3DDevice()->CreateGeometryShader(
					&fileData[0],
					fileData.size(),
					nullptr,
					&_geometryShader
				)
			);
	});
	// Once both shaders are loaded, create the mesh.
	auto createSky = (createPSTask && createVSTask && createGSTask).then([this]() {

		// Load mesh vertices. Each vertex has a position and a color.
		array<VertexPositionColor, numStars> skyVertices;
		for (auto i = 0; i < numStars; i++)
		{
			skyVertices[i] = VertexPositionColor({ XMFLOAT3(0.0f, 0.0f, 0.0001f * i), XMFLOAT3(1.0f, 1.0f, 1.0f) });
		}

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = skyVertices.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(skyVertices), D3D11_BIND_VERTEX_BUFFER);
		ThrowIfFailed(
			_device->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&_vertexBuffer
			)
		);

		array<std::uint16_t, numStars> skyIndices;
		for (auto i = 0; i < numStars; i++)
		{
			skyIndices[i] = i;
		}

		_indexCount = skyIndices.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = skyIndices.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(skyIndices), D3D11_BIND_INDEX_BUFFER);
		ThrowIfFailed(
			_device->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&_indexBuffer
			)
		);
	});
	
	(createSky).then([this]() {
		_loadingComplete = true;
	});
}

void StarySky::SetViewProjectionMatrixCB(XMMATRIX& view, XMMATRIX& projection)
{
	XMStoreFloat4x4(&_mvpBufferData.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&_mvpBufferData.projection, XMMatrixTranspose(projection));
}

void StarySky::ReleaseDeviceDependentResources()
{
	_loadingComplete = false;
	if(_vertexShader) _vertexShader.Reset();
	if(_geometryShader) _geometryShader.Reset();
	if(_pixelShader) _pixelShader.Reset();
	if(_inputLayout) _inputLayout.Reset();
	if(_mvpBuffer) _mvpBuffer.Reset();
	if(_vertexBuffer) _vertexBuffer.Reset();
	if(_indexBuffer) _indexBuffer.Reset();
	if(_rasterState) _rasterState.Reset();
}

void StarySky::Update(StepTimer const& timer)
{
	auto worldMatrix = XMMatrixIdentity();
	XMStoreFloat4x4(&_mvpBufferData.model, XMMatrixTranspose(worldMatrix));
}

void StarySky::Render()
{
	if (!_loadingComplete) return;

	auto context = _device->GetD3DDeviceContext();

	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(_mvpBuffer.Get(), 0, NULL, &_mvpBufferData, 0, 0, 0);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	context->IASetInputLayout(_inputLayout.Get());
	// Attach our vertex shader.
	context->VSSetShader(_vertexShader.Get(), nullptr, 0);
	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(0, 1, _mvpBuffer.GetAddressOf(), nullptr, nullptr);

	context->RSSetState(_rasterState.Get());
	
	// Attach our pixel shader.
	context->PSSetShader(_pixelShader.Get(), nullptr, 0);
	context->GSSetShader(_geometryShader.Get(), nullptr, 0);
	context->GSSetConstantBuffers1(0, 1, _mvpBuffer.GetAddressOf(), nullptr, nullptr);

	context->HSSetShader(nullptr, nullptr, 0);
	context->DSSetShader(nullptr, nullptr, 0);

	// Draw the objects.
	context->DrawIndexed(_indexCount, 0, 0);
}