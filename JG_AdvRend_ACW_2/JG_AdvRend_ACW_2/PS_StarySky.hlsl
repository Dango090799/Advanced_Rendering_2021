struct PSInput
{
	float4 Pos : SV_POSITION;
	float3 Colour : COLOR0;
	float2 UV : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
	return float4(input.Colour, 1);
}