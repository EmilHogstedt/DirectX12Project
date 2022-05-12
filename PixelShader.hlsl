
struct VS_OUT
{
    float4 outPositionSS    : SV_Position;
    float4 outColor         : COLOR;
};

float4 main(in VS_OUT psIn) : SV_TARGET
{
    return psIn.outColor;
}