// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

cbuffer CameraBuffer : register(b1)
{
	float3 cameraPosition;
	float cameraPadding;
};

cbuffer TimeBuffer : register(b2){
	float time;
	float3 timePadding;
}

cbuffer OffsetBuffer : register(b3){
	float3 positionOffset;
	float3 rotationOffset;
	float3 scaleOffset;
}

//Type definitions
struct PatchConstantOutput
{
	float edges[3] : SV_TessFactor;
	float inside : SV_InsideTessFactor;
};

struct DomainShaderInput
{
	float3 Position : POSITION;
	float2 UV : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
};

struct PixelShaderInput
{
	float4 PositionH : SV_POSITION;
	float3 PositionW : POSITION;
	float2 UV : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
	float3 ViewDirection : VIEWDIR;
};

float4 BernsteinBasis(float t)
{
	float invT = 1.0f - t;

	return float4(invT * invT * invT, 3.0f * t * invT * invT, 3.0f * t * t * invT, t * t * t);
}

float4 EvaluateCubicHermite(float4 basis)
{
	return float4(
			basis.x + basis.y,
			basis.y / 3.0,
			-basis.z / 3.0,
			basis.z + basis.w
	);
}

float hash(float n)
{
	return frac(sin(n) * 43758.5453);
}

float noise(float3 x)
{
	float3 p = floor(x);
	float3 f = frac(x);

	f = f * f * (3.0 - 2.0 * f);
	float n = p.x + p.y * 57.0 + 113.0 * p.z;

	return lerp(lerp(lerp(hash(n + 0.0), hash(n + 1.0), f.x),
		lerp(hash(n + 57.0), hash(n + 58.0), f.x), f.y),
		lerp(lerp(hash(n + 113.0), hash(n + 114.0), f.x),
			lerp(hash(n + 170.0), hash(n + 171.0), f.x), f.y), f.z);
}

[domain("tri")]
PixelShaderInput main(in PatchConstantOutput input, in const float3 UVW : SV_DomainLocation, const OutputPatch<DomainShaderInput, 3> patch)
{
	PixelShaderInput output;

	float4 basis = BernsteinBasis(UVW.x);
	output.PositionW = EvaluateCubicHermite(basis);
	
	//Calculate new vertex position
	output.UV = UVW.x * patch[0].UV + UVW.y * patch[1].UV + UVW.z * patch[2].UV;
	output.Normal = UVW.x * patch[0].Normal + UVW.y * patch[1].Normal + UVW.z * patch[2].Normal;
	output.Tangent = UVW.x * patch[0].Tangent + UVW.y * patch[1].Tangent + UVW.z * patch[2].Tangent;
	output.Binormal = UVW.x * patch[0].Binormal + UVW.y * patch[1].Binormal + UVW.z * patch[2].Binormal;

	output.Normal = normalize(output.Normal);
	output.Tangent = normalize(output.Tangent);
	output.Binormal = normalize(output.Binormal);

	output.PositionW += noise(output.PositionW * time);
	output.PositionW += positionOffset;
	
	output.ViewDirection = normalize(cameraPosition.xyz - output.PositionW);

	output.PositionH = mul(float4(output.PositionW, 1.0f), view);
	output.PositionH = mul(output.PositionH, projection);

	return output;
}