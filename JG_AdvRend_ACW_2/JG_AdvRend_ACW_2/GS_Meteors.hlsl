struct GS_Input
{
	float4 PositionH : SV_POSITION;
	float3 PositionW : POSITION;
	float3 ViewDir : VIEWDIR;
	float2 UV : TEXCOORD;
};

struct GS_Output
{
	float4 PositionH : SV_POSITION;
	float3 PositionW : POSITION;
	float3 ViewDir : VIEWDIR;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
	float2 UV : TEXCOORD;
};

float3 CalculateNormal(float3 a, float3 b, float3 c)
{
	return normalize(cross(b - a, c - a));
}

float3 CalculateTangent(float3 v1, float3 v2, float2 tuVector, float2 tvVector)
{
	float3 tangent;
	float den = 1.0f / (tuVector.x * tvVector.y - tuVector.y * tvVector.x);

	tangent.x = (tvVector.y * v1.x - tvVector.x * v2.x) * den;
	tangent.y = (tvVector.y * v1.y - tvVector.x * v2.y) * den;
	tangent.z = (tvVector.y * v1.z - tvVector.x * v2.z) * den;

	float length = sqrt(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z);

	tangent.x = tangent.x / length;
	tangent.y = tangent.y / length;
	tangent.z = tangent.z / length;

	return tangent;
}

float3 CalculateBinormal(float3 v1, float3 v2, float2 tuVector, float2 tvVector)
{
	float3 binormal;
	float den = 1.0f / (tuVector.x * tvVector.y - tuVector.y * tvVector.x);

	binormal.x = (tuVector.x * v2.x - tuVector.y * v1.x) * den;
	binormal.y = (tuVector.x * v2.y - tuVector.y * v1.y) * den;
	binormal.z = (tuVector.x * v2.z - tuVector.y * v1.z) * den;

	float length = sqrt(binormal.x * binormal.x + binormal.y * binormal.y + binormal.z * binormal.z);

	binormal.x = binormal.x / length;
	binormal.y = binormal.y / length;
	binormal.z = binormal.z / length;

	return binormal;
}

[maxvertexcount(3)]
void main(
	triangle GS_Input input[3] : SV_POSITION, 
	inout TriangleStream< GS_Output > output
)
{
	for (uint i = 0; i < 3; i++)
	{
		GS_Output element;
		element.PositionW = input[i].PositionW;
		element.PositionH = input[i].PositionH;
		element.ViewDir = input[i].ViewDir;
		element.UV = input[i].UV;

		element.Normal = normalize(CalculateNormal(input[0].PositionH.xyz, input[1].PositionH.xyz, input[2].PositionH.xyz));

		float3 v1 = input[1].PositionH.xyz - input[0].PositionH.xyz;
		float3 v2 = input[2].PositionH.xyz - input[0].PositionH.xyz;

		float2 tuVector, tvVector;
		tuVector.x = input[1].UV.x - input[0].UV.x;
		tvVector.x = input[1].UV.y - input[0].UV.y;
		tuVector.y = input[2].UV.x - input[0].UV.x;
		tvVector.y = input[2].UV.y - input[0].UV.y;

		element.Tangent = normalize(CalculateTangent(v1, v2, tuVector, tvVector));
		element.Binormal = normalize(CalculateBinormal(v1, v2, tuVector, tvVector));
		
		output.Append(element);
	}
}