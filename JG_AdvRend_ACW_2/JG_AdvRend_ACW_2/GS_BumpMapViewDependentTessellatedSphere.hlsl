struct GSInput
{
	float4 PositionH : SV_POSITION;
	float3 PositionW : POSITION;
	//float4 PositionW : SV_POSITION;
	float2 UV : TEXCOORD0;
};

struct GSOutput
{
	float4 PositionH : SV_POSITION;
	float3 PositionW : POSITION;
	float2 UV : TEXCOORD0;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
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

float hash(float n)
{
	return frac(sin(n) * 43758.5453);
}

float noise(float3 x)
{
	float3 p = floor(x);
	float3 f = frac(x);

	f = f * f * (3.0 - 2.0 * f);
	float n = p.x + p.y * 57.0 + 113.0 * p.z;

	return lerp(lerp(lerp(hash(n + 0.0), hash(n + 1.0), f.x),
		lerp(hash(n + 57.0), hash(n + 58.0), f.x), f.y),
		lerp(lerp(hash(n + 113.0), hash(n + 114.0), f.x),
			lerp(hash(n + 170.0), hash(n + 171.0), f.x), f.y), f.z);
}

[maxvertexcount(3)]
void main(
	triangle GSInput input[3] : SV_POSITION, 
	inout TriangleStream< GSOutput > output
)
{
	for (uint i = 0; i < 3; i++)
	{
		GSOutput element;
		element.PositionH = input[i].PositionH;
		element.PositionW = input[i].PositionW;
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