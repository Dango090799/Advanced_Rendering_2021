cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
}

struct GSInput
{
	float4 Pos : SV_POSITION;
	float3 Colour : COLOR0;
};

struct GSOutput
{
	float4 Pos : SV_POSITION;
	float3 Colour : COLOR0;
	float2 UV : TEXCOORD0;
};

static const float3 g_positions[5] = {
	float3(-1, 1, 0),
	float3(-1, -1, 0),
	float3(1, 1, 0),
	float3(1, -1, 0),
	float3(0, 0, 0)
};

[maxvertexcount(12)]
void main(point GSInput input[1],
	inout TriangleStream<GSOutput> OutputStream)
{
	GSOutput output = (GSOutput)0;

	float4 vPos = input[0].Pos;
	vPos = mul(vPos, model);
	vPos = mul(vPos, view);

	float quadSize = 0.01f;

	// These triangles may need moving to clockwise declaration for culling mode to be changed
	// Triangle 1
	//
	// vertex 1
	output.Pos = vPos + float4(quadSize * g_positions[0], 0.0);
	output.Pos = mul(output.Pos, projection);

	output.Colour = float3(0.6, 0.6, 0.6);
	output.UV = (sign(input[0].Pos.xy) + 1.0) / 2.0;
	OutputStream.Append(output);

	// vertex 2
	output.Pos = vPos + float4(quadSize * g_positions[1], 0.0);
	output.Pos = mul(output.Pos, projection);

	output.Colour = float3(0.6, 0.6, 0.6);
	output.UV = (sign(input[0].Pos.xy) + 1.0) / 2.0;
	OutputStream.Append(output);

	// vertex 3
	output.Pos = vPos;
	output.Pos = mul(output.Pos, projection);

	output.Colour = float3(1, 1, 1);
	output.UV = (sign(input[0].Pos.xy) + 1.0) / 2.0;
	OutputStream.Append(output);

	OutputStream.RestartStrip();
	//
	// Triangle 2
	//
	// vertex 1
	output.Pos = vPos;
	output.Pos = mul(output.Pos, projection);

	output.Colour = float3(1, 1, 1);
	output.UV = (sign(input[0].Pos.xy) + 1.0) / 2.0;
	OutputStream.Append(output);

	// vertex 2
	output.Pos = vPos + float4(quadSize * g_positions[1], 0.0);
	output.Pos = mul(output.Pos, projection);

	output.Colour = float3(0.6, 0.6, 0.6);
	output.UV = (sign(input[0].Pos.xy) + 1.0) / 2.0;
	OutputStream.Append(output);

	// vertex 3
	output.Pos = vPos + float4(quadSize * g_positions[3], 0.0);
	output.Pos = mul(output.Pos, projection);

	output.Colour = float3(0.6, 0.6, 0.6);
	output.UV = (sign(input[0].Pos.xy) + 1.0) / 2.0;
	OutputStream.Append(output);

	OutputStream.RestartStrip();
	//
	// Triangle 3
	//
	// vertex 1
	output.Pos = vPos + float4(quadSize * g_positions[2], 0.0);
	output.Pos = mul(output.Pos, projection);

	output.Colour = float3(0.6, 0.6, 0.6);
	output.UV = (sign(input[0].Pos.xy) + 1.0) / 2.0;
	OutputStream.Append(output);

	// vertex 2
	output.Pos = vPos;
	output.Pos = mul(output.Pos, projection);

	output.Colour = float3(1, 1, 1);
	output.UV = (sign(input[0].Pos.xy) + 1.0) / 2.0;
	OutputStream.Append(output);

	// vertex 3
	output.Pos = vPos + float4(quadSize * g_positions[3], 0.0);
	output.Pos = mul(output.Pos, projection);

	output.Colour = float3(0.6, 0.6, 0.6);
	output.UV = (sign(input[0].Pos.xy) + 1.0) / 2.0;
	OutputStream.Append(output);

	OutputStream.RestartStrip();
	//
	// Triangle 4
	//
	// vertex 1
	output.Pos = vPos;
	output.Pos = mul(output.Pos, projection);

	output.Colour = float3(1, 1, 1);
	output.UV = (sign(input[0].Pos.xy) + 1.0) / 2.0;
	OutputStream.Append(output);

	// vertex 2
	output.Pos = vPos + float4(quadSize * g_positions[2], 0.0);
	output.Pos = mul(output.Pos, projection);

	output.Colour = float3(0.6, 0.6, 0.6);
	output.UV = (sign(input[0].Pos.xy) + 1.0) / 2.0;
	OutputStream.Append(output);

	// vertex 3
	output.Pos = vPos + float4(quadSize * g_positions[0], 0.0);
	output.Pos = mul(output.Pos, projection);

	output.Colour = float3(0.6, 0.6, 0.6);
	output.UV = (sign(input[0].Pos.xy) + 1.0) / 2.0;
	OutputStream.Append(output);
	OutputStream.RestartStrip();
}