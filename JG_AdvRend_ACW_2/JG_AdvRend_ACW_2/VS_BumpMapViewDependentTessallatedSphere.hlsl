cbuffer CameraPositionConstantBuffer : register(b1)
{
	float3 cameraPosition;
}

struct VS_INPUT
{
	float3 Position : POSITION;
	float3 Color : COLOR;
};

struct VS_OUTPUT
{
	float3 Position : POSITION;
	float TessFactor : TESS;
};

VS_OUTPUT VS_tess(VS_INPUT input)
{
	VS_OUTPUT output;
	output.Position = input.Position;
	output.TessFactor = 64.0f;
	
	return output;
}