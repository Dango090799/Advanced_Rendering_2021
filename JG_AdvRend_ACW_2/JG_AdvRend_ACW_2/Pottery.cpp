#include "pch.h"
#include "Pottery.h"

Pottery::Pottery(const shared_ptr<DeviceResources>& device)
	: _device(device), _position(0.0f, 0.9f, 1.0f), _rotation(-XM_PIDIV2, 0.0f, 0.0f), _scale(0.03f, 0.03f, 0.1f), _loadingComplete(false), _indexCount(0)
{
	CreateDeviceDependentResources();
}

void Pottery::CreateDeviceDependentResources()
{
	auto loadVSTask = ReadDataAsync(L"VS_Pottery.cso");
	auto loadPSTask = ReadDataAsync(L"PS_Pottery.cso");
	auto loadHSTask = ReadDataAsync(L"HS_Pottery.cso");
	auto loadDSTask = ReadDataAsync(L"DS_Pottery.cso");
	auto loadGSTask = ReadDataAsync(L"GS_Pottery.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const vector<byte>& fileData) {
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
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
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
		DX::ThrowIfFailed(_device->GetD3DDevice()->CreateBuffer(&cameraBufferDesc, nullptr, &_cameraBuffer));

		D3D11_RASTERIZER_DESC raster = CD3D11_RASTERIZER_DESC(D3D11_DEFAULT);
		raster.CullMode = D3D11_CULL_NONE;
		raster.FillMode = D3D11_FILL_SOLID;
		DX::ThrowIfFailed(_device->GetD3DDevice()->CreateRasterizerState(&raster, _rasterState.GetAddressOf()));

		D3D11_SAMPLER_DESC sampDesc = CD3D11_SAMPLER_DESC(D3D11_DEFAULT);
		DX::ThrowIfFailed(_device->GetD3DDevice()->CreateSamplerState(&sampDesc, &_samplerState));
		});

	// Once both shaders are loaded, create the mesh.
	auto createSphere = (createPSTask && createVSTask && createHSTask && createDSTask && createGSTask).then([this]() {
		static VertexPosition pointVertices[168] = {
			// Handle
			{ XMFLOAT3(-1.6000, 0.0000, 2.02500)}, { XMFLOAT3(-1.6000, -0.3000, 2.02500)},
			{ XMFLOAT3(-1.5000, -0.3000, 2.25000)}, { XMFLOAT3(-1.5000, 0.0000, 2.25000)},
			{ XMFLOAT3(-2.3000, 0.0000, 2.02500)}, { XMFLOAT3(-2.3000, -0.3000, 2.02500)},
			{ XMFLOAT3(-2.5000, -0.3000, 2.25000)}, { XMFLOAT3(-2.5000, 0.0000, 2.25000)},
			{ XMFLOAT3(-2.7000, 0.0000, 2.02500)}, { XMFLOAT3(-2.7000, -0.3000, 2.02500)},
			{ XMFLOAT3(-3.0000, -0.3000, 2.25000)}, { XMFLOAT3(-3.0000, 0.0000, 2.25000)},
			{ XMFLOAT3(-2.7000, 0.0000, 1.80000)}, { XMFLOAT3(-2.7000, -0.3000, 1.80000)},
			{ XMFLOAT3(-3.0000, -0.3000, 1.80000)}, { XMFLOAT3(-3.0000, 0.0000, 1.80000)},
			{ XMFLOAT3(-2.7000, 0.0000, 1.57500)}, { XMFLOAT3(-2.7000, -0.3000, 1.57500)},
			{ XMFLOAT3(-3.0000, -0.3000, 1.35000)}, { XMFLOAT3(-3.0000, 0.0000, 1.35000)},
			{ XMFLOAT3(-2.5000, 0.0000, 1.12500)}, { XMFLOAT3(-2.5000, -0.3000, 1.12500)},
			{ XMFLOAT3(-2.6500, -0.3000, 0.93750)}, { XMFLOAT3(-2.6500, 0.0000, 0.93750)},
			{ XMFLOAT3(-2.0000, 0.0000, 0.90000)}, { XMFLOAT3(-2.0000, -0.3000, 0.90000)},
			{ XMFLOAT3(-1.9000, -0.3000, 0.60000)}, { XMFLOAT3(-1.9000, 0.0000, 0.60000)},

			{ XMFLOAT3(-1.6000, 0.0000, 2.02500)}, { XMFLOAT3(-1.6000, 0.3000, 2.02500)},
			{ XMFLOAT3(-1.5000, 0.3000, 2.25000)}, { XMFLOAT3(-1.5000, 0.0000, 2.25000)},
			{ XMFLOAT3(-2.3000, 0.0000, 2.02500)}, { XMFLOAT3(-2.3000, 0.3000, 2.02500)},
			{ XMFLOAT3(-2.5000, 0.3000, 2.25000)}, { XMFLOAT3(-2.5000, 0.0000, 2.25000)},
			{ XMFLOAT3(-2.7000, 0.0000, 2.02500)}, { XMFLOAT3(-2.7000, 0.3000, 2.02500)},
			{ XMFLOAT3(-3.0000, 0.3000, 2.25000)}, { XMFLOAT3(-3.0000, 0.0000, 2.25000)},
			{ XMFLOAT3(-2.7000, 0.0000, 1.80000)}, { XMFLOAT3(-2.7000, 0.3000, 1.80000)},
			{ XMFLOAT3(-3.0000, 0.3000, 1.80000)}, { XMFLOAT3(-3.0000, 0.0000, 1.80000)},
			{ XMFLOAT3(-2.7000, 0.0000, 1.57500)}, { XMFLOAT3(-2.7000, 0.3000, 1.57500)},
			{ XMFLOAT3(-3.0000, 0.3000, 1.35000)}, { XMFLOAT3(-3.0000, 0.0000, 1.35000)},
			{ XMFLOAT3(-2.5000, 0.0000, 1.12500)}, { XMFLOAT3(-2.5000, 0.3000, 1.12500)},
			{ XMFLOAT3(-2.6500, 0.3000, 0.93750)}, { XMFLOAT3(-2.6500, 0.0000, 0.93750)},
			{ XMFLOAT3(-2.0000, 0.0000, 0.90000)}, { XMFLOAT3(-2.0000, 0.3000, 0.90000)},
			{ XMFLOAT3(-1.9000, 0.3000, 0.60000)}, { XMFLOAT3(-1.9000, 0.0000, 0.60000)},

			//Body
			{ XMFLOAT3(1.5000,  0.0000, 2.40000)}, { XMFLOAT3(1.5000, -0.8400, 2.40000)},
			{ XMFLOAT3(0.8400, -1.5000, 2.40000)}, { XMFLOAT3(0.0000, -1.5000, 2.40000)},
			{ XMFLOAT3(1.7500,  0.0000, 1.87500)}, { XMFLOAT3(1.7500, -0.9800, 1.87500)},
			{ XMFLOAT3(0.9800, -1.7500, 1.87500)}, { XMFLOAT3(0.0000, -1.7500, 1.87500)},
			{ XMFLOAT3(2.0000,  0.0000, 1.35000)}, { XMFLOAT3(2.0000, -1.1200, 1.35000)},
			{ XMFLOAT3(1.1200, -2.0000, 1.35000)}, { XMFLOAT3(0.0000, -2.0000, 1.35000)},
			{ XMFLOAT3(2.0000,  0.0000, 0.90000)}, { XMFLOAT3(2.0000, -1.1200, 0.90000)},
			{ XMFLOAT3(1.1200, -2.0000, 0.90000)}, { XMFLOAT3(0.0000, -2.0000, 0.90000)},
			{ XMFLOAT3(2.0000,  0.0000, 0.45000)}, { XMFLOAT3(2.0000, -1.1200, 0.45000)},
			{ XMFLOAT3(1.1200, -2.0000, 0.45000)}, { XMFLOAT3(0.0000, -2.0000, 0.45000)},
			{ XMFLOAT3(1.5000,  0.0000, 0.22500)}, { XMFLOAT3(1.5000, -0.8400, 0.22500)},
			{ XMFLOAT3(0.8400, -1.5000, 0.22500)}, { XMFLOAT3(0.0000, -1.5000, 0.22500)},
			{ XMFLOAT3(1.5000,  0.0000, 0.15000)}, { XMFLOAT3(1.5000, -0.8400, 0.15000)},
			{ XMFLOAT3(0.8400, -1.5000, 0.15000)}, { XMFLOAT3(0.0000, -1.5000, 0.15000)},

			{ XMFLOAT3(1.5000,  0.0000, 2.40000) }, { XMFLOAT3(1.5000, 0.8400, 2.40000) },
			{ XMFLOAT3(0.8400, 1.5000, 2.40000) }, { XMFLOAT3(0.0000, 1.5000, 2.40000) },
			{ XMFLOAT3(1.7500,  0.0000, 1.87500) }, { XMFLOAT3(1.7500, 0.9800, 1.87500) },
			{ XMFLOAT3(0.9800, 1.7500, 1.87500) }, { XMFLOAT3(0.0000, 1.7500, 1.87500) },
			{ XMFLOAT3(2.0000,  0.0000, 1.35000) }, { XMFLOAT3(2.0000, 1.1200, 1.35000) },
			{ XMFLOAT3(1.1200, 2.0000, 1.35000) }, { XMFLOAT3(0.0000, 2.0000, 1.35000) },
			{ XMFLOAT3(2.0000,  0.0000, 0.90000) }, { XMFLOAT3(2.0000, 1.1200, 0.90000) },
			{ XMFLOAT3(1.1200, 2.0000, 0.90000) }, { XMFLOAT3(0.0000, 2.0000, 0.90000) },
			{ XMFLOAT3(2.0000,  0.0000, 0.45000) }, { XMFLOAT3(2.0000, 1.1200, 0.45000) },
			{ XMFLOAT3(1.1200, 2.0000, 0.45000) }, { XMFLOAT3(0.0000, 2.0000, 0.45000) },
			{ XMFLOAT3(1.5000,  0.0000, 0.22500) }, { XMFLOAT3(1.5000, 0.8400, 0.22500) },
			{ XMFLOAT3(0.8400, 1.5000, 0.22500) }, { XMFLOAT3(0.0000, 1.5000, 0.22500) },
			{ XMFLOAT3(1.5000,  0.0000, 0.15000) }, { XMFLOAT3(1.5000, 0.8400, 0.15000) },
			{ XMFLOAT3(0.8400, 1.5000, 0.15000) }, { XMFLOAT3(0.0000, 1.5000, 0.15000) },

			{ XMFLOAT3(-1.5000,  0.0000, 2.40000) }, { XMFLOAT3(-1.5000, -0.8400, 2.40000) },
			{ XMFLOAT3(-0.8400, -1.5000, 2.40000) }, { XMFLOAT3(-0.0000, -1.5000, 2.40000) },
			{ XMFLOAT3(-1.7500,  0.0000, 1.87500) }, { XMFLOAT3(-1.7500, -0.9800, 1.87500) },
			{ XMFLOAT3(-0.9800, -1.7500, 1.87500) }, { XMFLOAT3(-0.0000, -1.7500, 1.87500) },
			{ XMFLOAT3(-2.0000,  0.0000, 1.35000) }, { XMFLOAT3(-2.0000, -1.1200, 1.35000) },
			{ XMFLOAT3(-1.1200, -2.0000, 1.35000) }, { XMFLOAT3(-0.0000, -2.0000, 1.35000) },
			{ XMFLOAT3(-2.0000,  0.0000, 0.90000) }, { XMFLOAT3(-2.0000, -1.1200, 0.90000) },
			{ XMFLOAT3(-1.1200, -2.0000, 0.90000) }, { XMFLOAT3(-0.0000, -2.0000, 0.90000) },
			{ XMFLOAT3(-2.0000,  0.0000, 0.45000) }, { XMFLOAT3(-2.0000, -1.1200, 0.45000) },
			{ XMFLOAT3(-1.1200, -2.0000, 0.45000) }, { XMFLOAT3(-0.0000, -2.0000, 0.45000) },
			{ XMFLOAT3(-1.5000,  0.0000, 0.22500) }, { XMFLOAT3(-1.5000, -0.8400, 0.22500) },
			{ XMFLOAT3(-0.8400, -1.5000, 0.22500) }, { XMFLOAT3(-0.0000, -1.5000, 0.22500) },
			{ XMFLOAT3(-1.5000,  0.0000, 0.15000) }, { XMFLOAT3(-1.5000, -0.8400, 0.15000) },
			{ XMFLOAT3(-0.8400, -1.5000, 0.15000) }, { XMFLOAT3(-0.0000, -1.5000, 0.15000) },

			{ XMFLOAT3(-1.5000,  0.0000, 2.40000) }, { XMFLOAT3(-1.5000, 0.8400, 2.40000) },
			{ XMFLOAT3(-0.8400, 1.5000, 2.40000) }, { XMFLOAT3(-0.0000, 1.5000, 2.40000) },
			{ XMFLOAT3(-1.7500,  0.0000, 1.87500) }, { XMFLOAT3(-1.7500, 0.9800, 1.87500) },
			{ XMFLOAT3(-0.9800, 1.7500, 1.87500) }, { XMFLOAT3(-0.0000, 1.7500, 1.87500) },
			{ XMFLOAT3(-2.0000,  0.0000, 1.35000) }, { XMFLOAT3(-2.0000, 1.1200, 1.35000) },
			{ XMFLOAT3(-1.1200, 2.0000, 1.35000) }, { XMFLOAT3(-0.0000, 2.0000, 1.35000) },
			{ XMFLOAT3(-2.0000,  0.0000, 0.90000) }, { XMFLOAT3(-2.0000, 1.1200, 0.90000) },
			{ XMFLOAT3(-1.1200, 2.0000, 0.90000) }, { XMFLOAT3(-0.0000, 2.0000, 0.90000) },
			{ XMFLOAT3(-2.0000,  0.0000, 0.45000) }, { XMFLOAT3(-2.0000, 1.1200, 0.45000) },
			{ XMFLOAT3(-1.1200, 2.0000, 0.45000) }, { XMFLOAT3(-0.0000, 2.0000, 0.45000) },
			{ XMFLOAT3(-1.5000,  0.0000, 0.22500) }, { XMFLOAT3(-1.5000, 0.8400, 0.22500) },
			{ XMFLOAT3(-0.8400, 1.5000, 0.22500) }, { XMFLOAT3(-0.0000, 1.5000, 0.22500) },
			{ XMFLOAT3(-1.5000,  0.0000, 0.15000) }, { XMFLOAT3(-1.5000, 0.8400, 0.15000) },
			{ XMFLOAT3(-0.8400, 1.5000, 0.15000) }, { XMFLOAT3(-0.0000, 1.5000, 0.15000) },
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = pointVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;

		CD3D11_BUFFER_DESC vertexBufferDescription(sizeof(pointVertices), D3D11_BIND_VERTEX_BUFFER);

		DX::ThrowIfFailed(_device->GetD3DDevice()->CreateBuffer(&vertexBufferDescription, &vertexBufferData, &_vertexBuffer));

		static unsigned int pointIndices[192] = {
			//Handle
			0, 1, 2, 3, 4, 5, 6, 7,
			  8, 9, 10, 11, 12, 13, 14, 15,
			12, 13, 14, 15, 16, 17, 18, 19,
			  20, 21, 22, 23, 24, 25, 26, 27,

			28, 29, 30, 31, 32, 33, 34, 35,
			  36, 37, 38, 39, 40, 41, 42, 43,
			40, 41, 42, 43, 44, 45, 46, 47,
			  48, 49, 50, 51, 52, 53, 54, 55,

			// Body
			56, 57, 58, 59, 60, 61, 62, 63,
			  64, 65, 66, 67, 68, 69, 70, 71,
			68, 69, 70, 71, 72, 73, 74, 75,
			  76, 77, 78, 79, 80, 81, 82, 83,

			84, 85, 86, 87, 88, 89, 90, 91,
			  92, 93, 94, 95, 96, 97, 98, 99,
			96, 97, 98, 99, 100, 101, 102, 103,
			  104, 105, 106, 107, 108, 109, 110, 111,

			112, 113, 114, 115, 116, 117, 118, 119,
			  120, 121, 122, 123, 124, 125, 126, 127,
			124, 125, 126, 127, 128, 129, 130, 131,
			  132, 133, 134, 135, 136, 137, 138, 139,

			140, 141, 142, 143, 144, 145, 146, 147,
			  148, 149, 150, 151, 152, 153, 154, 155,
			152, 153, 154, 155, 156, 157, 158, 159,
			  160, 161, 162, 163, 164, 165, 166, 167,
		};

		_indexCount = ARRAYSIZE(pointIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = pointIndices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;

		CD3D11_BUFFER_DESC indexBufferDescription(sizeof(pointIndices), D3D11_BIND_INDEX_BUFFER);

		DX::ThrowIfFailed(_device->GetD3DDevice()->CreateBuffer(&indexBufferDescription, &indexBufferData, &_indexBuffer));

		DX::ThrowIfFailed(CreateDDSTextureFromFile(_device->GetD3DDevice(), L"pottery_2.dds", nullptr, &_colourTexture));
	});

	(createSphere).then([this]() {
		_loadingComplete = true;
		});
}

void Pottery::SetViewProjectionMatrixCB(XMMATRIX& view, XMMATRIX& projection)
{
	XMStoreFloat4x4(&_mvpBufferData.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&_mvpBufferData.projection, XMMatrixTranspose(projection));
}

void Pottery::SetCameraPositionCB(XMFLOAT3& position)
{
	_cameraBufferData.position = position;
}

void Pottery::ReleaseDeviceDependentResources()
{
	_loadingComplete = false;
	if (_vertexShader) _vertexShader.Reset();
	if (_hullShader) _hullShader.Reset();
	if (_domainShader) _domainShader.Reset();
	if (_geometryShader) _geometryShader.Reset();
	if (_pixelShader) _pixelShader.Reset();
	if (_inputLayout) _inputLayout.Reset();
	if (_mvpBuffer) _mvpBuffer.Reset();
	if (_vertexBuffer) _vertexBuffer.Reset();
	if (_indexBuffer) _indexBuffer.Reset();
	if (_rasterState) _rasterState.Reset();
}

void Pottery::Update(StepTimer const& timer)
{
	auto worldMatrix = XMMatrixIdentity();

	worldMatrix = XMMatrixMultiply(worldMatrix, DirectX::XMMatrixScaling(_scale.x, _scale.y, _scale.z));
	worldMatrix = XMMatrixMultiply(worldMatrix, DirectX::XMMatrixRotationQuaternion(DirectX::XMQuaternionRotationRollPitchYaw(_rotation.x, _rotation.y, _rotation.z)));
	worldMatrix = XMMatrixMultiply(worldMatrix, DirectX::XMMatrixTranslation(_position.x, _position.y, _position.z));

	XMStoreFloat4x4(&_mvpBufferData.model, XMMatrixTranspose(worldMatrix));
}

void Pottery::Render()
{
	if (!_loadingComplete) return;

	auto context = _device->GetD3DDeviceContext();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST);
	context->RSSetState(_rasterState.Get());

	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(_mvpBuffer.Get(), 0, NULL, &_mvpBufferData, 0, 0, 0);
	context->UpdateSubresource1(_cameraBuffer.Get(), 0, NULL, &_cameraBufferData, 0, 0, 0);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPosition);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetInputLayout(_inputLayout.Get());
	// Attach our vertex shader.
	context->VSSetShader(_vertexShader.Get(), nullptr, 0);

	context->HSSetShader(_hullShader.Get(), nullptr, 0);

	context->DSSetShader(_domainShader.Get(), nullptr, 0);
	context->DSSetConstantBuffers1(0, 1, _mvpBuffer.GetAddressOf(), nullptr, nullptr);
	context->DSSetConstantBuffers1(1, 1, _cameraBuffer.GetAddressOf(), nullptr, nullptr);

	context->GSSetShader(_geometryShader.Get(), nullptr, 0);
	
	// Attach our pixel shader.
	context->PSSetShader(_pixelShader.Get(), nullptr, 0);
	context->PSSetConstantBuffers1(0, 1, _cameraBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetSamplers(0, 1, &_samplerState);
	context->PSSetShaderResources(0, 1, &_colourTexture);

	// Draw the objects.
	context->DrawIndexed(_indexCount, 0, 0);
}
