struct VS_OUT
{
    float4 outPositionSS    : SV_Position;
    float4 outColor         : COLOR;
    float3 outNormal        : NORMAL;
};

struct VPInverseBuffer
{
    matrix VPInverseMatrix;
};

ConstantBuffer<VPInverseBuffer> vpInverseBuffer : register(b0, space1);

RaytracingAccelerationStructure scene : register(t0, space1);

float4 main(in VS_OUT psIn) : SV_TARGET
{
    RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;

    RayDesc ray;
    ray.Origin = psIn.outPositionSS.xyz;
    ray.Direction = normalize((mul(float4(ray.Origin, 1.0f), vpInverseBuffer.VPInverseMatrix)).xyz);

    ray.TMin = 0.0f;
    ray.TMax = 1000.0f;

    query.TraceRayInline(scene, 0, 0xFF, ray);
    query.Proceed();

    if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    return float4(psIn.outColor.xyz, 1.0f);
}