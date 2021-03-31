cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

cbuffer CameraConstantBuffer : register(b1)
{
	float3 cameraPosition;
	float padding;
}

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 position : SV_POSITION;
	float2 canvasXY : TEXCOORD0;
};

#define NUMBER_OF_LIGHTS 1

static int MAX_MARCHING_STEPS = 255;
static float MIN_DIST = 1.0;
static float MAX_DIST = 50.0;
static float EPSILON = 0.0001;

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
	{0.2, 0.2, 0.2, 1.0, 0.4, 0.4, 0.4, 1.0, 0.4, 0.4, 0.4, 1.0, -2.0, 5.0, 0.0, 20}
};

struct Ray {
	float3 o; //origin
	float3 d; //direction=
};

struct outputPS
{
	float4 colour : SV_TARGET;
	float depth : SV_DEPTH;
};

float length2(float2 p)
{
	return sqrt(p.x * p.x + p.y * p.y);
}

float length6(float2 p)
{
	p = p * p * p;
	p = p * p;
	return pow(p.x + p.y, 1.0 / 6.0);
}

float length8(float2 p)
{
	p = p * p;
	p = p * p;
	p = p * p;
	return pow(p.x + p.y, 1.0 / 8.0);
}

float dot2(float2 v)
{
	return dot(v, v);
}

//Sphere Signed Distance Function with a radius of 1 (p = sphere point location)

float torusSDF(float3 p, float2 t)
{
	float2 q = float2(length(p.xz) - t.x, p.y);
	return length(q) - t.y;
}

float roundBoxSDF(float3 p, float3 b, float r)
{
	float3 q = abs(p) - b;
	return min(max(q.x, max(q.y, q.z)), 0.0) + length(max(q, 0.0)) - r;
}

float cylinderSDF(float3 p, float2 h)
{
	float2 d = abs(float2(length(p.xz), p.y)) - h;
	return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float cylinder6SDF(float3 p, float2 h)
{
	return max(length6(p.xz) - h.x, abs(p.y) - h.y);
}

float cylinderSDF(float3 p, float3 a, float3 b, float r)
{
	float3 pa = p - a;
	float3 ba = b - a;
	float baba = dot(ba, ba);
	float paba = dot(pa, ba);

	float x = length(pa * baba - ba * paba) - r * baba;
	float y = abs(paba - baba * 0.5) - baba * 0.5;
	float x2 = x * x;
	float y2 = y * y * baba;
	float d = (max(x, y) < 0.0) ? -min(x2, y2) : (((x > 0.0) ? x2 : 0.0) + ((y > 0.0) ? y2 : 0.0));
	return sign(d) * sqrt(abs(d)) / baba;
}

float torus82SDF(float3 p, float2 t)
{
	float2 q = float2(length2(p.xz) - t.x, p.y);
	return length8(q) - t.y;
}

float boxSDF(float3 p, float3 b)
{
	float3 d = abs(p) - b;
	return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

float coneSDF(float3 p, float3 c)
{
	float2 q = float2(length(p.xz), p.y);
	float d1 = -q.y - c.z;
	float d2 = max(dot(q, c.xy), q.y);
	return length(max(float2(d1, d2), 0.0)) + min(max(d1, d2), 0.0);
}

float cappedConeSDF(float3 p, float h, float r1, float r2)
{
	float2 q = float2(length(p.xz), p.y);

	float2 k1 = float2(r2, h);
	float2 k2 = float2(r2 - r1, 2.0 * h);
	float2 ca = float2(q.x - min(q.x, (q.y < 0.0) ? r1 : r2), abs(q.y) - h);
	float2 cb = q - k1 + k2 * clamp(dot(k1 - q, k2) / dot2(k2), 0.0, 1.0);
	float s = (cb.x < 0.0 && ca.y < 0.0) ? -1.0 : 1.0;
	return s * sqrt(min(dot2(ca), dot2(cb)));
}

float roundConeSDF(float3 p, float r1, float r2, float h)
{
	float2 q = float2(length(p.xz), p.y);

	float b = (r1 - r2) / h;
	float a = sqrt(1.0 - b * b);
	float k = dot(q, float2(-b, a));

	if (k < 0.0) return length(q) - r1;
	if (k > a * h) return length(q - float2(0.0, h)) - r2;

	return dot(q, float2(a, b)) - r1;
}

float roundConeSDF(float3 p, float3 a, float3 b, float r1, float r2)
{
	float3  ba = b - a;
	float l2 = dot(ba, ba);
	float rr = r1 - r2;
	float a2 = l2 - rr * rr;
	float il2 = 1.0 / l2;

	float3 pa = p - a;
	float y = dot(pa, ba);
	float z = y - l2;
	float x2 = dot2(pa * l2 - ba * y);
	float y2 = y * y * l2;
	float z2 = z * z * l2;

	float k = sign(rr) * rr * rr * x2;
	if (sign(z) * a2 * z2 > k) return  sqrt(x2 + z2) * il2 - r2;
	if (sign(y) * a2 * y2 < k) return  sqrt(x2 + y2) * il2 - r1;
	return (sqrt(x2 * a2 * il2) + y * rr) * il2 - r1;
}

float ellipsoidSDF(float3 p, float3 r)
{
	float k0 = length(p / r);
	float k1 = length(p / (r * r));
	return k0 * (k0 - 1.0) / k1;

}

float equilateralTriangleSDF(float2 p)
{
	const float k = 1.73205;
	p.x = abs(p.x) - 1.0;
	p.y = p.y + 1.0 / k;
	if (p.x + k * p.y > 0.0) p = float2(p.x - k * p.y, -k * p.x - p.y) / 2.0;
	p.x += 2.0 - 2.0 * clamp((p.x + 2.0) / 2.0, 0.0, 1.0);
	return -length(p) * sign(p.y);
}

float triPrismSDF(float3 p, float2 h)
{
	float3 q = abs(p);
	float d1 = q.z - h.y;
	h.x *= 0.866025;
	float d2 = equilateralTriangleSDF(p.xy / h.x) * h.x;
	return length(max(float2(d1, d2), 0.0)) + min(max(d1, d2), 0.);
}

float hexPrismSDF(float3 p, float2 h)
{
	float3 q = abs(p);

	const float3 k = float3(-0.8660254, 0.5, 0.57735);
	p = abs(p);
	p.xy -= 2.0 * min(dot(k.xy, p.xy), 0.0) * k.xy;
	float2 d = float2(
		length(p.xy - float2(clamp(p.x, -k.z * h.x, k.z * h.x), h.x)) * sign(p.y - h.x),
		p.z - h.y);
	return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float octahedronSDF(float3 p, float s)
{
	p = abs(p);

	float m = p.x + p.y + p.z - s;

	float3 q;
	if (3.0 * p.x < m) q = p.xyz;
	else if (3.0 * p.y < m) q = p.yzx;
	else if (3.0 * p.z < m) q = p.zxy;
	else return m * 0.57735027;

	float k = clamp(0.5 * (q.z - q.y + s), 0.0, s);
	return length(float3(q.x, q.y - s + k, q.z - k));
}

float4 unionSDF(float4 sdfDisOne, float4 sdfDisTwo)
{
	return (sdfDisOne.x < sdfDisTwo.x) ? sdfDisOne : sdfDisTwo;
}

float3 twistSDF(float3 p, float rep)
{
	float  c = cos(rep * p.y + rep);
	float  s = sin(rep * p.y + rep);
	float2x2   m = float2x2(c, -s, s, c);
	return float3(mul(p.xz, m), p.y);
}

static float3 va = float3(0.0, 0.57735, 0.0);
static float3 vb = float3(0.0, -1.0, 1.15470);
static float3 vc = float3(1.0, -1.0, -0.57735);
static float3 vd = float3(-1.0, -1.0, -0.57735);

float4 SierpinskiTetrahedron(float3 p)
{
	float a = 0.0;
	float s = 1.0;
	float r = 1.0;
	float dm;
	float3 v;
	for (int i = 0; i < 8; i++)
	{
		float d, t;
		d = dot(p - va, p - va);              v = va; dm = d; t = 0.0;
		d = dot(p - vb, p - vb); if (d < dm) { v = vb; dm = d; t = 1.0; }
		d = dot(p - vc, p - vc); if (d < dm) { v = vc; dm = d; t = 2.0; }
		d = dot(p - vd, p - vd); if (d < dm) { v = vd; dm = d; t = 3.0; }
		p = v + 2.0 * (p - v); r *= 2.0;
	}

	return float4((sqrt(dm) - 1.0) / r, saturate(p));
}

//Signed Distance Function for the scene, function return value of called SDF 
//Determines location of P relative to the surface of the function (sphere)
float4 sceneSDF(float3 samplePoint)
{
	//Contains hit distance (x) and colour (yzw)
	float4 closestHit = float4(1e10, 0.0f, 0.0f, 0.0f);
	
	//Ray Marched Implicit Geometric Primitives
	closestHit = unionSDF(closestHit, float4(roundConeSDF(samplePoint - float3(0.3, 0.5f, 0.3), float3(0.02, 0.0, 0.0), float3(-0.02, 0.06, 0.02), 0.03, 0.01), 0.18f, 0.22f, 1.0f));
	closestHit = unionSDF(closestHit, float4(coneSDF(samplePoint - float3(0.0, 0.53f, 0.0), float3(0.16, 0.12, 0.06)), 0.55f, 0.23f, 0.38f));
	closestHit = unionSDF(closestHit, float4(cappedConeSDF(samplePoint - float3(0.3, 0.5f, 0.0f), 0.03, 0.04, 0.02), 0.80f, 0.78f, 0.45f));
	closestHit = unionSDF(closestHit, float4(0.6 * torusSDF(twistSDF(samplePoint - float3(0.0, 0.5f, 0.3), 60.0f), float2(0.04, 0.01)), 0.28f, 0.51f, 0.08f));
	closestHit = unionSDF(closestHit, float4(torusSDF(samplePoint - float3(-0.3, 0.5f, -0.3), float2(0.04, 0.01)), 0.41f, 0.27f, 0.54f));
	closestHit = unionSDF(closestHit, float4(torus82SDF(samplePoint - float3(0.0, 0.5f, -0.3), float2(0.04, 0.01)), 0.52f, 0.75f, 0.42f));
	closestHit = unionSDF(closestHit, float4(boxSDF(samplePoint - float3(-0.3, 0.5f, 0.0), float3(0.05f, 0.05f, 0.05f)), 0.31f, 0.47f, 0.63f));
	closestHit = unionSDF(closestHit, float4(roundBoxSDF(samplePoint - float3(-0.3, 0.5f, 0.3), float3(0.04f, 0.04f, 0.04f), 0.016), 1.0f, 0.27f, 0.0f));
	closestHit = unionSDF(closestHit, float4(ellipsoidSDF(samplePoint - float3(0.3, 0.5f, -0.3), float3(0.05, 0.05, 0.02)), 0.8f, 0.41f, 0.79f));
	closestHit = unionSDF(closestHit, float4(triPrismSDF(samplePoint - float3(-0.6, 0.5f, -0.3), float2(0.05, 0.02)), 0.92f, 0.68f, 0.92f));
	closestHit = unionSDF(closestHit, float4(cylinderSDF(samplePoint - float3(-0.6, 0.5f, 0.0), float3(0.002, -0.002, 0.0), float3(-0.02, 0.06, 0.02), 0.016), 0.78f, 0.38f, 0.08f));
	closestHit = unionSDF(closestHit, float4(cylinderSDF(samplePoint - float3(-0.6, 0.5f, 0.3), float2(0.02, 0.04)), 0.98f, 0.63f, 0.42f));
	closestHit = unionSDF(closestHit, float4(cylinder6SDF(samplePoint - float3(0.3, 0.5f, 0.6), float2(0.02, 0.04)), 0.29f, 0.46f, 0.43f));
	closestHit = unionSDF(closestHit, float4(octahedronSDF(samplePoint - float3(0.0, 0.5f, 0.6), 0.07), 0.46f, 0.61f, 0.52f));
	closestHit = unionSDF(closestHit, float4(hexPrismSDF(samplePoint - float3(-0.3, 0.5f, 0.6), float2(0.05, 0.01)), 0.59f, 1.0f, 1.0f));
	closestHit = unionSDF(closestHit, float4(roundConeSDF(samplePoint - float3(-0.6, 0.5f, 0.6), 0.04, 0.02, 0.06), 1.0f, 0.2f, 0.0f));

	//SierpinskiTetrahedron
	closestHit = unionSDF(closestHit, SierpinskiTetrahedron(2.0f * (samplePoint - float3(-1.0f, 2.0f, -2.0f))));
	
	return closestHit;
}

//Calculate surface normals using the gradiant around a point by sampling through SDF
float3 estimateGradiantNormal(float3 p)
{
	return normalize(float3(sceneSDF(float3(p.x + EPSILON, p.y, p.z)).x - sceneSDF(float3(p.x - EPSILON, p.y, p.z)).x,
		sceneSDF(float3(p.x, p.y + EPSILON, p.z)).x - sceneSDF(float3(p.x, p.y - EPSILON, p.z)).x,
		sceneSDF(float3(p.x, p.y, p.z + EPSILON)).x - sceneSDF(float3(p.x, p.y, p.z - EPSILON)).x));
}

float4 PhongIllumination(float surfacePoint, float3 normal, float shininess, float3 rayDirection, float4 diffuseColour)
{
	float4 totalAmbient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 totalDiffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 totalSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);

	for (int i = 0; i < NUMBER_OF_LIGHTS; i++)
	{
		totalAmbient += lights[i].ambientColour * diffuseColour;

		float3 lightDirection = normalize(lights[i].lightPosition - surfacePoint);
		float nDotL = dot(normal, lightDirection);
		float3 reflection = normalize(reflect(-lightDirection, normal));
		float rDotV = max(0.0f, dot(reflection, -rayDirection));

		totalDiffuse += saturate(lights[i].diffuseColour * nDotL * diffuseColour);

		if (nDotL > 0.0f)
		{
			float4 specularIntensity = float4(1.0, 1.0, 1.0, 1.0);
			totalSpecular += lights[i].specularColour * pow(pow(rDotV, lights[i].specularPower), shininess) * specularIntensity;
		}
	}

	return totalAmbient + totalDiffuse + totalSpecular;
}

//start = starting distance away from origin
//end = max travel distance away from origin
float4 rayMarching(Ray ray, float start, float end)
{
	float depth = start;

	for (int i = 0; i < MAX_MARCHING_STEPS; i++)
	{
		float4 distanceAndColour = sceneSDF(ray.o + depth * ray.d);

		if (distanceAndColour.x < EPSILON)
		{
			//Hit the surface
			return float4(depth, distanceAndColour.yzw);
		}

		//Move along the ray
		depth += distanceAndColour.x;

		if (depth >= end)
		{
			//Give up, nothing hit along max travel distance
			return float4(end, 0.0f, 0.0f, 0.0f);
		}
	}

	return float4(end, 0.0f, 0.0f, 0.0f);
}

// A pass-through function for the (interpolated) color data.
outputPS main(PixelShaderInput input)
{
	outputPS output;

	float3 PixelPos = float3(input.canvasXY, -MIN_DIST);

	Ray eyeray;
	eyeray.o = cameraPosition;
	eyeray.d = normalize(mul(float4(PixelPos, 0.0f), transpose(view)));

	float4 distanceAndColour = rayMarching(eyeray, EPSILON, MAX_DIST);

	if (distanceAndColour.x > MAX_DIST - EPSILON)
	{
		discard;
	}

	float3 surfacePoint = cameraPosition + distanceAndColour.x * eyeray.d;

	float4 pv = mul(float4(surfacePoint, 1.0f), view);
	pv = mul(pv, projection);
	output.depth = pv.z / pv.w;

	output.colour = PhongIllumination(surfacePoint, estimateGradiantNormal(surfacePoint), 40.0f, eyeray.d, float4(distanceAndColour.yzw, 1.0));

	output.colour = float4(lerp(output.colour.xyz, float3(1.0f, 0.97255f, 0.86275f), 1.0 - exp(-0.0005 * distanceAndColour.x * distanceAndColour.x * distanceAndColour.x)), 1.0f);

	return output;
}