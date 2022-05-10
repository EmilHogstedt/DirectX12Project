
struct VS_OUT
{
    float4 outPositionSS : SV_Position;
};

struct ColorCB
{
    float3 Color;
};

ConstantBuffer<ColorCB> colorConstantBuffer : register(b0, space0);

float4 main(in VS_OUT psIn) : SV_TARGET
{
    return float4(colorConstantBuffer.Color, 1.0f);
}