struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float3 Color : COLOR;
	float2 UV : TEXCOORD0;
};

float4 PS_Tess(PS_INPUT input) : SV_TARGET{
	return float4(input.Color, 1);
}