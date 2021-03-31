struct VS_INPUT
{
	float3 Position : POSITION;
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