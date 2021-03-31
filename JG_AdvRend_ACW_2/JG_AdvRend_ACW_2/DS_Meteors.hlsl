cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
}

cbuffer CameraPositionConstantBuffer : register(b1)
{
	float3 cameraPosition;
	float padding;
}

struct HS_Tri_Tess_Param
{
	float Edges[4] : SV_TessFactor;
	float Inside[2] : SV_InsideTessFactor;
};

struct DS_INPUT
{
	float3 Position : POSITION;
};

struct DS_OUTPUT
{
	float4 PositionH : SV_POSITION;
	float3 PositionW : POSITION;
	float3 ViewDir : VIEWDIR;
	float2 UV : TEXCOORD;
};

[domain("quad")]
DS_OUTPUT DS_TriTess(HS_Tri_Tess_Param input,
	float2 UV : SV_DomainLocation,
	const OutputPatch<DS_INPUT, 4> tri)
{
	const float pi2 = 6.28318530;
	const float pi = pi2 / 2.0f;
	DS_OUTPUT output;

	float u = UV.x; float v = UV.y;
	u *= pi2;
	v *= pi;
	float Xuv = cos(u) * sin(v);
	float Yuv = sin(u) * sin(v);
	float Zuv = cos(v);
	output.PositionW = float3(Xuv, Yuv, Zuv);

	output.PositionH = mul(float4(output.PositionW, 1.0f), model);
	output.PositionH = mul(output.PositionH, view);
	output.PositionH = mul(output.PositionH, projection);

	output.UV = UV.xy;
	output.ViewDir = normalize(cameraPosition.xyz - output.PositionW);

	return output;
}