#include "math.hlsl"
#define LOCAL_SIZE 128

cbuffer Params : register(b0)
{
    uint DrawCallCount;
};

struct D3D12_DRAW_ARGUMENTS
{
    uint VertexCountPerInstance;
    uint InstanceCount;
    uint StartVertexLocation;
    uint StartInstanceLocation;
};

struct PerInstanceData
{
    uint IndicesCount;
    uint InstanceIndex;
    uint VertexBufferIndex;
    uint IndexBufferIndex;
    int MaterialIndex;
};

RWStructuredBuffer<D3D12_DRAW_ARGUMENTS> IndirectArguments : register(u0);
StructuredBuffer<PerInstanceData> InstanceBuffer : register(t0);

[numthreads(LOCAL_SIZE, 1, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    if (dispatchID.x > DrawCallCount)
        return;
    
    D3D12_DRAW_ARGUMENTS args;
    args.InstanceCount = 1;
    args.StartInstanceLocation = dispatchID.x;
    args.StartVertexLocation = 0;
    args.VertexCountPerInstance = InstanceBuffer.Load(dispatchID.x).IndicesCount;
    
    IndirectArguments[dispatchID.x] = args;
}