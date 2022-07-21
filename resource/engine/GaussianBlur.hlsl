// 定数バッファー
cbuffer cb_0 : register(b0)
{
    float4 weights[2];
};

// テクスチャ情報用の定数バッファー
cbuffer texInfoCB : register(b1)
{
    int2 texSize; // オリジナルテクスチャのサイズ
    int2 xBlurTexSize; // Xブラーの出力先のテクスチャのサイズ
    int2 yBlurTexSize; // Yブラーの出力先のテクスチャのサイズ
};

//入出力画像にアクセスするための変数を定義
Texture2D<float4> inputImage : register(t0);
RWTexture2D<float4> outputImage : register(u0);

float4 GetPixelColor(int x, int y, int2 texSize)
{
    x = clamp(0, texSize.x, x);
    y = clamp(0, texSize.y, y);

    return inputImage[uint2(x, y)];
}

static const int THREAD_DIV = 4;

//Xブラーを実装
[numthreads(THREAD_DIV, THREAD_DIV, 1)]
void XBlur(uint3 DTid : SV_DispatchThreadID)
{
    uint2 basepos = uint2(DTid.x * 2, DTid.y);

    float4 color = GetPixelColor(basepos.x, basepos.y, texSize) * weights[0].x;
    color += GetPixelColor(basepos.x + 1, basepos.y, texSize) * weights[0].y;
    color += GetPixelColor(basepos.x + 2, basepos.y, texSize) * weights[0].z;
    color += GetPixelColor(basepos.x + 3, basepos.y, texSize) * weights[0].w;
    color += GetPixelColor(basepos.x + 4, basepos.y, texSize) * weights[1].x;
    color += GetPixelColor(basepos.x + 5, basepos.y, texSize) * weights[1].y;
    color += GetPixelColor(basepos.x + 6, basepos.y, texSize) * weights[1].z;
    color += GetPixelColor(basepos.x + 7, basepos.y, texSize) * weights[1].w;

    color += GetPixelColor(basepos.x - 1, basepos.y, texSize) * weights[0].y;
    color += GetPixelColor(basepos.x - 2, basepos.y, texSize) * weights[0].z;
    color += GetPixelColor(basepos.x - 3, basepos.y, texSize) * weights[0].w;
    color += GetPixelColor(basepos.x - 4, basepos.y, texSize) * weights[1].x;
    color += GetPixelColor(basepos.x - 5, basepos.y, texSize) * weights[1].y;
    color += GetPixelColor(basepos.x - 6, basepos.y, texSize) * weights[1].z;
    color += GetPixelColor(basepos.x - 7, basepos.y, texSize) * weights[1].w;

    outputImage[DTid.xy] = color;
}

[numthreads(THREAD_DIV, THREAD_DIV, 1)]
void YBlur(uint3 DTid : SV_DispatchThreadID)
{
    uint2 basepos = uint2(DTid.x, DTid.y * 2);

    float4 color = GetPixelColor(basepos.x, basepos.y, xBlurTexSize) * weights[0].x;
    color += GetPixelColor(basepos.x, basepos.y + 1, xBlurTexSize) * weights[0].y;
    color += GetPixelColor(basepos.x, basepos.y + 2, xBlurTexSize) * weights[0].z;
    color += GetPixelColor(basepos.x, basepos.y + 3, xBlurTexSize) * weights[0].w;
    color += GetPixelColor(basepos.x, basepos.y + 4, xBlurTexSize) * weights[1].x;
    color += GetPixelColor(basepos.x, basepos.y + 5, xBlurTexSize) * weights[1].y;
    color += GetPixelColor(basepos.x, basepos.y + 6, xBlurTexSize) * weights[1].z;
    color += GetPixelColor(basepos.x, basepos.y + 7, xBlurTexSize) * weights[1].w;

    color += GetPixelColor(basepos.x, basepos.y - 1, xBlurTexSize) * weights[0].y;
    color += GetPixelColor(basepos.x, basepos.y - 2, xBlurTexSize) * weights[0].z;
    color += GetPixelColor(basepos.x, basepos.y - 3, xBlurTexSize) * weights[0].w;
    color += GetPixelColor(basepos.x, basepos.y - 4, xBlurTexSize) * weights[1].x;
    color += GetPixelColor(basepos.x, basepos.y - 5, xBlurTexSize) * weights[1].y;
    color += GetPixelColor(basepos.x, basepos.y - 6, xBlurTexSize) * weights[1].z;
    color += GetPixelColor(basepos.x, basepos.y - 7, xBlurTexSize) * weights[1].w;

    outputImage[DTid.xy] = color;
}

[numthreads(THREAD_DIV, THREAD_DIV, 1)]
void Final(uint3 DTid : SV_DispatchThreadID)
{
    // バイリニアフィルタをかける
    uint2 basepos = uint2(DTid.x / 2, DTid.y / 2);
    float4 color = GetPixelColor(basepos.x, basepos.y, yBlurTexSize);
    color += GetPixelColor(basepos.x, basepos.y + 1, yBlurTexSize);
    color += GetPixelColor(basepos.x + 1, basepos.y, yBlurTexSize);
    color += GetPixelColor(basepos.x + 1, basepos.y + 1, yBlurTexSize);

    // 加重平均を取る
    color /= 4.0f;
    outputImage[DTid.xy] = color;
}