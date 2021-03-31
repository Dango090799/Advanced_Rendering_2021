struct VS_INPUT
{
	float4 Position : POSITION;
	float3 Color : COLOR;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float3 Color : COLOR;
};

VS_OUTPUT VS_tess(VS_INPUT input)
{
	VS_OUTPUT output;
	output.Position = input.Position;
	output.Color = input.Color;
	return output;
}