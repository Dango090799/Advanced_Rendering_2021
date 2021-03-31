static float EPSILON = 0.0001;
static float nearPlane = 1.00;
static float farPlane = 100.0;

static float4 LightColor = float4(1, 1, 1, 1);
static float3 LightPos = float3(0, 10, 2);

static float4 sphereColor_1 = float4(1, 0, 0, 1); //sphere1 color
static float4 sphereColor_2 = float4(0, 1, 0, 1); //sphere2 color
static float4 sphereColor_3 = float4(1, 0, 1, 1); //sphere3 color

static float4 cubeColor_1 = float4(1, 1, 1, 1); //cube color
static float4 cubeColor_2 = float4(0, 0, 1, 1); //cube color
static float4 cubeColor_3 = float4(1, 1, 0, 1); //cube color

static float shininess = 10;

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

struct Sphere
{
    float3 centre;
    float rad2; // radius * radius

};

struct Cube
{
    float3 mi;
    float3 ma;
};

struct Material
{
    float4 color;
    float Kd, Ks, Kr, shininess;
};

static const int matOffsetSphere = 0;
static const int matOffsetCube = 3;

#define NOBJECTS 6
static Material materials[NOBJECTS] =
{
    { sphereColor_1, 0.3, 0.5, 0.7, shininess },
    { sphereColor_2, 0.5, 0.7, 0.4, shininess },
    { sphereColor_3, 0.5, 0.3, 0.3, shininess },
    { cubeColor_1, 0.5, 0.3, 0.3, shininess },
    { cubeColor_2, 0.5, 0.3, 0.3, shininess },
    { cubeColor_3, 0.5, 0.3, 0.3, shininess },
};

#define NOSPHERES 3
static Sphere spheres[NOSPHERES] =
{
    { -1.5, 1.5f, 0.9, 0.0075 },
    { -1.8, 1.3f, 1.0, 0.004 },
    { -1.6, 1.4f, 1.3, 0.01 }
};

#define NOCUBES 3
static Cube cubes[NOCUBES] =
{
    { -1.4, 1.2, 0.9, -1.3, 1.3, 1.0 },
    { -1.8, 1.5, 1.1, -1.7, 1.6, 1.2 },
    { -1.8, 1.3, 0.7, -1.7, 1.4, 0.8 }
};

float3 SphereNormal(Sphere s, float3 pos)
{
    return normalize(pos - s.centre);
}

float SphereIntersect(Sphere s, Ray ray, out bool hit)
{
    float t;
    float3 v = s.centre - ray.o;
    float A = dot(v, ray.d);
    float B = dot(v, v) - A * A;
    float R = sqrt(s.rad2);
    if (B > R * R)
    {
        hit = false;
        t = farPlane;
    }
    else
    {
        float disc = sqrt(R * R - B);
        t = A - disc;
        if (t < 0.0)
        {
            hit = false;
        }
        else
            hit = true;
    }
    return t;
}

float3 get_normal(int face_hit)
{
    switch (face_hit)
    {
    case 0:
        return (float3(-1, 0, 0)); // -x face
    case 1:
        return (float3(0, -1, 0)); // -y face
    case 2:
        return (float3(0, 0, -1)); // -z face
    case 3:
        return (float3(1, 0, 0)); // +x face
    case 4:
        return (float3(0, 1, 0)); // +y face
    case 5:
        return (float3(0, 0, 1)); // +z face
    }
    return (float3)0;
}

float CubeIntersect(Ray ray, Cube cube, out bool hit, out float3 normal)
{
    float t = -1;
    float3 p0 = cube.mi;
    float3 p1 = cube.ma;
    float3 o = ray.o;
    float3 d = ray.d;
    float3 t_min;
    float3 t_max;

    double a = 1.0 / d.x;
    if (a >= 0) {
        t_min.x = (p0.x - o.x) * a;
        t_max.x = (p1.x - o.x) * a;
    }
    else {
        t_min.x = (p1.x - o.x) * a;
        t_max.x = (p0.x - o.x) * a;
    }
    double b = 1.0 / d.y;
    if (b >= 0) {
        t_min.y = (p0.y - o.y) * b;
        t_max.y = (p1.y - o.y) * b;
    }
    else {
        t_min.y = (p1.y - o.y) * b;
        t_max.y = (p0.y - o.y) * b;
    }
    double c = 1.0 / d.z;
    if (c >= 0) {
        t_min.z = (p0.z - o.z) * c;
        t_max.z = (p1.z - o.z) * c;
    }
    else {
        t_min.z = (p1.z - o.z) * c;
        t_max.z = (p0.z - o.z) * c;
    }
    double t0, t1;
    int face_in, face_out;
    // finding largest
    if (t_min.x > t_min.y) {
        t0 = t_min.x;
        face_in = (a >= 0.0) ? 0 : 3;
    }
    else {
        t0 = t_min.y;
        face_in = (b >= 0.0) ? 1 : 4;
    }
    if (t_min.z > t0) {
        t0 = t_min.z;
        face_in = (c >= 0.0) ? 2 : 5;
    }
    // find smallest
    if (t_max.x < t_max.y) {
        t1 = t_max.x;
        face_out = (a >= 0.0) ? 3 : 0;
    }
    else {
        t1 = t_max.y;
        face_out = (b >= 0.0) ? 4 : 1;
    }
    if (t_max.z < t1) {
        t1 = t_max.z;
        face_out = (c >= 0.0) ? 5 : 2;
    }
    if (t0 < t1 && t1 > EPSILON)
    {
        if (t0 > EPSILON)
        {
            t = t0;
            normal = get_normal(face_in);
        }
        else {
            t = t1;
            normal = get_normal(face_out);
        }

        hit = true;
        return t;
    }
    else
    {
        hit = false;
        return t;
    }
}

float3 NearestHit(Ray ray, out int hitobj, out bool anyhit, out float mint, out float3 n)
{
    mint = farPlane;
    hitobj = -1;
    anyhit = false;
    for (int i = 0; i < NOSPHERES; i++)
    {
        bool hit = false;
        float t = SphereIntersect(spheres[i], ray, hit);
        if (hit)
        {
            if (t < mint)
            {
                hitobj = i;
                mint = t;
                anyhit = true;
            }
        }
    }

    for (int j = 0; j < NOCUBES; j++)
    {
        bool hit = false;
        float3 cn;
        float t = CubeIntersect(ray, cubes[j], hit, cn);
        if (hit)
        {
            if (t < mint)
            {
                hitobj = j + matOffsetCube;
                mint = t;
                anyhit = true;
                n = cn;
            }
        }
    }

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
    anyHit = false;

    float3 i = NearestHit(ray, hitobj, hit, mint, n);

    for (int depth = 1; depth < 5; depth++)
    {
        if (hit)
        {
            anyHit = true;

            if (hitobj >= 0 && hitobj < 3)
            {
                n = SphereNormal(spheres[hitobj], i);
            }

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

float4 main(VS_Quad input) : SV_TARGET
{
    float dist2Imageplane = nearPlane;
    float3 PixelPos = float3(input.canvasXY, -dist2Imageplane);

    Ray eyeray;
    eyeray.o = mul(float4(float3(0.0f, 0.0f, 0.0f), 1.0f), invView);
    eyeray.d = normalize(mul(float4(PixelPos, 0.0f), invView));

    bool anyHit = false;
    float4 colorDistance = RayTracing(eyeray, anyHit);

    if (!anyHit)
        discard;

    return float4(colorDistance.xyz, 1.0);
}
