
#include "common_shaders.hlsli"

struct v2f
{
    float4 ClipPosition : SV_Position;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 TexCoord : TEXCOORD0;
};

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

v2f main(uint vertexID : SV_VertexID)
{
    const uint vertexStride = 24;
    
    ByteAddressBuffer IndexBuffer = Buffers[IndexBufferIndex];
    ByteAddressBuffer VertexBuffer = Buffers[VertexBufferIndex];
    
    uint index = IndexBuffer.Load((vertexID + IndexOffset) * 4);
    
    uint VertexAddress = index * vertexStride;
    uint TexCoordAddress = VertexAddress + 12;
    uint NormalAddress = VertexAddress + 16;
    uint TangentAddress = VertexAddress + 20;
    
    float3 Position = float3(asfloat(VertexBuffer.Load(VertexAddress)), asfloat
    (VertexBuffer.Load(VertexAddress + 4)), asfloat(VertexBuffer.Load(VertexAddress + 8)));
    
    float2 TexCoord = UnpackFloat2From16(VertexBuffer.Load(TexCoordAddress));
    float3 Normal = UnpackSNorm4x8(VertexBuffer.Load(NormalAddress)).xyz;
    
    v2f o;
    
    o.ClipPosition = mul(ModelViewProjection, float4(Position, 1.0f));
    o.Normal = Normal;
    o.TexCoord = TexCoord;
    
    return o;
}

#endif

#ifdef STAGE_PIXEL

struct DeferredOutput
{
    float3 Color : SV_Target0;
    float2 Normal : SV_Target1;
};

float2 OctWrap(float2 v)
{
    return (1.0 - abs(v.yx)) * sign(v.xy);
}
 
float2 PackNormal(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xy = n.z >= 0.0 ? n.xy : OctWrap(n.xy);
    n.xy = n.xy * 0.5 + 0.5;
    return n.xy;
}

DeferredOutput main(v2f i)
{
    DeferredOutput o;
    
    o.Color = 1.0f.xxx;
    o.Normal = PackNormal(i.Normal);
    
    return o;
}

#endif

