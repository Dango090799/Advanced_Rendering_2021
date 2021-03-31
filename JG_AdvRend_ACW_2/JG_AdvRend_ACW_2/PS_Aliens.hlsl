#define NUMBER_OF_LIGHTS 1

cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

cbuffer CameraPositionConstantBuffer : register(b1) {
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

struct Ray {
	float3 o; //origin
	float3 d; //direction=
};

struct PS_OUTPUT
{
	float4 Colour : SV_TARGET;
	float Depth : SV_DEPTH;
};

static Light lights[NUMBER_OF_LIGHTS] = {
	//LightOne
	{0.2, 0.2, 0.2, 1.0, 0.4, 0.4, 0.4, 1.0, 0.4, 0.4, 0.4, 1.0, 0.0, 3.0, 0.0, 20},
};

struct PixelShaderInput
{
	float4 PositionH : SV_POSITION;
	float3 PositionW : POSITION;
	float2 UV : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
	float3 ViewDirection : VIEWDIR;
};

float4 PhongIllumination(float3 pos, float3 normal, float3 viewDir, float4 diffuse)
{
	float4 totalAmbient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 totalDiffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 totalSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);

	for (int i = 0; i < NUMBER_OF_LIGHTS; i++)
	{
		totalAmbient += lights[i].ambientColour * diffuse;

		float3 lightDirection = normalize(lights[i].lightPosition - pos);
		float nDotL = dot(normal, lightDirection);
		float3 reflection = normalize(reflect(-lightDirection, normal));
		float rDotV = max(0.0f, dot(reflection, viewDir));

		totalDiffuse += saturate(lights[i].diffuseColour * nDotL * diffuse);

		if (nDotL > 0.0f)
		{
			float4 specularIntensity = float4(1.0, 1.0, 1.0, 1.0);
			totalSpecular += lights[i].specularColour * (pow(rDotV, lights[i].specularPower)) * specularIntensity;
		}
	}
	return totalAmbient + totalDiffuse + totalSpecular;
}

float4 main(PixelShaderInput input) : SV_TARGET
{
	PS_OUTPUT output;
	return PhongIllumination(input.PositionW, input.Normal, input.ViewDirection, saturate(float4(1.0f, 0.0f, 0.0f, 1.0f)));
}