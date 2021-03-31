cbuffer ModelViewProjectionBuffer : register(b0)
{
	matrix Model;
	matrix View;
	matrix Projection;
}

cbuffer DisplacementBuffer : register(b1){
	float displacementPower;
	float3 padding;
}

struct HS_Tri_Tess_Param
{
	float Edges[4] : SV_TessFactor;
	float Inside[2] : SV_InsideTessFactor;
};

struct DS_INPUT
{
	float3 PositionW : POSITION;
	float2 UV : TEXCOORD0;
};

struct DS_OUTPUT
{
	float4 PositionH : SV_POSITION;
	float3 PositionW : POSITION;
	float2 UV : TEXCOORD0;
};

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

static float PI = 3.14159265359;

[domain("quad")]
DS_OUTPUT DS_TriTess(HS_Tri_Tess_Param input,
	float2 UV : SV_DomainLocation,
	const OutputPatch<DS_INPUT, 4> quad)
{
	DS_OUTPUT output;
	
	float3 pos = 0;
	
	float radius = 6000.0f;
	pos.x = radius * cos(UV.x * (2 * PI)) * sin(UV.y * PI);
	pos.y = radius * sin(UV.x * (2 * PI)) * sin(UV.y * PI);
	pos.z = radius * cos(UV.y * PI);
	
	float height = displacementPower * noise(pos);
	pos += height;// *normalize(pos);

	output.PositionH = mul(float4(pos, 1), Model);
	output.PositionH = mul(output.PositionH, View);
	output.PositionH = mul(output.PositionH, Projection);
	
	output.UV = UV;
	output.PositionW = pos;
	
	return output;
}