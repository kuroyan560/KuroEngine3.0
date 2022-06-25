#include"ModelInfo.hlsli"
#include"Camera.hlsli"

cbuffer cbuff0 : register(b0)
{
    Camera lightCamera;
}

cbuffer cbuff1 : register(b1)
{
    matrix world;
}

cbuffer cbuff2 : register(b2)
{
    matrix bones[256]; //ボーン行列
}

struct VSOutput
{
    float4 svpos : SV_POSITION;
    float2 depth : TEXCOORD1;
};

VSOutput VSmain(Vertex input)
{
    float4 resultPos = input.pos;
	
	//ボーン行列
	//BDEF4(ボーン4つ又は3つと、それぞれのウェイト値。ウェイト合計が1.0である保障はしない)
    if (input.boneNo[2] != NO_BONE)
    {
        int num;
        
        if (input.boneNo[3] != NO_BONE)    //ボーン４つ
        {
            num = 4;
        }
        else //ボーン３つ
        {
            num = 3;
        }
        
        matrix mat = bones[input.boneNo[0]] * input.weight[0];
        for (int i = 1; i < num; ++i)
        {
            mat += bones[input.boneNo[i]] * input.weight[i];
        }
        resultPos = mul(mat, input.pos);
    }
	//BDEF2(ボーン2つと、ボーン1のウェイト値(PMD方式))
    else if (input.boneNo[1] != NO_BONE)
    {
        matrix mat = bones[input.boneNo[0]] * input.weight[0];
        mat += bones[input.boneNo[1]] * (1 - input.weight[0]);
        
        resultPos = mul(mat, input.pos);
    }
	//BDEF1(ボーンのみ)
    else if (input.boneNo[0] != NO_BONE)
    {
        resultPos = mul(bones[input.boneNo[0]], input.pos);
    }
	
    VSOutput output;
    float4 worldPos = mul(world, resultPos);
    output.svpos = mul(lightCamera.proj, mul(lightCamera.view, worldPos));
    
    //ライトから見た深度値（深度値最大1000) とその２乗
    output.depth.x = length(worldPos.xyz - lightCamera.eyePos) / 1000.0f;
    output.depth.y = output.depth.x * output.depth.x;
    return output;
}


float4 PSmain(VSOutput input) : SV_TARGET
{
    return float4(input.depth.x, input.depth.y, 0.0f, 1.0f);
}

float4 main(float4 pos : POSITION) : SV_POSITION
{
    return pos;
}