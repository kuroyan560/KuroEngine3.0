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
    int2 myPixelIdx = DTid;
    
    //自身が所属する矩形の各角のインデックス取得
    int x0Idx = myPixelIdx.x / rectLength.x;
    int x1Idx = x0Idx + 1;
    int y0Idx = myPixelIdx.y / rectLength.y;
    int y1Idx = y0Idx + 1;
    
    //各角の勾配ベクトル取得
    float2 grad_LU = grads[y0Idx * (split + 1) + x0Idx];
    float2 grad_LB = grads[y1Idx * (split + 1) + x0Idx];
    float2 grad_RU = grads[y0Idx * (split + 1) + x1Idx];
    float2 grad_RB = grads[y1Idx * (split + 1) + x1Idx];
    
    //自身が所属する矩形の各角の座標取得
    int x0Pos = x0Idx * rectLength.x;
    int x1Pos = x1Idx * rectLength.x;
    int y0Pos = y0Idx * rectLength.y;
    int y1Pos = y1Idx * rectLength.y;
    
    //各角に対しての相対座標
    float2 uv_LU = (myPixelIdx - int2(x0Pos, y0Pos)) / rectLength;
    float2 uv_RU = (myPixelIdx - int2(x1Pos, y0Pos)) / rectLength;
    float2 uv_LB = (myPixelIdx - int2(x0Pos, y1Pos)) / rectLength;
    float2 uv_RB = (myPixelIdx - int2(x1Pos, y1Pos)) / rectLength;
    
    //自身が所属する矩形上での相対座標(左上基準）
    float2 uvOnSplit = uv_LU;
    
    //左上と右上の対で補間
    float w_LU = GradWaveLet(uv_LU, grad_LU);
    float w_RU = GradWaveLet(uv_RU, grad_RU);
    float w_U = lerp(w_LU, w_RU, uvOnSplit.x);
    
    //左下と右下の対で補間
    float w_LB = GradWaveLet(uv_LB, grad_LB);
    float w_RB = GradWaveLet(uv_RB, grad_RB);
    float w_B = lerp(w_LB, w_RB, uvOnSplit.x);
    
    //Y軸方向に補間
    float result = lerp(w_U, w_B, uvOnSplit.y);
    
    pixels[DTid] = float4(result, result, result, 1.0f);
}