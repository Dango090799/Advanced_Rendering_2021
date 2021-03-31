#pragma once

using namespace DirectX;

namespace JG_AdvRend_ACW_2
{
	// Constant buffer used to send MVP matrices to the vertex shader.
	struct ModelViewProjectionConstantBuffer
	{
		XMFLOAT4X4 model;
		XMFLOAT4X4 view;
		XMFLOAT4X4 projection;
	};

	struct CameraPositionConstantBuffer
	{
		XMFLOAT3 position;
		float dummy;
	};

	// Used to send per-vertex data to the vertex shader.
	struct VertexPositionColor
	{
		XMFLOAT3 pos;
		XMFLOAT3 color;
	};

	struct VertexPositionTexcoordNormalTangentBinormal
	{
		XMFLOAT3 position;
		XMFLOAT2 texcoord;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
		XMFLOAT3 binormal;
	};

	struct VertexPosition
	{
		XMFLOAT3 position;
	};

	struct InverseViewConstantBuffer
	{
		XMFLOAT4X4 inverseView;
	};

	struct TessellationFactorConstantBuffer
	{
		float tessellationFactor;
		XMFLOAT3 padding;
	};

	struct DisplacementPowerConstantBuffer
	{
		float displacementPower;
		XMFLOAT3 padding;
	};

	struct TimeConstantBuffer
	{
		float time;
		XMFLOAT3 padding;
	};

	struct OffsetConstantBuffer
	{
		XMFLOAT3 position;
		XMFLOAT3 rotation;
		XMFLOAT3 scale;
		XMFLOAT3 padding;
	};
}