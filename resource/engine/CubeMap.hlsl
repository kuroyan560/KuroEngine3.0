#include"Camera.hlsli"

cbuffer cbuff0 : register(b0)
{
    Camera cam;
}

cbuffer cbuff1 : register(b1)
{
    matrix world;
}

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct VSOutput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VSOutput VSmain(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    VSOutput output;
    output.svpos = mul(cam.proj, mul(cam.view, mul(world, pos)));
    output.uv = uv;
    return output;
}


float4 PSmain(VSOutput input) : SV_TARGET
{
    return tex.Sample(smp, input.uv);
}

float4 main(float4 pos : POSITION) : SV_POSITION
{
    return pos;
}