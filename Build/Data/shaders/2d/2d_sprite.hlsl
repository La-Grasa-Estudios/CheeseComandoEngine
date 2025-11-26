#include "shaders/common_shaders.hlsli"

struct v2f
{
    float4 ClipPos : SV_Position;
    float2 TexCoord : TEXCOORD0;
    nointerpolation uint instanceID : INSTANCEID;
};

struct i2v
{
    float2 Position : TEXCOORD0;
    uint instanceID : SV_InstanceID;
};

struct SpriteInstance
{
    float4x4 Transform;
    float4 uv1;
    float4 uv2;
    float4 instanceColor;
    int texture;
    int flags;
	uint userData;
    uint padding;
};

struct ConstantDataStruct
{
    uint BatchIndex;
};

VK_PUSH_CONSTANT ConstantBuffer<ConstantDataStruct> DrawData : REGISTER_CBUFFER(0, 0);

static const int FLAG_NEAREST = 0x1;

StructuredBuffer<SpriteInstance> Instances : REGISTER_SRV(10, 0);

#ifdef STAGE_VERTEX

cbuffer FrameData : register(b1)
{
    float4x4 ProjView[16];
};

static const int uvLut[6] =
{
    0, 0, 1, 0, 1, 1 // Uvs
};

static const int swlut[6] =
{
    0, 1, 1, 0, 1, 0 // Swizzle  
};

v2f main(in i2v input, in uint vertexID : SV_VertexID)
{
    SpriteInstance instance = Instances[input.instanceID + DrawData.BatchIndex];
    float4 uv = uvLut[vertexID] == 0 ? instance.uv1 : instance.uv2;
    
	v2f output;
    output.ClipPos = mul(ProjView[instance.userData & 0xF], mul(instance.Transform, float4(float3(input.Position, 0.0), 1.0)));
    output.ClipPos.z = 0.0f;
    output.TexCoord = swlut[vertexID] == 0 ? uv.xy : uv.zw;
    output.instanceID = input.instanceID + DrawData.BatchIndex;
    
	return output;
}

#endif

#ifdef STAGE_PIXEL

VK_BINDING(0, 1) Texture2D Textures[] : REGISTER_SRV(0, 1);
SamplerState BilinearSampler : register(s0, space0);
SamplerState NearestSampler : register(s1, space0);

float4 main(v2f input) : SV_Target
{
    float4 color = 1.0.xxxx;
    SpriteInstance instance = Instances[input.instanceID];

    if (instance.texture != -1)
    {
        float2 TexCoord = input.TexCoord;
        
        bool useNearest = (instance.flags & FLAG_NEAREST) != 0;
        
        if (useNearest)
            color = Textures[NonUniformResourceIndex(instance.texture)].Sample(NearestSampler, TexCoord);
        else
            color = Textures[NonUniformResourceIndex(instance.texture)].Sample(BilinearSampler, TexCoord);
    }  

    uint userData = instance.userData >> 4;
    
    if (userData == 2)
    {
        float a = color.r;
        clip(color.r - 0.2f);
        color.rgb = 1.0f.rrr;
        color.a = a;
    }
    
    color = color * instance.instanceColor;
	
    return color;
}

#endif