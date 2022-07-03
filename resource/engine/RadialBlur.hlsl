// 定数バッファー
cbuffer cb_0 : register(b0)
{
    int2 texSize;
    float blurPower;
    float2 centerUV;
};

//入出力画像にアクセスするための変数を定義
Texture2D<float4> inputImage : register(t0);
RWTexture2D<float4> outputImage : register(u0);

float4 GetPixelColor(int2 pos, int2 texSize)
{
    pos.x = clamp(0, texSize.x, pos.x);
    pos.y = clamp(0, texSize.y, pos.y);

    return inputImage[uint2(pos.x, pos.y)];
}

[numthreads(4, 4, 1)]
void CSmain( uint3 DTid : SV_DispatchThreadID )
{
    uint2 basepos = uint2(DTid.x, DTid.y);
    float2 dir = centerUV - basepos / texSize;
    float2 offset = dir * blurPower;

    float4 color = GetPixelColor(basepos                    , texSize) * 0.19f;
    color += GetPixelColor(basepos + offset         , texSize) * 0.17f;
    color += GetPixelColor(basepos + offset * 2.0f, texSize) * 0.15f;
    color += GetPixelColor(basepos + offset * 3.0f, texSize) * 0.13f;
    color += GetPixelColor(basepos + offset * 4.0f, texSize) * 0.11f;
    color += GetPixelColor(basepos + offset * 5.0f, texSize) * 0.09f;
    color += GetPixelColor(basepos + offset * 6.0f, texSize) * 0.07f;
    color += GetPixelColor(basepos + offset * 7.0f, texSize) * 0.05f;
    color += GetPixelColor(basepos + offset * 8.0f, texSize) * 0.03f;
    color += GetPixelColor(basepos + offset * 9.0f, texSize) * 0.01f;

    outputImage[DTid.xy] = color;
}