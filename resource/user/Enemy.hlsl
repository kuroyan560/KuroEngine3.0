#include"../Engine/ModelInfo.hlsli"
#include"../Engine/Camera.hlsli"

//ボーン最大数はEnemyManagerと合わせる必要がある
static const int MAX_BONE_NUM = 32;
struct BoneMatricies
{
    matrix mat[MAX_BONE_NUM];
};

cbuffer cbuff0 : register(b0)
{
    Camera cam;
}

StructuredBuffer<matrix> worldMatricies : register(t0);
StructuredBuffer<BoneMatricies> boneMatricies : register(t1);

Texture2D<float4> tex : register(t2);
SamplerState smp : register(s0);

struct VSOutput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VSOutput VSmain(Vertex input, uint instanceID : SV_InstanceID)
{
    float4 resultPos = input.pos;
	
	//ボーン行列
	//bdef4(ボーン4つ又は3つと、それぞれのウェイト値。ウェイト合計が1.0である保障はしない)
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
        
        matrix mat = boneMatricies[instanceID].mat[input.boneNo[0]] * input.weight[0];
        for (int i = 1; i < num; ++i)
        {
            mat += boneMatricies[instanceID].mat[input.boneNo[i]] * input.weight[i];
        }
        resultPos = mul(mat, input.pos);
    }
	//bdef2(ボーン2つと、ボーン1のウェイト値(pmd方式))
    else if (input.boneNo[1] != NO_BONE)
    {
        matrix mat = boneMatricies[instanceID].mat[input.boneNo[0]] * input.weight[0];
        mat += boneMatricies[instanceID].mat[input.boneNo[1]] * (1 - input.weight[0]);
        
        resultPos = mul(mat, input.pos);
    }
	//bdef1(ボーンのみ)
    else if (input.boneNo[0] != NO_BONE)
    {
        resultPos = mul(boneMatricies[instanceID].mat[input.boneNo[0]], input.pos);
    }
	
    VSOutput output;
    output.svpos = mul(cam.proj, mul(cam.view, mul(worldMatricies[instanceID], resultPos)));
    output.uv = input.uv;
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