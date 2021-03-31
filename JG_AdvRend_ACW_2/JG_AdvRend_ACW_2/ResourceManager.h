#pragma once
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <locale.h>
#include <map>
#include <memory>
#include <fstream>
#include <iostream>
#include <vector>
#include <d3d11.h>
#include <DirectXMath.h>

#include <DDSTextureLoader.h>
#include "..\Common\DeviceResources.h"
#include "..\Common\DirectXHelper.h"
#include "..\Common\StepTimer.h"
#include "..\Content\ShaderStructures.h"

#include "..\\Content\ShaderStructures.h"

using namespace JG_AdvRend_ACW_2;
using namespace DX;
using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;

class ResourceManager
{
public:
	ResourceManager() = default;
	~ResourceManager() = default;

public:
	bool LoadModel(shared_ptr<DeviceResources>& const device, const char* const modelFileName, int& indexCount, ComPtr<ID3D11Buffer>& vertexBuffer, ComPtr<ID3D11Buffer>& indexBuffer);
};

