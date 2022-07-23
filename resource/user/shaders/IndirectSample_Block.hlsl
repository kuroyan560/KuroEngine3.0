#include"../../engine/Camera.hlsli"

//cbuffer cbuff0 : register(b0)
//{
//    matrix proj;
//    matrix view;
//    float4 color;
//    float scale;
//    float3 vel;
//    float3 offset;
//}

cbuffer cbuff0 : register(b0)
{
    Camera cam;
}

cbuffer cbuff1 : register(b1)
{
    float4 color;
    float scale;
    float3 vel;
    float3 pos;
};

SamplerState smp : register(s0);

float4 VSmain(float4 pos : POSITION) : POSITION
{
	return pos;
}

static const float PI = 3.14159265359f;
static const uint vertNum = 36;

//CUBE ====================================
static const int3 cubeOffset[8] =
{
    { -1, -1, -1 },
    { 1, -1, -1 },
    { -1, 1, -1 },
    { 1, 1, -1 },
    { -1, -1, 1 },
    { 1, -1, 1 },
    { -1, 1, 1 },
    { 1, 1, 1 }
};
static const int cubeOffsetIdx[vertNum] =
{
    0, 2, 1, //手前１
    1, 3, 2, //手前2
    1, 3, 5, //右1
    5, 7, 3, //右２
    5, 7, 4, //奥1
    4, 6, 7, //奥2
    4, 6, 0, //左1
    0, 2, 6, //左2
    2, 6, 3, //上1
    3, 7, 6, //上2
    0, 1, 5, //下1
    0, 4, 5, //下2
};
static const float3 cubeNormal[6] =
{
    { 0, 0, -1 },
    { 1, 0, 0 },
    { 0, 0, 1 },
    { -1, 0, 0 },
    { 0, 1, 0 },
    { 0, -1, 0 }
};

//==============================

//回転行列を求める
matrix GetRoateMat(float3 Angle)
{
    //ラジアンに直す
    Angle.x = PI / 180 * Angle.x;
    Angle.y = PI / 180 * Angle.y;
    Angle.z = PI / 180 * Angle.z;
    
    matrix matX =
    {
        1, 0, 0, 0,
        0, cos(Angle.x), -sin(Angle.x), 0,
        0, sin(Angle.x), cos(Angle.x), 0,
        0, 0, 0, 1
    };
    matrix matY =
    {
        cos(Angle.y), -sin(Angle.y), 0, 0,
        sin(Angle.y), cos(Angle.y), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    matrix matZ =
    {
        cos(Angle.z), 0, sin(Angle.z), 0,
        0, 1, 0, 0,
        -sin(Angle.z), 0, cos(Angle.z), 0,
        0, 0, 0, 1
    };

    return mul(mul(matX, matY), matZ);
}

struct GSOutput
{
    float4 svpos : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float depthInView : CAM_Z;
};

[maxvertexcount(64)]
void GSmain(
	point float4 input[1] : POSITION, //１点からプリミティブ生成
	inout TriangleStream<GSOutput> output
)
{
    GSOutput element;
    //カラーはそのまま渡す
    element.color = color;
    
    float4 center = input[0]; //中心
    center.xyz += pos;
    //matrix rotateMat = GetRoateMat(input[0].rotate); //回転角度
    
    //形状によって頂点数が変わる
    for (uint i = 0; i < vertNum; ++i)
    {
        //CUBE
        float side_h = scale / 2.0f;
        int index = cubeOffsetIdx[i];
        float4 pos = center;
        pos.x += cubeOffset[index].x * side_h;
        pos.y += cubeOffset[index].y * side_h;
        pos.z += cubeOffset[index].z * side_h;
            
        //pos = center + mul(rotateMat, pos - center);
            
        index = i / 6;
        element.normal = cubeNormal[index];
        //element.ray = normalize(pos.xyz - cameraPos);
            
        pos = mul(cam.view, pos); //ビュー変換
        element.depthInView = pos.z;
        pos = mul(cam.proj, pos);   //プロジェクション変換
        element.svpos = pos;
        
        output.Append(element);
    }
}

struct PSOutput
{
    float4 color : SV_Target0;
    float4 emissive : SV_Target1;
    float depth : SV_Target2;
};

PSOutput PSmain(GSOutput input)
{
    //光の向かうベクトル（平行光線）
    float3 light = normalize(float3(1, -1, 1));
	//ライトのカラー（白）
    float3 lightColor = float3(1, 1, 1);
    float4 result = input.color;
    
    float p = dot(input.normal * -1.0f, light.xyz);
    p = p * 0.5f + 0.5f;
    p = p * p;
    
    float3 toon;
    if (p < 0.2f)
    {
        toon = (0.7f, 0.7f, 0.7f);
    }
    else if (p < 0.5f)
    {
        toon = (0.8f, 0.8f, 0.8f);
    }
    else
    {
        toon = (1.0f, 1.0f, 1.0f);
    }
    
    result.xyz *= toon.xyz;
    
    PSOutput output;
    output.color = result;
    output.emissive = result;
    output.depth = input.depthInView;
    return output;
}

float4 main(float4 pos : POSITION) : SV_POSITION
{
    return pos;
}