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

VK_PUSH_CONSTANT cbuffer DrawData : register(b0)
{
    uint BatchIndex;
};

static const int FLAG_NEAREST = 0x1;

StructuredBuffer<SpriteInstance> Instances : register(t10);

#ifdef STAGE_VERTEX

cbuffer FrameData : register(b1)
{
    float4x4 ProjView[2];
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
    SpriteInstance instance = Instances[input.instanceID + BatchIndex];
    float4 uv = uvLut[vertexID] == 0 ? instance.uv1 : instance.uv2;
    
	v2f output;
    output.ClipPos = mul(ProjView[instance.userData & 0x1], mul(instance.Transform, float4(float3(input.Position, 0.0), 1.0)));
    output.TexCoord = swlut[vertexID] == 0 ? uv.xy : uv.zw;
    output.instanceID = input.instanceID + BatchIndex;
    
	return output;
}

#endif

cbuffer Custom : register(b2)
{
    float time;
	float dissolve;
};

#ifdef STAGE_PIXEL

VK_BINDING(0, 2) Texture2D Textures[] : register(t0, space2);
SamplerState BilinearSampler : register(s0);
SamplerState NearestSampler : register(s1);

float3 mod289(float3 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float2 mod289(float2 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float3 permute(float3 x) {
    return mod289((x * 34.0 + 1.0) * x);
}

float3 taylorInvSqrt(float3 r) {
    return 1.79284291400159 - 0.85373472095314 * r;
}

// output noise is in range [-1, 1]
float snoise(float2 v) {
    const float4 C = float4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                            0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                            -0.577350269189626, // -1.0 + 2.0 * C.x
                            0.024390243902439); // 1.0 / 41.0

    // First corner
    float2 i  = floor(v + dot(v, C.yy));
    float2 x0 = v -   i + dot(i, C.xx);

    // Other corners
    float2 i1;
    i1.x = step(x0.y, x0.x);
    i1.y = 1.0 - i1.x;

    // x1 = x0 - i1  + 1.0 * C.xx;
    // x2 = x0 - 1.0 + 2.0 * C.xx;
    float2 x1 = x0 + C.xx - i1;
    float2 x2 = x0 + C.zz;

    // Permutations
    i = mod289(i); // Avoid truncation effects in permutation
    float3 p =
      permute(permute(i.y + float3(0.0, i1.y, 1.0))
                    + i.x + float3(0.0, i1.x, 1.0));

    float3 m = max(0.5 - float3(dot(x0, x0), dot(x1, x1), dot(x2, x2)), 0.0);
    m = m * m;
    m = m * m;

    // Gradients: 41 points uniformly over a line, mapped onto a diamond.
    // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
    float3 x = 2.0 * frac(p * C.www) - 1.0;
    float3 h = abs(x) - 0.5;
    float3 ox = floor(x + 0.5);
    float3 a0 = x - ox;

    // Normalise gradients implicitly by scaling m
    m *= taylorInvSqrt(a0 * a0 + h * h);

    // Compute final noise value at P
    float3 g = float3(
        a0.x * x0.x + h.x * x0.y,
        a0.y * x1.x + h.y * x1.y,
        g.z = a0.z * x2.x + h.z * x2.y
    );
    return 130.0 * dot(m, g);
}

float snoise01(float2 v) {
    return snoise(v) * 0.5 + 0.5;
}

float4 main(v2f input) : SV_Target
{
    float4 color = 1.0.xxxx;
    SpriteInstance instance = Instances[input.instanceID];
	
	float noise = snoise01(input.TexCoord * 10.0f) + 1.0f - dissolve * 2.0f;
		
    
    if (instance.texture != -1)
    {
        float2 TexCoord = input.TexCoord;
        
        bool useNearest = (instance.flags & FLAG_NEAREST) != 0;
        
        if (useNearest)
            color = Textures[NonUniformResourceIndex(instance.texture)].Sample(NearestSampler, TexCoord);
        else
            color = Textures[NonUniformResourceIndex(instance.texture)].Sample(BilinearSampler, TexCoord);
    }
    
    uint userData = instance.userData >> 1;
    
    if (userData == 2)
    {
        float a = color.r;
        clip(color.r - 0.2f);
        color.rgb = 1.0f.rrr;
        color.a = a;
    }
    
    color = color * instance.instanceColor;
	
	clip(noise - 0.5);
	if (noise - 0.5 < 0.08)
		color.rgb = 1.0f.xxx;
	if (noise - 0.5 < 0.04)
		color.rgb = 0.0f.xxx;
	
    return color;
}

#endif