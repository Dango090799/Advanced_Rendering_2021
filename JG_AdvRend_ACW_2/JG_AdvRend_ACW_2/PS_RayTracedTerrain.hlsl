static float EPSILON = 0.0001;
static float nearPlane = 1.00;
static float farPlane = 100.0;

cbuffer ModelViewProjCB : register(b0)
{
    matrix model;
    matrix view;
    matrix projection;
}

cbuffer CameraCB : register(b2)
{
    float3 cameraPos;
    float padding;
};

cbuffer InverseCB : register(b1) {
    matrix invView;
}

static float4 LightColor = float4(1, 1, 1, 1);
static float3 LightPos = float3(0, 10, 2);

struct Material
{
    float4 color;
    float Kd, Ks, Kr, shininess;
};

static float shininess = 40.0f;
static float4 planeColor = float4(1, 1, 1, 1);

static Material materials[1] =
{
    { planeColor, 0.3, 0.5, 0.7, shininess },
};

struct VS_Quad
{
    float4 position : SV_POSITION;
    float2 canvasXY : TEXCOORD0;
};

struct Ray
{
    float3 o; // origin
    float3 d; // direction
};

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

float plane(float3 p, float3 origin, float3 normal) {
    //return dot(p - origin, normal);
    return dot(p, normal) + noise(origin);
}

float PlaneIntersect(Ray ray, float3 normal, float3 centre, out bool hit)
{
    float t;
    float den = dot(normal, ray.d);

	if(abs(den) > EPSILON)
	{
        t = dot(centre - ray.o, normal) / den;

		if(t > EPSILON)
		{
            hit = true;
            return t;
		}
	}

    hit = false;
    return t;
}

float3 NearestHit(Ray ray, out int hitobj, out bool anyhit, out float mint, out float3 n)
{
    mint = farPlane;
    hitobj = -1;
    anyhit = false;

	// Test for hit on plane using PlaneIntersect
    bool hit = false;
    float t = PlaneIntersect(ray, float3(0,1,0), float3(0,-15,0), hit);
    if (hit)
    {
        if (t < mint)
        {
            hitobj = 0;
            mint = t;
            anyhit = true;
        }
    }
	
    n = float3(0, 1, 0);
    return ray.o + ray.d * mint;
}

float4 Phong(float3 n, float3 l, float3 v, float shininess, float4 diffuseColor, float4 specularColor)
{
    float NdotL = dot(n, l);
    float diff = saturate(NdotL);
    float3 r = reflect(l, n);
    float spec = pow(saturate(dot(v, r)), shininess) * (NdotL > 0.0);
    return diff * diffuseColor + spec * specularColor;
}

float4 Shade(float3 hitPos, float3 normal, float3 viewDir, int hitobj, float lightIntensity)
{
    Ray shadowRay;
    shadowRay.o = hitPos.xyz;
    shadowRay.d = LightPos - hitPos.xyz;
    float shadowFactor = 1.0;

    float3 lightDir = normalize(LightPos - hitPos);
    float4 diff = materials[hitobj].color * materials[hitobj].Kd;
    float4 spec = materials[hitobj].color * materials[hitobj].Ks;
    return LightColor * lightIntensity * Phong(normal, lightDir, viewDir, materials[hitobj].shininess, diff, spec) * shadowFactor;
}

float4 RayTracing(Ray ray, out bool anyHit)
{
    int hitobj;
    bool hit = false;
    float3 n;
    float4 c = (float4) 0;
    float lightInensity = 1.0;
    float mint = 0.0f;

    float3 i = NearestHit(ray, hitobj, hit, mint, n);

    for (int depth = 1; depth < 5; depth++)
    {
        if (hit)
        {
            anyHit = true;

            c += Shade(i, n, ray.d, hitobj, lightInensity);

            // shoot refleced ray
            lightInensity *= materials[hitobj].Kr;
            ray.o = i;
            ray.d = reflect(ray.d, n);
            i = NearestHit(ray, hitobj, hit, mint, n);
        }
    }
    return float4(c.xyz, mint);
}

struct outputPS
{
    float4 colour : SV_TARGET;
    float depth : SV_DEPTH;
};

outputPS main(VS_Quad input)
{
    outputPS output;

    float3 PixelPos = float3(input.canvasXY, -nearPlane);

    Ray eyeray;
    eyeray.o = mul(float4(float3(0.0f, 0.0f, 0.0f), 1.0f), invView);
    eyeray.d = normalize(mul(float4(PixelPos, 0.0f), invView));

    bool anyhit = false;
    float4 distanceAndColour = RayTracing(eyeray, anyhit);

    if (distanceAndColour.x > farPlane - EPSILON)
    {
        discard;
    }

    if (!anyhit) discard;
	
    float3 surfacePoint = cameraPos + distanceAndColour.x * eyeray.d;

    float4 pv = mul(float4(surfacePoint, 1.0f), view);
    pv = mul(pv, projection);
    output.depth = pv.z / pv.w;

    output.colour = float4(distanceAndColour.yzw, 1.0f);

    //output.colour = float4(lerp(output.colour.xyz, float3(1.0f, 0.97255f, 0.86275f), 1.0 - exp(-0.0005 * distanceAndColour.x * distanceAndColour.x * distanceAndColour.x)), 1.0f);

    return output;
}

//float4 main(VS_Quad input) : SV_TARGET
//{
//    float dist2Imageplane = nearPlane;
//    float3 PixelPos = float3(input.canvasXY, -dist2Imageplane);
//
//    Ray eyeray;
//    eyeray.o = mul(float4(float3(0.0f, 0.0f, 0.0f), 1.0f), invView);
//    eyeray.d = normalize(mul(float4(PixelPos, 0.0f), invView));
//
//    bool anyHit = false;
//    float4 colorDistance = RayTracing(eyeray, anyHit);
//
//    if (!anyHit)
//        discard;
//
//    return float4(colorDistance.xyz, 1.0);
//}