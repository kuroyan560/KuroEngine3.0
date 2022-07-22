#include"../../Engine/ModelInfo.hlsli"
#include"../../Engine/Camera.hlsli"
#include"../../Engine/Math.hlsli"

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

cbuffer cbuff1 : register(b1)
{
    Material material;
}
StructuredBuffer<matrix> worldMatricies : register(t0);
StructuredBuffer<BoneMatricies> boneMatricies : register(t1);

TextureCube cubeMap : register(t2);

Texture2D<float4> baseTex : register(t3);
Texture2D<float4> metalnessTex : register(t4);
Texture2D<float4> roughnessTex : register(t5);

SamplerState smp : register(s0);

struct VSOutput
{
    float4 svpos : SV_POSITION;
    float3 worldpos : POSITION;
    float3 normal : NORMAL; // 法線
    float2 uv : TEXCOORD;
    float3 reflect : REFLECT;
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
    float4 wpos = mul(worldMatricies[instanceID], resultPos); //ワールド変換
    output.svpos = mul(cam.view, wpos); //ビュー変換
    output.svpos = mul(cam.proj, output.svpos); //プロジェクション変換
    output.worldpos = wpos;
    output.normal = normalize(mul(worldMatricies[instanceID], input.normal));
    output.uv = input.uv;
    
    //視線ベクトルと法線より反射ベクトルを求める
    output.reflect = normalize(reflect(normalize(wpos.xyz - cam.eyePos), output.normal));
    return output;
}

static float3 s_baseColor;
static float s_metalness;
static float s_roughness;

//Schlickによる近似式
float SchlickFresnel(float f0, float f90, float cosine)
{
    float m = saturate(1 - cosine);
    float m2 = m * m;
    float m5 = m2 * m2 * m;
    return lerp(f0, f90, m5);
}

//UE4のGGX分布
float DistributionGGX(float alpha, float NdotH)
{
    if (!any(alpha))
        return 0.0f;
    float alpha2 = alpha * alpha;
    float t = NdotH * NdotH * (alpha2 - 1.0f) + 1.0f;
    return alpha2 / (PI * t * t);
}

float3 SchlickFresnel3(float3 f0, float3 f90, float cosine)
{
    float m = saturate(1 - cosine);
    float m2 = m * m;
    float m5 = m2 * m2 * m;
    return lerp(f0, f90, m5);
}

float3 DisneyFresnel(float LdotH)
{
    float luminance = 0.3f * s_baseColor.r + 0.6 * s_baseColor.g + 0.1f * s_baseColor.b;
    float3 tintColor = s_baseColor / luminance;
    float3 nonMetalColor = material.specular_pbr * 0.08f * tintColor;
    float3 specularColor = lerp(nonMetalColor, s_baseColor, s_metalness);
    return SchlickFresnel3(specularColor, float3(1, 1, 1), LdotH);
}

//UE4のSmithモデル
float GeometricSmith(float cosine)
{
    float k = (s_roughness + 1.0f);
    k = k * k / 8.0f;
    return cosine / (cosine * (1.0f - k) + k);
}

//鏡面反射の計算
float3 CookTorranceSpecular(float NdotL, float NdotV, float NdotH, float LdotH)
{
    float Ds = DistributionGGX(s_roughness * s_roughness, NdotH);
    float3 Fs = DisneyFresnel(LdotH);
    float Gs = GeometricSmith(NdotL) * GeometricSmith(NdotV);
    float m = 4.0f * NdotL * NdotV;
    return Ds * Fs * Gs / m;
}

//双方向反射分布関数
float3 BRDF(float3 LigDirection, float3 LigColor, float3 WorldNormal, float3 WorldPos, float3 EyePos)
{
    float3 L = -LigDirection;
    float3 V = normalize(EyePos - WorldPos);
    float NdotL = dot(WorldNormal, L);
    float NdotV = dot(WorldNormal, V);
    if (NdotL <= 0 || NdotV <= 0)
    {
        return float3(0, 0, 0);
    }
    
    float3 H = normalize(L + V);
    float NdotH = dot(WorldNormal, H);
    float LdotH = dot(L, H);
    
    float diffuseReflectance = 1.0f / PI;
    
    float energyBias = 0.5f * s_roughness;
    float Fd90 = energyBias + 2.0f * LdotH * LdotH * s_roughness;
    float FL = SchlickFresnel(1.0f, Fd90, NdotL);
    float FV = SchlickFresnel(1.0f, Fd90, NdotV);
    float energyFactor = lerp(1.0f, 1.0f / 1.51f, s_roughness);
    float Fd = FL * FV * energyFactor;
    
    float3 diffuseColor = diffuseReflectance * Fd * s_baseColor * (1 - s_metalness);
    float3 specularColor = CookTorranceSpecular(NdotL, NdotV, NdotH, LdotH) * LigColor;
    
    return (diffuseColor + specularColor);
}


float4 PSmain(VSOutput input) : SV_TARGET
{
    float3 normal = input.normal;
    
    s_baseColor = material.baseColor + baseTex.Sample(smp, input.uv).rgb;
    s_metalness = material.metalness + metalnessTex.Sample(smp, input.uv).r;
    s_roughness = material.roughness + roughnessTex.Sample(smp, input.uv).r;
    
    float4 cubeMapLig = cubeMap.Sample(smp, input.reflect);
    
     //ライトの影響
    float3 ligEffect = { 0.0f, 0.0f, 0.0f };
    ligEffect = BRDF(-input.reflect, cubeMapLig.xyz, normal, input.worldpos, cam.eyePos);
    
    float4 result = float4(ligEffect, 1.0f - material.transparent);
    return result;
}

float4 main(float4 pos : POSITION) : SV_POSITION
{
    return pos;
}