struct HullShaderInput
{
	float3 Position : POSITION;
	float2 UV : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
	float Tess : TESS;
};

struct PatchConstantOutput
{
	float edges[3] : SV_TessFactor;
	float inside : SV_InsideTessFactor;
};

struct DomainShaderInput
{
	float3 Position : POSITION;
	float2 UV : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
};

PatchConstantOutput PatchConstantFunction(InputPatch<HullShaderInput, 3> inputPatch, uint patchId : SV_PrimitiveID)
{
	PatchConstantOutput output;

	output.edges[0] = 0.5f * (inputPatch[1].Tess + inputPatch[2].Tess);
	output.edges[1] = 0.5f * (inputPatch[2].Tess + inputPatch[0].Tess);
	output.edges[2] = 0.5f * (inputPatch[0].Tess + inputPatch[1].Tess);

	output.inside = output.edges[0];

	return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchConstantFunction")]
[maxtessfactor(64.0f)]
DomainShaderInput main(InputPatch<HullShaderInput, 3> inputPatch, uint i : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
	DomainShaderInput output;
	
	output.Position = inputPatch[i].Position;
	output.UV = inputPatch[i].UV;
	output.Normal = inputPatch[i].Normal;
	output.Tangent = inputPatch[i].Tangent;
	output.Binormal = inputPatch[i].Binormal;

	return output;
}