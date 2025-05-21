#ifndef __cplusplus
static const uint LIGHT_SPOT = 0;
static const uint LIGHT_POINT = 1;
static const uint LIGHT_DIRECTIONAL = 2;
static const uint LIGHT_AREA = 3;

static const uint TEXTURES_BIND_SPACE = 2;
#else
typedef uint32_t uint;
#endif

//#define XBOX

struct Light
{
    float3 Orientation;
    float3 Position;
    float3 Color;
    float Range;
    uint Type;
};

float4 Unpack4x8UNorm(uint packedValue)
{
    float4 unpacked;
#ifndef XBOX
    unpacked.x = float((packedValue >> 0) & 0xFF) / 255.0;
    unpacked.y = float((packedValue >> 8) & 0xFF) / 255.0;
    unpacked.z = float((packedValue >> 16) & 0xFF) / 255.0;
    unpacked.w = float((packedValue >> 24) & 0xFF) / 255.0;
#else
    unpacked.x = __XB_UnpackByte0(packedValue);
    unpacked.y = __XB_UnpackByte1(packedValue);
    unpacked.z = __XB_UnpackByte2(packedValue);
    unpacked.w = __XB_UnpackByte3(packedValue);
#endif
    return unpacked;
}

float3 Unpack3x8UNorm(uint packedValue)
{
    return Unpack4x8UNorm(packedValue).rgb;
}

float4 UnpackSNorm4x8(uint packedValue)
{
    int SignedValue = int(packedValue);
    int4 Packed = int4(SignedValue << 24, SignedValue << 16, SignedValue << 8, SignedValue) >> 24;
    return clamp(float4(Packed) / 127.0, -1.0, 1.0);
}

float2 UnpackFloat2From16(uint packedValue)
{
    return float2(
        f16tof32(packedValue & 0xFFFF),
        f16tof32(packedValue >> 16)
    );
}