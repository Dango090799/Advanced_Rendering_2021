#define NUM_OF_CONTROL_POINTS 16

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

struct PatchConstantOutput
{
	float edges[4] : SV_TessFactor;
	float inside[2] : SV_InsideTessFactor;
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

float4 BernsteinBasis(float t)
{
	float invT = 1.0f - t;

	return float4(invT * invT * invT, 3.0f * t * invT * invT, 3.0f * t * t * invT, t * t * t);
}

float3 EvaluateBezier(const OutputPatch<DS_INPUT, NUM_OF_CONTROL_POINTS> patch,
	float4 basisU,
	float4 basisV)
{
	float3 value = float3(0, 0, 0);
	value = basisV.x * (patch[0].Position * basisU.x + patch[1].Position * basisU.y + patch[2].Position * basisU.z + patch[3].Position * basisU.w);
	value += basisV.y * (patch[4].Position * basisU.x + patch[5].Position * basisU.y + patch[6].Position * basisU.z + patch[7].Position * basisU.w);
	value += basisV.z * (patch[8].Position * basisU.x + patch[9].Position * basisU.y + patch[10].Position * basisU.z + patch[11].Position * basisU.w);
	value += basisV.w * (patch[12].Position * basisU.x + patch[13].Position * basisU.y + patch[14].Position * basisU.z + patch[15].Position * basisU.w);

	return value;
}

[domain("quad")]
DS_OUTPUT main(in PatchConstantOutput input, in const float2 UV : SV_DomainLocation, const OutputPatch<DS_INPUT, NUM_OF_CONTROL_POINTS> patch)
{
	DS_OUTPUT output;

	float4 basisU = BernsteinBasis(UV.x);
	float4 basisV = BernsteinBasis(UV.y);

	output.PositionW = EvaluateBezier(patch, basisU, basisV);
	output.ViewDir = normalize(cameraPosition.xyz - output.PositionW);

	output.PositionH = mul(float4(output.PositionW, 1.0f), model);
	output.PositionH = mul(output.PositionH, view);
	output.PositionH = mul(output.PositionH, projection);

	output.UV = UV;

	return output;
}