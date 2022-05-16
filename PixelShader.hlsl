struct VS_OUT
{
    float4 outPositionSS    : SV_Position;
    float4 outColor         : COLOR;
    float3 outNormal        : NORMAL;
};

RaytracingAccelerationStructure scene : register(t0, space1);

float4 main(in VS_OUT psIn) : SV_TARGET
{
    return float4(psIn.outNormal.xyz, 1.0f);
}