struct VS_INPUT
{
	float3 Position : POSITION;
};

struct VS_OUTPUT
{
	float3 Position : POSITION;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.Position = input.Position;
	return output;
}