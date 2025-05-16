struct v2f
{
	float4 ClipPos : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

struct i2v
{
    float2 Position : TEXCOORD0;
};

cbuffer DrawData : register(b0)
{
    float4x4 Transform;
    float4 uv1;
    float4 uv2;
    int texture;
};

#ifdef STAGE_VERTEX

cbuffer FrameData : register(b1)
{
    float4x4 ProjView;
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
    float4 uv = uvLut[vertexID] == 0 ? uv1 : uv2;
    
	v2f output;
    output.ClipPos = mul(ProjView, mul(Transform, float4(float3(input.Position, 0.0), 1.0)));
    output.TexCoord = swlut[vertexID] == 0 ? uv.xy : uv.zw;
	return output;
}

#endif

#ifdef STAGE_PIXEL

Texture2D Textures[] : register(t0, space2);
SamplerState BilinearSampler : register(s0);

float4 main(v2f input) : SV_Target
{
    float4 color = 1.0.xxxx;
    
    if (texture != -1)
    {
        float2 TexCoord = input.TexCoord;
        color = Textures[texture].Sample(BilinearSampler, TexCoord);
    }
    
    return color;
}

#endif