cbuffer cb : register(b0)
{
    row_major float4x4 worldTransform : packoffset(c0);
    row_major float4x4 viewTransform : packoffset(c197);
};

struct VertexInput
{
    float3 inPos : POSITION;
    float3 inColor : COLOR;
};

struct VertexOutput
{
    float3 color : COLOR;
    float4 position : SV_Position;
};

VertexOutput main(VertexInput vertexInput)
{
    float3 inColor = vertexInput.inColor;
    float3 inPos = vertexInput.inPos;
    float3 outColor = inColor;
    float4 position = mul(mul(worldTransform, float4(inPos, 1.0)), viewTransform);
    
    VertexOutput output;
    output.position = position;
    output.color = outColor;
    return output;
}
