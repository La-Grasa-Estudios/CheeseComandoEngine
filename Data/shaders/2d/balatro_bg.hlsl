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

cbuffer FrameData : register(b1)
{
    float4x4 ProjView[2];
	float2 ScreenResolution;
};

cbuffer Custom : register(b2)
{
    float time;
};

#ifdef STAGE_VERTEX

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

#ifdef STAGE_PIXEL

VK_BINDING(0, 2) Texture2D Textures[] : register(t0, space2);
SamplerState BilinearSampler : register(s0);
SamplerState NearestSampler : register(s1);

static const float spin_speed = 7.0f;
static const float4 colour_1 = float4(0.871, 0.267, 0.231, 1.0);
static const float4 colour_2 = float4(0.0, 0.42, 0.706, 1.0);
static const float4 colour_3 = float4(0.086, 0.137, 0.145, 1.0);
static const float contrast = 3.5f;
static const float spin_amount = 0.25f;

#define PIXEL_SIZE_FAC 700.
#define SPIN_EASE 1.0

float4 main(v2f input) : SV_Target
{
    //Convert to UV coords (0-1) and floor for pixel effect
    float pixel_size = length(ScreenResolution)/PIXEL_SIZE_FAC;
    float2 uv = (floor(input.ClipPos.xy*(1./pixel_size))*pixel_size - 0.5*ScreenResolution)/length(ScreenResolution) - float2(0.12, 0.);
    float uv_len = length(uv);
	
	//Adding in a center swirl, changes with time. Only applies meaningfully if the 'spin amount' is a non-zero float
    float speed = (spin_speed*SPIN_EASE*0.2) + 302.2;
    float new_pixel_angle = (atan2(uv.y, uv.x)) + speed - SPIN_EASE*20.*(1.*spin_amount*uv_len + (1. - 1.*spin_amount));
    float2 mid = (ScreenResolution/length(ScreenResolution))/2.;
    uv = (float2((uv_len * cos(new_pixel_angle) + mid.x), (uv_len * sin(new_pixel_angle) + mid.y)) - mid);

	//Now add the paint effect to the swirled UV
    uv *= 30.;
    speed = time*(2.);
	float2 uv2 = float2(uv.x, uv.y);

    for(int i=0; i < 5; i++) {
		uv2 += sin(max(uv.x, uv.y)) + uv;
		uv  += 0.5*float2(cos(5.1123314 + 0.353*uv2.y + speed*0.131121),sin(uv2.x - 0.113*speed));
		uv  -= 1.0*cos(uv.x + uv.y) - 1.0*sin(uv.x*0.711 - uv.y);
	}

    //Make the paint amount range from 0 - 2
    float contrast_mod = (0.25*contrast + 0.5*spin_amount + 1.2);
	float paint_res =min(2., max(0.,length(uv)*(0.035)*contrast_mod));
    float c1p = max(0.,1. - contrast_mod*abs(1.-paint_res));
    float c2p = max(0.,1. - contrast_mod*abs(paint_res));
    float c3p = 1. - min(1., c1p + c2p);

    float4 ret_col = (0.3/contrast)*colour_1 + (1. - 0.3/contrast)*(colour_1*c1p + colour_2*c2p + float4(c3p*colour_3.rgb, c3p*colour_1.a));
	
	SpriteInstance instance = Instances[input.instanceID];
	float4 color = 0.0f.xxxx;
	
	if (instance.texture != -1)
    {
        float2 TexCoord = input.TexCoord;
        
        bool useNearest = (instance.flags & FLAG_NEAREST) != 0;
        
        if (useNearest)
            color = Textures[NonUniformResourceIndex(instance.texture)].Sample(NearestSampler, TexCoord);
        else
            color = Textures[NonUniformResourceIndex(instance.texture)].Sample(BilinearSampler, TexCoord);
    }
	
    return lerp(ret_col, color, 0.01f);
}

#endif