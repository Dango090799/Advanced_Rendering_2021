cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
}

cbuffer CameraPositionConstantBuffer : register(b1)
{
	float3 cameraPosition;
}

struct VS_INPUT
{
	float4 Position : POSITION;
	float3 Color : COLOR;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float3 Color : COLOR;
	float TessFactor : TESS;
};

VS_OUTPUT VS_tess(VS_INPUT input)
{
	VS_OUTPUT output;
	output.Position = input.Position;
	output.Color = input.Color;

	float3 modelPosition = mul(output.Position, model).xyz;
	float distanceToCamera = distance(modelPosition, cameraPosition);
	float tess = saturate((3.0f - distanceToCamera) / (3.0f - 1.0f));
	output.TessFactor = 1.0f + tess * (64.0f - 1.0f);
	
	return output;
}