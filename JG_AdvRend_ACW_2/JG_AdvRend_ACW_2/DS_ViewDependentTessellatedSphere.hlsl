cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
}

struct HS_Tri_Tess_Param
{
	float Edges[4] : SV_TessFactor;
	float Inside[2] : SV_InsideTessFactor;
};

struct DS_OUTPUT
{
	float4 Position : SV_POSITION;
	float3 Color : COLOR;
	float2 UV : TEXCOORD0;
};

[domain("quad")]
DS_OUTPUT DS_TriTess(HS_Tri_Tess_Param input,
	float2 UV : SV_DomainLocation,
	const OutputPatch<DS_OUTPUT, 4> tri)
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
	float3 sphere = float3(Xuv, Yuv, Zuv);


	output.Position = float4(sphere, 1);
	output.Position = mul(output.Position, model);
	output.Position = mul(output.Position, view);
	output.Position = mul(output.Position, projection);
	output.Color = float3(1, 1, 1);

	output.UV = UV.xy;

	return output;
}