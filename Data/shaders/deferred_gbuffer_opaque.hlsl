
#include "common_shaders.hlsli"

#ifdef STAGE_VERTEX

cbuffer ConstantData : register(b0)
{
    uint VertexBufferIndex;
    uint IndexBufferIndex;
    uint IndexOffset;
};

cbuffer Camera : register(b1)
{
    float4x4 ModelViewProjection;
};

ByteAddressBuffer Buffers[] : register(t0, space1);

float4 main(uint vertexID : SV_VertexID) : SV_Position
{
    const uint vertexStride = 24;
    
    ByteAddressBuffer IndexBuffer = Buffers[IndexBufferIndex];
    ByteAddressBuffer VertexBuffer = Buffers[VertexBufferIndex];
    
    uint index = IndexBuffer.Load((vertexID + IndexOffset) * 4);
    uint VertexAddress = index * vertexStride;
    float3 Position = float3(asfloat(VertexBuffer.Load(VertexAddress)), asfloat
    (VertexBuffer.Load(VertexAddress + 4)), asfloat(VertexBuffer.Load(VertexAddress + 8)));
    
    return mul(ModelViewProjection, float4(Position, 1.0f));
}

#endif

#ifdef STAGE_PIXEL

float4 main() : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

#endif

