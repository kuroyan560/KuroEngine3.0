#include"../Camera.hlsli"

cbuffer cbuff0 : register(b0)
{
    Camera cam;
}

cbuffer cbuff1 : register(b1)
{
    matrix world;
    uint hit;
}

struct VSOutput
{
    float4 svpos : SV_Position;
    float4 color : COLOR;
};

static const float ALPHA = 0.5f;

VSOutput VSmain(float4 pos : POSITION)
{
    VSOutput output;
    output.svpos = mul(world, pos); //ワールド変換
    output.svpos = mul(cam.view, output.svpos); //ビュー変換
    output.svpos = mul(cam.proj, output.svpos); //プロジェクション変換
    output.color = lerp(float4(1.0f, 1.0f, 1.0f, ALPHA), float4(1.0f, 0.0f, 0.0f, ALPHA), hit);
	return output;
}

float4 PSmain(VSOutput input) : SV_Target
{
    return input.color;
}

float4 main(float4 pos : POSITION) : SV_POSITION
{
    return pos;
}