
struct VS_OUT
{
    float4 outPositionSS : SV_Position;
};

float4 main(in VS_OUT psIn) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}