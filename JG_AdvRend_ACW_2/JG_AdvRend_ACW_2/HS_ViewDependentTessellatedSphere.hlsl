struct HS_Tri_Tess_Param
{
	float Edges[4] : SV_TessFactor;
	float Inside[2] : SV_InsideTessFactor;
};

struct HS_INPUT
{
	float4 Position : SV_POSITION;
	float3 Color : COLOR;
	float TessFactor : TESS;
};

struct HS_OUTPUT
{
	float4 Position : SV_POSITION;
	float3 Color : COLOR;
};

HS_Tri_Tess_Param ConstantHS(InputPatch <HS_INPUT, 4> ip)
{
	HS_Tri_Tess_Param output;
	
	output.Edges[0] = output.Edges[1] = output.Edges[2] = output.Edges[3] = ip[0].TessFactor;
	output.Inside[0] = output.Inside[1] = ip[0].TessFactor;

	return output;
}

[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HS_OUTPUT HS_TriTess(InputPatch<HS_INPUT, 4> patch,
	uint i : SV_OutputControlPointID)
{
	HS_OUTPUT output;
	output.Position = patch[i].Position;
	output.Color = patch[i].Color;
	return output;
}