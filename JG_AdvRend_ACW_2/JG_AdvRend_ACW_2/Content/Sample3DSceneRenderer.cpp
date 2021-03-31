#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const shared_ptr<DeviceResources>& deviceResources) :
	m_degreesPerSecond(45),
	m_tracking(false),
	m_deviceResources(deviceResources),
	_displacementPower(500.0f)
{
	// Create device independent resources
	Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
	DX::ThrowIfFailed(
		m_deviceResources->GetDWriteFactory()->CreateTextFormat(
			L"Segoe UI",
			nullptr,
			DWRITE_FONT_WEIGHT_LIGHT,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			32.0f,
			L"en-US",
			&textFormat
		)
	);

	DX::ThrowIfFailed(
		textFormat.As(&_textFormat)
	);

	DX::ThrowIfFailed(
		_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR)
	);

	DX::ThrowIfFailed(
		m_deviceResources->GetD2DFactory()->CreateDrawingStateBlock(&_stateBlock)
	);
	
	_camera = make_unique<Camera>();
	_camera->SetPosition(0.0f, 0.5f, -0.5f);
	_camera->SetRotation(0.0f, 0.0f, 0.0f);

	m_resourceManager = make_shared<ResourceManager>();

	_starySky = make_unique<StarySky>(deviceResources);
	_wireframeTessellatedSphere = make_unique<WireframeTessellatedSphere>(deviceResources);
	_viewDependentTessellatedSphere = make_unique<ViewDependentTessellatedSphere>(deviceResources);
	_bumpMapViewDependentTessallatedSphere = make_unique<BumpMapViewDependentTessallatedSphere>(deviceResources);
	_terrain = make_unique<Terrain>(deviceResources, m_resourceManager);
	_rocks = make_unique<Rocks>(deviceResources);
	_rayTracedSphereCube = make_unique<RayTracedSphereCube>(deviceResources);
	_rayMarchObjects = make_unique<RayMarchObjects>(deviceResources);
	_pottery = make_unique<Pottery>(deviceResources);
	
	for(auto i = 0; i < numMeteors; i++)
	{
		float randX = -1 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1 - -1)));
		float randY = -0.5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (-0.2 - -0.5)));
		float randZ = -1 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1 - -1)));

		float randPosX = -4 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (4 - -4)));
		float randPosY = 2 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (3.5 - 2)));
		float randPosZ = -4 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (4 - -4)));
		
		_meteors.emplace_back(make_unique<Meteors>(deviceResources, XMFLOAT3(randPosX, randPosY, randPosZ), XMFLOAT3(randX, randY, randZ)));
	}

	_aliens.emplace_back(make_unique<Aliens>(deviceResources, m_resourceManager, XMFLOAT3(1.0f, 0.5f, 1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f)));
	_aliens.emplace_back(make_unique<Aliens>(deviceResources, m_resourceManager, XMFLOAT3(1.0f, 0.5f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f)));
	
	DX::ThrowIfFailed(
		m_deviceResources->GetD2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_whiteBrush)
	);
	
	CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
	);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();
	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);
	XMStoreFloat4x4(&m_projectionMatrix, perspectiveMatrix * orientationMatrix);
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(StepTimer const& timer)
{
	std::wstring displacementDispText = L"Displacement Power: " + std::to_wstring(_displacementPower) + L" ([ & ] to adjust)";

	Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
	DX::ThrowIfFailed(
		m_deviceResources->GetDWriteFactory()->CreateTextLayout(
			displacementDispText.c_str(),
			(uint32)displacementDispText.length(),
			_textFormat.Get(),
			1200.0f, // Max width of the input text.
			100.0f, // Max height of the input text.
			&textLayout
		)
	);

	DX::ThrowIfFailed(
		textLayout.As(&_textLayout)
	);

	DX::ThrowIfFailed(
		_textLayout->GetMetrics(&_textMetrics)
	);
	
	CheckInputCameraMovement(timer);
	_starySky->Update(timer);
	_wireframeTessellatedSphere->Update(timer);
	_viewDependentTessellatedSphere->Update(timer);
	_bumpMapViewDependentTessallatedSphere->Update(timer);
	_terrain->Update(timer);	
	_rocks->Update(timer);
	_pottery->Update(timer);

	for (auto& m : _meteors)
	{
		//// Delete meteors if they hit the groud
		if(m->GetPosition().y < 0.0f)
		{
			_meteors.erase(std::find(_meteors.begin(), _meteors.end(), m));

			float randX = -1 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1 - -1)));
			float randY = -0.5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (-0.2 - -0.5)));
			float randZ = -1 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1 - -1)));

			float randPosX = -4 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (4 - -4)));
			float randPosY = 2 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (3.5 - 2)));
			float randPosZ = -4 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (4 - -4)));

			_meteors.emplace_back(make_unique<Meteors>(m_deviceResources, XMFLOAT3(randPosX, randPosY, randPosZ), XMFLOAT3(randX, randY, randZ)));
		}
		m->Update(timer);
	}

	for (auto& a : _aliens) a->Update(timer);
	
	_rayMarchObjects->Update(timer);
	_rayTracedSphereCube->Update(timer);
}

void Sample3DSceneRenderer::CheckInputCameraMovement(StepTimer const& timer)
{
	if (QueryKeyPressed(VirtualKey::W))
	{
		_camera->MoveForwards(0.5f * timer.GetElapsedSeconds());
	}

	if (QueryKeyPressed(VirtualKey::S))
	{
		_camera->MoveBackwards(0.5f * timer.GetElapsedSeconds());
	}

	if (QueryKeyPressed(VirtualKey::A))
	{
		_camera->MoveLeft(0.5f * timer.GetElapsedSeconds());
	}

	if (QueryKeyPressed(VirtualKey::D))
	{
		_camera->MoveRight(0.5f * timer.GetElapsedSeconds());
	}

	if (QueryKeyPressed(VirtualKey::Up))
	{
		_camera->AddRotationY(50.0f * timer.GetElapsedSeconds());
	}

	if (QueryKeyPressed(VirtualKey::Down))
	{
		_camera->AddRotationY(-50.0f * timer.GetElapsedSeconds());
	}

	if (QueryKeyPressed(VirtualKey::Left))
	{
		_camera->AddRotationX(50.0f * timer.GetElapsedSeconds());
	}

	if (QueryKeyPressed(VirtualKey::Right))
	{
		_camera->AddRotationX(-50.0f * timer.GetElapsedSeconds());
	}

	if (QueryKeyPressed(static_cast<VirtualKey>(VK_OEM_4)))
	{
		_displacementPower -= 0.5f;
	}

	if (QueryKeyPressed(static_cast<VirtualKey>(VK_OEM_6)))
	{
		_displacementPower += 0.5f;
	}
}

bool Sample3DSceneRenderer::QueryKeyPressed(VirtualKey key)
{
	return (CoreWindow::GetForCurrentThread()->GetKeyState(key) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{
	_camera->Render();

	XMMATRIX viewMatrix = XMMatrixIdentity();
	_camera->GetViewMatrix(viewMatrix);

	_terrain->SetViewProjectionMatrixCB(viewMatrix, XMLoadFloat4x4(&m_projectionMatrix));
	_terrain->SetCameraPositionCB(_camera->GetPosition());
	_terrain->Render();
	
	_rocks->SetViewProjectionMatrixCB(viewMatrix, XMLoadFloat4x4(&m_projectionMatrix));
	_rocks->SetCameraPositionCB(_camera->GetPosition());
	_rocks->Render();
	
	_starySky->SetViewProjectionMatrixCB(viewMatrix, XMLoadFloat4x4(&m_projectionMatrix));
	_starySky->Render();
	
	_wireframeTessellatedSphere->SetViewProjectionMatrixCB(viewMatrix, XMLoadFloat4x4(&m_projectionMatrix));
	_wireframeTessellatedSphere->Render();

	_viewDependentTessellatedSphere->SetViewProjectionMatrixCB(viewMatrix, XMLoadFloat4x4(&m_projectionMatrix));
	_viewDependentTessellatedSphere->SetCameraPositionCB(_camera->GetPosition());
	_viewDependentTessellatedSphere->Render();

	_bumpMapViewDependentTessallatedSphere->SetViewProjectionMatrixCB(viewMatrix, XMLoadFloat4x4(&m_projectionMatrix));
	_bumpMapViewDependentTessallatedSphere->SetCameraPositionCB(_camera->GetPosition());
	_bumpMapViewDependentTessallatedSphere->SetDisplacementPowerCB(_displacementPower);
	_bumpMapViewDependentTessallatedSphere->Render();

	_pottery->SetViewProjectionMatrixCB(viewMatrix, XMLoadFloat4x4(&m_projectionMatrix));
	_pottery->SetCameraPositionCB(_camera->GetPosition());
	_pottery->Render();

	for (auto& m : _meteors)
	{
		m->SetViewProjectionMatrixCB(viewMatrix, XMLoadFloat4x4(&m_projectionMatrix));
		m->SetCameraPositionCB(_camera->GetPosition());
		m->Render();
	}

	for(auto& a : _aliens)
	{
		a->SetViewProjectionMatrixCB(viewMatrix, XMLoadFloat4x4(&m_projectionMatrix));
		a->SetCameraPositionCB(_camera->GetPosition());
		a->Render();
	}
	
	_rayMarchObjects->SetViewProjectionMatrixCB(viewMatrix, XMLoadFloat4x4(&m_projectionMatrix));
	_rayMarchObjects->SetCameraPositionCB(_camera->GetPosition());
	_rayMarchObjects->Render();
	
	_rayTracedSphereCube->SetViewProjectionMatrixCB(viewMatrix, XMLoadFloat4x4(&m_projectionMatrix));
	_rayTracedSphereCube->SetCameraPositionCB(_camera->GetPosition());
	_rayTracedSphereCube->SetInverseViewMatrixConstantBuffer(XMMatrixInverse(nullptr, viewMatrix));
	_rayTracedSphereCube->Render();

	ID2D1DeviceContext* context = m_deviceResources->GetD2DDeviceContext();
	Windows::Foundation::Size logicalSize = m_deviceResources->GetLogicalSize();

	context->SaveDrawingState(_stateBlock.Get());
	context->BeginDraw();

	// Position on the bottom right corner
	D2D1::Matrix3x2F screenTranslation = D2D1::Matrix3x2F::Translation(
		logicalSize.Width - _textMetrics.layoutWidth,
		logicalSize.Height - _textMetrics.height - 40
	);

	context->SetTransform(screenTranslation * m_deviceResources->GetOrientationTransform2D());

	DX::ThrowIfFailed(
		_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING)
	);

	context->DrawTextLayout(
		D2D1::Point2F(0.f, 0.f),
		_textLayout.Get(),
		_whiteBrush.Get()
	);

	HRESULT hr = context->EndDraw();
	if (hr != D2DERR_RECREATE_TARGET)
	{
		DX::ThrowIfFailed(hr);
	}

	context->RestoreDrawingState(_stateBlock.Get());
}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
	DX::ThrowIfFailed(
		m_deviceResources->GetD2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_whiteBrush)
	);
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	_whiteBrush.Reset();
	
	_starySky->ReleaseDeviceDependentResources();
	_wireframeTessellatedSphere->ReleaseDeviceDependentResources();
	_viewDependentTessellatedSphere->ReleaseDeviceDependentResources();
	_bumpMapViewDependentTessallatedSphere->ReleaseDeviceDependentResources();
	_terrain->ReleaseDeviceDependentResources();
	_rocks->ReleaseDeviceDependentResources();
	_rayTracedSphereCube->ReleaseDeviceDependentResources();
	_rayMarchObjects->ReleaseDeviceDependentResources();
	_pottery->ReleaseDeviceDependentResources();

	for (auto& m : _meteors) m->ReleaseDeviceDependentResources();
	for (auto& a : _aliens) a->ReleaseDeviceDependentResources();
}