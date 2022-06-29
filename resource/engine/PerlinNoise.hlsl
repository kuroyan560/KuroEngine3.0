cbuffer cbuff0 : register(b0)
{
    float2 rectLength;
    int split;
}
StructuredBuffer<float2> grads : register(t0);
RWTexture2D<float4> pixels : register(u0);  

float EaseCurve(float t)
{
    return 6 * pow(t, 5) - 15 * pow(t, 4) + 10 * pow(t, 3);
}

[numthreads(1, 1, 1)]
void CSmain( uint2 DTid : SV_DispatchThreadID )
{
    //自身が所属する矩形の各角のインデックス取得
    int x0Idx = DTid.x / split;
    int x1Idx = x0Idx + 1;
    int y0Idx = DTid.y / split;
    int y1Idx = y0Idx + 1;
    
    //各角の勾配ベクトル取得
    float2 grad_LU = grads[y0Idx][x0Idx];
    float2 grad_LB = grads[y1Idx][x0Idx];
    float2 grad_RU = grads[y0Idx][x1Idx];
    float2 grad_RB = grads[y1Idx][x1Idx];
    
    //自身が所属する矩形の各角の座標取得
    int x0 = x0Idx * rectLength;
    int x1 = x1Idx * rectLength;
    int y0 = y0Idx * rectLength;
    int y1 = y1Idx * rectLength;
    
    //距離ベクトル計算
    float2 vec_LU = DTid - float2(x0, y0);
    float2 vec_LB = DTid - float2(x0, y1);
    float2 vec_RU = DTid - float2(x1, y0);
    float2 vec_RB = DTid - float2(x1, y1);
    
    //影響度
    float affect_LU = dot(grad_LU, vec_LU);
    float affect_LB = dot(grad_LB, vec_LB);
    float affect_RU = dot(grad_RU, vec_RU);
    float affect_RB = dot(grad_RB, vec_RB);
    
    float2 uv = normalize(vec_LU);
    uv.x = EaseCurve(uv.x);
    uv.y = EaseCurve(uv.y);
    float u1 = lerp(affect_LB, affect_LU, uv.x);
    float u2 = lerp(affect_RU, affect_RB, uv.x);
    
    float average = lerp(u1, u2, uv.y);
    pixels[DTid] = float4(average, average, average, 1.0f);
}