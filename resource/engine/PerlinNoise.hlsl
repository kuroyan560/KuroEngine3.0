cbuffer cbuff0 : register(b0)
{
    float2 rectLength;
    int split;
}
StructuredBuffer<float2> grads : register(t0);
RWTexture2D<float4> pixels : register(u0);  

float Wavelet(float t)
{
    return 1 - 3 * pow(t, 2) + 2 * pow(abs(t), 3);
}

//勾配を設定したウェーブレット関数
float GradWaveLet(float2 uv, float2 grad)
{
    float l = dot(uv, grad);
    float c = Wavelet(uv.x) * Wavelet(uv.y);
    return c * l;
}

[numthreads(1, 1, 1)]
void CSmain( uint2 DTid : SV_DispatchThreadID )
{
    //自身が所属する矩形の各角のインデックス取得
    int x0Idx = DTid.x / rectLength;
    int x1Idx = x0Idx + 1;
    int y0Idx = DTid.y / rectLength;
    int y1Idx = y0Idx + 1;
    
    //各角の勾配ベクトル取得
    float2 grad_LU = grads[y0Idx][x0Idx];
    float2 grad_LB = grads[y1Idx][x0Idx];
    float2 grad_RU = grads[y0Idx][x1Idx];
    float2 grad_RB = grads[y1Idx][x1Idx];
    
    //自身が所属する矩形の各角の座標取得
    int x0Pos = x0Idx * rectLength;
    int x1Pos = x1Idx * rectLength;
    int y0Pos = y0Idx * rectLength;
    int y1Pos = y1Idx * rectLength;
    
    //自身の相対座標取得
    float2 uv = DTid / (rectLength * split) * 2.0f - 1.0f;  //-1.0f ~ 1.0fの範囲に直す
    
    //左上と右上の対で補間
    float w00 = GradWaveLet(uv, grad_LU);
    float w10 = GradWaveLet(uv, grad_RU);
    float wy0 = lerp(w00, w10, uv.x);
    
    //左下と右下の対で補間
    float w01 = GradWaveLet(uv, grad_LB);
    float w11 = GradWaveLet(uv, grad_RB);
    float wy1 = lerp(w01, w11, uv.x);
    
    //Y軸方向に補間
    float result = lerp(wy0, wy1, uv.y);
    
    pixels[DTid] = float4(result, result, result, 1.0f);
}