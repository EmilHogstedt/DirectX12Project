struct Vertex
{
    float3 inPositionLS;
    float4 inColor;
    float3 inNormal;
};

struct VS_OUT
{
    float4 outPositionCS    : SV_Position;
    float4 outColor         : COLOR;
};

StructuredBuffer<Vertex> vertices : register(t0, space0);
StructuredBuffer<unsigned int> indices: register(t1, space0);

struct WVPConstantBuffer
{
    matrix WVPMatrix;
};

ConstantBuffer<WVPConstantBuffer> wvpConstantBuffer : register(b0, space0);

VS_OUT main(uint vertexID : SV_VertexID)
{
    Vertex input = vertices[indices[vertexID]];
    VS_OUT vsOut = (VS_OUT)0;
    vsOut.outPositionCS = mul(float4(input.inPositionLS, 1.0f), wvpConstantBuffer.WVPMatrix);
    vsOut.outColor = input.inColor;
    return vsOut;
}