#define NUM_OF_CONTROL_POINTS 16

struct HS_INPUT
{
	float3 Position : POSITION;
};

struct PatchConstantOutput
{
	float edges[4] : SV_TessFactor;
	float inside[2] : SV_InsideTessFactor;
};

struct HS_OUTPUT
{
	float3 Position : BEZIERPOS;
};

PatchConstantOutput PatchConstantFunction(InputPatch<HS_INPUT, NUM_OF_CONTROL_POINTS> inputPatch, uint patchID : SV_PrimitiveID)
{
	PatchConstantOutput output;
	float tessellationFactor = 32.0f;

	output.edges[0] = output.edges[1] = output.edges[2] = output.edges[3] = tessellationFactor;
	output.inside[0] = output.inside[1] = tessellationFactor;

	return output;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_OF_CONTROL_POINTS)]
[patchconstantfunc("PatchConstantFunction")]
[maxtessfactor(64.0f)]
HS_OUTPUT main(InputPatch<HS_INPUT, NUM_OF_CONTROL_POINTS> inputPatch, uint outputControlPointID : SV_OutputControlPointID, uint patchID : SV_PrimitiveID)
{
	HS_OUTPUT output;
	output.Position = inputPatch[outputControlPointID].Position;
	return output;
}