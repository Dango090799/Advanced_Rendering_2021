#include "pch.h"
#include "Rocks.h"

Rocks::Rocks(const shared_ptr<DeviceResources>& device)
	: _device(device), _loadingComplete(false), _indexCount(0)
{
	CreateDeviceDependentResources();
}

void Rocks::CreateDeviceDependentResources()
{
	auto loadVSTask = ReadDataAsync(L"VS_Rocks.cso");
	auto loadPSTask = ReadDataAsync(L"PS_Rocks.cso");
	auto loadGSTask = ReadDataAsync(L"GS_Rocks.cso");

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

		D3D11_RASTERIZER_DESC raster = CD3D11_RASTERIZER_DESC(D3D11_DEFAULT);
		raster.CullMode = D3D11_CULL_NONE;
		raster.FillMode = D3D11_FILL_SOLID;
		ThrowIfFailed(_device->GetD3DDevice()->CreateRasterizerState(&raster, _rasterState.GetAddressOf()));
		});

	// Once both shaders are loaded, create the mesh.
	auto createSphere = (createPSTask && createVSTask && createGSTask).then([this]() {
		// Load mesh vertices. Each vertex has a position and a color.
		static VertexPosition pointVertices[400000] = {};

		//Density
		auto spacing = 0.2f;
		auto count = 0;

		auto rangeX = 5.0f;
		auto rangeZ = 5.0f;
		auto minRandY = 0.015;
		auto maxRandY = 0.025;

		for (float x = -rangeX; x <= rangeX; x = x + spacing)
		{
			for (float z = -rangeZ; z <= rangeZ; z = z + spacing)
			{
				auto randX = static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / spacing));
				auto randY = minRandY + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / ((maxRandY)-minRandY)));
				auto randZ = static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / spacing));
				pointVertices[count].position = DirectX::XMFLOAT3(x + randX, -0.025f + randY, z + randZ);
				count++;
			}
		}

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = pointVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;

		CD3D11_BUFFER_DESC vertexBufferDescription(sizeof(pointVertices), D3D11_BIND_VERTEX_BUFFER);

		DX::ThrowIfFailed(_device->GetD3DDevice()->CreateBuffer(&vertexBufferDescription, &vertexBufferData, &_vertexBuffer));

		static unsigned int pointIndices[400000] = {};

		for (unsigned int i = 0; i < 400000; i++)
		{
			pointIndices[i] = i;
		}

		_indexCount = ARRAYSIZE(pointIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = pointIndices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;

		CD3D11_BUFFER_DESC indexBufferDescription(sizeof(pointIndices), D3D11_BIND_INDEX_BUFFER);

		DX::ThrowIfFailed(_device->GetD3DDevice()->CreateBuffer(&indexBufferDescription, &indexBufferData, &_indexBuffer));
	});

	(createSphere).then([this]() {
		_loadingComplete = true;
	});
}

void Rocks::SetViewProjectionMatrixCB(XMMATRIX& view, XMMATRIX& projection)
{
	XMStoreFloat4x4(&_mvpBufferData.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&_mvpBufferData.projection, XMMatrixTranspose(projection));
}

void Rocks::SetCameraPositionCB(XMFLOAT3& position)
{
	_cameraBufferData.position = position;
}

void Rocks::ReleaseDeviceDependentResources()
{
	_loadingComplete = false;
	if (_vertexShader) _vertexShader.Reset();
	if (_geometryShader) _geometryShader.Reset();
	if (_pixelShader) _pixelShader.Reset();
	if (_inputLayout) _inputLayout.Reset();
	if (_mvpBuffer) _mvpBuffer.Reset();
	if (_vertexBuffer) _vertexBuffer.Reset();
	if (_indexBuffer) _indexBuffer.Reset();
	if (_rasterState) _rasterState.Reset();
}

void Rocks::Update(StepTimer const& timer)
{
	XMStoreFloat4x4(&_mvpBufferData.model, XMMatrixTranspose(XMMatrixIdentity()));
}

void Rocks::Render()
{
	if (!_loadingComplete) return;

	auto context = _device->GetD3DDeviceContext();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	context->RSSetState(_rasterState.Get());

	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(_mvpBuffer.Get(), 0, NULL, &_mvpBufferData, 0, 0, 0);
	context->UpdateSubresource1(_cameraBuffer.Get(), 0, NULL, &_cameraBufferData, 0, 0, 0);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetInputLayout(_inputLayout.Get());
	
	// Attach our vertex shader.
	context->VSSetShader(_vertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, _mvpBuffer.GetAddressOf(), nullptr, nullptr);
	
	// Attach our pixel shader.
	context->PSSetShader(_pixelShader.Get(), nullptr, 0);
	context->PSSetConstantBuffers1(0, 1, _mvpBuffer.GetAddressOf(), nullptr, nullptr);

	context->GSSetShader(_geometryShader.Get(), nullptr, 0);
	context->GSSetConstantBuffers1(0, 1, _mvpBuffer.GetAddressOf(), nullptr, nullptr);
	context->GSSetConstantBuffers1(2, 1, _cameraBuffer.GetAddressOf(), nullptr, nullptr);

	context->HSSetShader(nullptr, nullptr, 0);
	context->DSSetShader(nullptr, nullptr, 0);

	// Draw the objects.
	context->DrawIndexed(_indexCount, 0, 0);
}
