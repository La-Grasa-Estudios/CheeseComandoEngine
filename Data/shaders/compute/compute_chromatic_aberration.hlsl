#define LOCAL_SIZE 16

cbuffer Params : register(b0)
{
    uint2 ScreenResolution;
};

Texture2D InputTexture : register(t0);
RWTexture2D<float4> OutputTexture : register(u0);

[numthreads(LOCAL_SIZE, LOCAL_SIZE, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    if (dispatchID.x > ScreenResolution.x || dispatchID.y > ScreenResolution.y)
        return;
    
    float redOffset = 0.009;
    float greenOffset = 0.006;
    float blueOffset = -0.006;
    
    float2 texCoord = dispatchID.xy / float2(ScreenResolution.xy);
    float2 direction = texCoord - 0.5f.xx;
    
    int2 posR = clamp((texCoord + (direction * redOffset)) * ScreenResolution, 0, ScreenResolution);
    int2 posG = clamp((texCoord + (direction * greenOffset)) * ScreenResolution, 0, ScreenResolution);
    int2 posB = clamp((texCoord + (direction * blueOffset)) * ScreenResolution, 0, ScreenResolution);
    
    float r = InputTexture[posR].r;
    float g = InputTexture[posG].g;
    float2 ba = InputTexture[posB].ba;
    
    OutputTexture[dispatchID.xy] = float4(r, g, ba.x, ba.y);
}