#pragma once
#include "ShaderStructures.h"
#include "..\Common\DeviceResources.h"
#include "..\Common\StepTimer.h"
#include <vector>
#include <string>

#include "Camera.h"
#include "StarySky.h"
#include "WireframeTessellatedSphere.h"
#include "ViewDependentTessellatedSphere.h"
#include "BumpMapViewDependentTessallatedSphere.h"
#include "ResourceManager.h"
#include "Terrain.h"
#include "Rocks.h"
#include "RayTracedSphereCube.h"
#include "RayMarchObjects.h"
#include "Pottery.h"
#include "Meteors.h"
#include "Aliens.h"

using namespace DX;
using namespace std;
using namespace DirectX;
using namespace Windows::System;
using namespace Windows::UI::Core;
using namespace Windows::Foundation;

// This sample renderer instantiates a basic rendering pipeline.
class Sample3DSceneRenderer
{
public:
	Sample3DSceneRenderer(const shared_ptr<DeviceResources>& deviceResources);
	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();
	void ReleaseDeviceDependentResources();
	void Update(StepTimer const& timer);
	void Render();

private:
	void CheckInputCameraMovement(StepTimer const& timer);
	bool QueryKeyPressed(VirtualKey key);

private:
	// Cached pointer to device resources.
	shared_ptr<DeviceResources> m_deviceResources;
	shared_ptr<ResourceManager> m_resourceManager;

	unique_ptr<Camera> _camera;
	unique_ptr<StarySky> _starySky;
	unique_ptr<WireframeTessellatedSphere> _wireframeTessellatedSphere;
	unique_ptr<ViewDependentTessellatedSphere> _viewDependentTessellatedSphere;
	unique_ptr<BumpMapViewDependentTessallatedSphere> _bumpMapViewDependentTessallatedSphere;
	unique_ptr<Terrain> _terrain;
	unique_ptr<Rocks> _rocks;
	unique_ptr<RayTracedSphereCube> _rayTracedSphereCube;
	unique_ptr<RayMarchObjects> _rayMarchObjects;
	unique_ptr<Pottery> _pottery;
	vector<unique_ptr<Meteors>> _meteors;
	vector<unique_ptr<Aliens>> _aliens;

	XMFLOAT4X4 m_projectionMatrix;

	float	m_degreesPerSecond, _displacementPower;
	bool	m_tracking;
	int numMeteors = 10;
	
	// Resources related to text rendering.
	Microsoft::WRL::ComPtr<ID2D1DrawingStateBlock1> _stateBlock;
	Microsoft::WRL::ComPtr<IDWriteTextFormat2>      _textFormat;
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>    _whiteBrush;
	Microsoft::WRL::ComPtr<IDWriteTextLayout3>      _textLayout;
	DWRITE_TEXT_METRICS	                            _textMetrics;
};

