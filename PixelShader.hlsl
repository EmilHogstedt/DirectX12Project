struct VS_OUT
{
    float4 outPositionSS    : SV_Position;
    float4 outPosWorld      : POSWORLD;
    float3 outNormal        : NORMAL;
};

struct PointLight
{
    float3 pos;
    float3 col;
};

struct ObjectColor
{
    float4 color;
};

struct VPInverseBuffer
{
    matrix VPInverseMatrix;
    float2 elementsP;
};

struct CameraBuffer
{
    float3 pos;
};

RaytracingAccelerationStructure scene : register(t0, space1);

ConstantBuffer<VPInverseBuffer> vpInverseBuffer : register(b0, space1);
ConstantBuffer<ObjectColor> objectColor : register(b1, space1);
ConstantBuffer<CameraBuffer> camera : register(b2, space1);

static const PointLight light1 = { -40.0f, 20.0f, 80.0f, 1.0f, 0.0f, 0.0f };
static const PointLight light2 = { 50.0f, 50.0f, 10.0f, 0.7f, 0.7f, 0.3f };
static const PointLight light3 = { 0.0f, 50.0f, -5.0f, 0.0f, 0.7f, 0.7f };

//Modifiers
static const float ambient = 0.2f;
static const float specular = 0.6f;
static const float diffuse = 0.7f;

float3 CalculateLight(PointLight light, float4 outPosWorld, float3 normal, float3 cameraPos, float4 color)
{
    float dist = length(light.pos - outPosWorld);
    float attenuation = 1.0f / (1.0f + 0.0f * dist + 0.0001f * (dist * dist));
    float3 lightDir = normalize(light.pos - outPosWorld);
    float3 viewDir = normalize(camera.pos - outPosWorld);

    //Ambient
    float3 ambientColor = ambient * light.col;
    
    //Diffuse
    float diff = max(dot(lightDir, normal), 0.0f);
    float3 diffuseColor = diff * light.col * diffuse;

    //Specular
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 64);
    float3 specularColor = specular * spec * light.col;
    
    ambientColor = ambientColor * color.xyz * attenuation;
    diffuseColor = diffuseColor * color.xyz * attenuation;
    specularColor = specularColor * color.xyz * attenuation;
    //float3 finalCol = (ambientColor + diffuseColor + specularColor) * color.xyz;
    return (ambientColor + diffuseColor + specularColor);
}

float4 main(in VS_OUT psIn) : SV_TARGET
{
    float3 normal = normalize(psIn.outNormal);

    float3 result = float3(0.0f, 0.0f, 0.0f);
    result += CalculateLight(light3, psIn.outPosWorld, normal, camera.pos, objectColor.color);
    result += CalculateLight(light1, psIn.outPosWorld, normal, camera.pos, objectColor.color);
    result += CalculateLight(light2, psIn.outPosWorld, normal, camera.pos, objectColor.color);

    //Shadows with raytracing
    /*
    RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;

    RayDesc ray;
    ray.Origin = psIn.outPosWorld.xyz;
    ray.Direction = normalize(outPosWorld.xyz - light1.pos);

    ray.TMin = 0.0f;
    ray.TMax = 10000.0f;

    query.TraceRayInline(scene, 0, 0xFF, ray);
    query.Proceed();

    if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        result *= 0.2f;
    }
    */

    return float4(result, objectColor.color.w);
}