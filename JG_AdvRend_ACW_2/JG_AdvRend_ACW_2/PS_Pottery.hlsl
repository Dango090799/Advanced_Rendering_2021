#define NUMBER_OF_LIGHTS 1

Texture2D txColor : register(t0);
SamplerState txSampler : register(s0);

cbuffer CameraPositionConstantBuffer : register(b0){
	float3 cameraPosition;
	float padding;
}

struct Light
{
	float4 ambientColour;
	float4 diffuseColour;
	float4 specularColour;
	float3 lightPosition;
	float specularPower;
};

static Light lights[NUMBER_OF_LIGHTS] = {
	//LightOne
	{0.5, 0.5, 0.5, 1.0, 0.4, 0.4, 0.4, 1.0, 0.4, 0.4, 0.4, 1.0, -2.0, 3.0, 4.0, 20},
};

struct PS_INPUT
{
	float4 PositionH : SV_POSITION;
	float3 PositionW : POSITION;
	float3 ViewDir : VIEWDIR;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
	float2 UV : TEXCOORD;
};

float4 PhongIllumination(float3 pos, float3 normal, float3 viewDir, float2 texcoord)
{
	float4 totalAmbient = 0;
	float4 totalDiffuse = 0;
	float4 totalSpecular = 0;
	float4 baseColour = txColor.Sample(txSampler, texcoord);

	totalAmbient += lights[0].ambientColour * baseColour;

	float3 lightDirection = normalize(lights[0].lightPosition - pos);
	float nDotL = dot(normal, lightDirection);
	float3 reflection = normalize(reflect(-lightDirection, normal));
	float rDotV = max(0.0f, dot(reflection, viewDir));

	totalDiffuse += saturate(lights[0].diffuseColour * nDotL * baseColour);

	if (nDotL > 0.0f)
	{
		totalSpecular += lights[0].specularColour * (pow(rDotV, lights[0].specularPower));
	}

	return totalAmbient + totalDiffuse + totalSpecular;
}

float4 main(PS_INPUT input) : SV_TARGET
{
	float3 normal = 2.0 * input.Normal.xyz - 1.0f;
	float3 bump = normalize(normal.x * input.Tangent + normal.y * input.Binormal + normal.z * input.Normal);

	return PhongIllumination(input.PositionW, bump, normalize(cameraPosition.xyz - input.PositionW), input.UV / 20);
}