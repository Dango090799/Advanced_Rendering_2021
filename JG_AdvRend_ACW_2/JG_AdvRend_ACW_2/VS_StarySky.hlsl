struct VSInput
{
	float3 Pos : POSITION;
	float3 Colour : COLOR0;
	uint ID : SV_VertexID;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
	float3 Colour : COLOR0;
};

float3 posf2(float t, float i)
{
	return float3(
		sin(t + i * .9553) +
		sin(t * 1.311 + i) +
		sin(t * 1.4 + i * 1.53) +
		sin(t * 1.84 + i * .76),
		sin(t + i * .79553 + 2.1) +
		sin(t * 1.311 + i * 1.1311 + 2.1) +
		sin(t * 1.4 + i * 1.353 - 2.1) +
		sin(t * 1.84 + i * .476 - 2.1),
		sin(t + i * .5553 - 2.1) +
		sin(t * 1.311 + i * 1.1 - 2.1) +
		sin(t * 1.4 + i * 1.23 + 2.1) +
		sin(t * 1.84 + i * .36 + 2.1)
		) * .2;
}

VSOutput main(VSInput input)
{
	VSOutput output;

	float t = 2 * 0.2;
	float i = input.ID + sin(input.ID) * 100.;

	output.Pos = float4(10 * posf2(t, i), 1.0);

	output.Colour = input.Colour;

	return output;
}

