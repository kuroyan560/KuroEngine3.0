#include"ModelInfo.hlsli"
#include"Camera.hlsli"
#include"LightInfo.hlsli"
#include"Math.hlsli"

static const int DIMENSION_NUM = 6;
static const float3 EYE_POS = float3(0, 0, 0);

cbuffer cbuff0 : register(b0)
{
    matrix viewProj[DIMENSION_NUM];
}

cbuffer cbuff1 : register(b1)
{
    LightInfo ligNum; //アクティブ中のライトの数の情報
}

StructuredBuffer<DirectionLight> dirLight : register(t0);
StructuredBuffer<PointLight> pointLight : register(t1);
StructuredBuffer<SpotLight> spotLight : register(t2);
StructuredBuffer<HemiSphereLight> hemiSphereLight : register(t3);


cbuffer cbuff2 : register(b2)
{
    matrix world;
}

Texture2D<float4> baseTex : register(t4);
Texture2D<float4> metalnessTex : register(t5);
Texture2D<float4> normalMap : register(t6);
Texture2D<float4> roughnessTex : register(t7);
SamplerState smp : register(s0);

static float3 s_baseColor;
static float s_metalness;
static float s_roughness;

cbuffer cbuff3 : register(b3)
{
    Material material;
}

struct VSOutput
{
    float4 worldpos : POSITION;
    float3 normal : NORMAL; // 法線
    float3 tangent : TANGENT;
    float3 biNormal : BINORMAL;
    float2 uv : TEXCOORD;
    float3 reflect : REFLECT;
};

VSOutput VSmain(Vertex input)
{
    float4 resultPos = input.pos;
	
	////ボーン行列
	////BDEF4(ボーン4つ又は3つと、それぞれのウェイト値。ウェイト合計が1.0である保障はしない)
 //   if (input.boneNo[2] != NO_BONE)
 //   {
 //       int num;
        
 //       if (input.boneNo[3] != NO_BONE)    //ボーン４つ
 //       {
 //           num = 4;
 //       }
 //       else //ボーン３つ
 //       {
 //           num = 3;
 //       }
        
 //       matrix mat = bones[input.boneNo[0]] * input.weight[0];
 //       for (int i = 1; i < num; ++i)
 //       {
 //           mat += bones[input.boneNo[i]] * input.weight[i];
 //       }
 //       resultPos = mul(mat, input.pos);
 //   }
	////BDEF2(ボーン2つと、ボーン1のウェイト値(PMD方式))
 //   else if (input.boneNo[1] != NONE)
 //   {
 //       matrix mat = bones[input.boneNo[0]] * input.weight[0];
 //       mat += bones[input.boneNo[1]] * (1 - input.weight[0]);
        
 //       resultPos = mul(mat, input.pos);
 //   }
	////BDEF1(ボーンのみ)
 //   else if (input.boneNo[0] != NONE)
 //   {
 //       resultPos = mul(bones[input.boneNo[0]], input.pos);
 //   }
	
    VSOutput output;
    float4 wpos = mul(world, resultPos); //ワールド変換
    output.worldpos = wpos;
    output.normal = normalize(mul(world, input.normal));
    output.tangent = normalize(mul(world, input.tangent));
    output.biNormal = normalize(mul(world, input.binormal));
    output.uv = input.uv;
    
    //視線ベクトルと法線より反射ベクトルを求める
    output.reflect = reflect(normalize(wpos.xyz - EYE_POS), output.normal);
    return output;
}

struct GSOutput
{
    float4 svpos : SV_POSITION;
    float3 worldpos : POSITION;
    float3 normal : NORMAL; // 法線
    float3 tangent : TANGENT;
    float3 biNormal : BINORMAL;
    float2 uv : TEXCOORD;
    float3 reflect : REFLECT;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(3 * DIMENSION_NUM)]
void GSmain(triangle VSOutput input[3],inout TriangleStream<GSOutput>output)
{
    for (int f = 0; f < DIMENSION_NUM;++f)
    {
        GSOutput element;
        matrix camViewProj = viewProj[f];
        element.RTIndex = f;
        
        for (int i = 0; i < 3;++i)
        {
            element.svpos = mul(camViewProj, input[i].worldpos);
            element.worldpos = input[i].worldpos;
            element.normal = input[i].normal;
            element.tangent = input[i].tangent;
            element.biNormal = input[i].biNormal;
            element.uv = input[i].uv;
            element.reflect = input[i].reflect;
            output.Append(element);
        }
        output.RestartStrip();
    }

}

struct PSOutput
{
    float4 color : SV_Target0;
    //float4 emissive : SV_Target1;
};

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
    if (NdotL < 0 || NdotV < 0)
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
    float3 specularColor = CookTorranceSpecular(NdotL, NdotV, NdotH, LdotH);
    
    return diffuseColor + specularColor;
}

PSOutput PSmain(GSOutput input) : SV_TARGET
{
    float3 normal = input.normal;
    
    s_baseColor = material.baseColor + baseTex.Sample(smp, input.uv).rgb;
    s_metalness = material.metalness + metalnessTex.Sample(smp, input.uv).r;
    s_roughness = material.roughness + roughnessTex.Sample(smp, input.uv).r;
    
     //ライトの影響
    float3 ligEffect = { 0.0f, 0.0f, 0.0f };
    
    //ディレクションライト
    for (int i = 0; i < ligNum.dirLigNum; ++i)
    {
        if (!dirLight[i].active)
            continue;
        
        float3 dir = dirLight[i].direction;
        float3 ligCol = dirLight[i].color.xyz * dirLight[i].color.w;
        ligEffect += BRDF(dir, ligCol, normal, input.worldpos, EYE_POS);
    }
    //ポイントライト
    for (int i = 0; i < ligNum.ptLigNum; ++i)
    {
        if (!pointLight[i].active)
            continue;
        
        float3 dir = input.worldpos - pointLight[i].pos;
        dir = normalize(dir);
        float3 ligCol = pointLight[i].color.xyz * pointLight[i].color.w;
        
        //距離による減衰
        float3 distance = length(input.worldpos - pointLight[i].pos);
		//影響率は距離に比例して小さくなっていく
        float affect = 1.0f - 1.0f / pointLight[i].influenceRange * distance;
		//影響力がマイナスにならないように補正をかける
        if (affect < 0.0f)
            affect = 0.0f;
		//影響を指数関数的にする
        affect = pow(affect, 3.0f);
        
        ligEffect += BRDF(dir, ligCol, normal, input.worldpos, EYE_POS) * affect;
    }
    //スポットライト
    for (int i = 0; i < ligNum.spotLigNum; ++i)
    {
        if (!spotLight[i].active)
            continue;
        
        float3 ligDir = input.worldpos - spotLight[i].pos;
        ligDir = normalize(ligDir);
        float3 ligCol = spotLight[i].color.xyz * spotLight[i].color.w;

        //スポットライトとの距離を計算
        float3 distance = length(input.worldpos - spotLight[i].pos);
       	//影響率は距離に比例して小さくなっていく
        float affect = 1.0f - 1.0f / spotLight[i].influenceRange * distance;
        //影響力がマイナスにならないように補正をかける
        if (affect < 0.0f)
            affect = 0.0f;
    //影響を指数関数的にする
        affect = pow(affect, 3.0f);
    
        float3 dir = normalize(spotLight[i].target - spotLight[i].pos);
        float angle = dot(ligDir, dir);
        angle = abs(acos(angle));
        affect = 1.0f - 1.0f / spotLight[i].angle * angle;
        if (affect < 0.0f)
            affect = 0.0f;
        affect = pow(affect, 0.5f);
        
        ligEffect += BRDF(dir, ligCol, normal, input.worldpos, EYE_POS) * affect;
    }
    //天球
    for (int i = 0; i < ligNum.hemiSphereNum; ++i)
    {
        if (!hemiSphereLight[i].active)
            continue;
        
        float t = dot(normal.xyz, hemiSphereLight[i].groundNormal);
        t = (t + 1.0f) / 2.0f;
        float3 hemiLight = lerp(hemiSphereLight[i].groundColor, hemiSphereLight[i].skyColor, t);
        ligEffect *= hemiLight;
    }
    
    float4 result = float4(ligEffect, 1.0f - material.transparent);
    
    PSOutput output;
    output.color = result;
    //output.emissive = emissiveMap.Sample(smp, input.uv);
    
    ////明るさ計算
    //float bright = dot(result.xyz, float3(0.2125f, 0.7154f, 0.0721f));
    //if (1.0f < bright)
    //    output.emissive += result;
    //output.emissive.w = result.w;
    
    return output;
}

float4 main(float4 pos : POSITION) : SV_POSITION
{
    return pos;
}