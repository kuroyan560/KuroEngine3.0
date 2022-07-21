cbuffer cbuff0 : register(b0)
{
    float nearPint;
    float farPint;
    float pintLength;
};

Texture2D<float4> blurImg : register(t0);
Texture2D<float> depthMap : register(t1);
RWTexture2D<float4> outputImage : register(u0);

[numthreads(32, 32, 1)]
void CSmain(uint2 DTid : SV_DispatchThreadID)
{
    float depth = depthMap[DTid];
    if (nearPint <= depth && depth <= farPint)
    {
        outputImage[DTid] = float4(0, 0, 0, 0);
        return;
    }
        
    float4 color = blurImg[DTid];
    
     //深度値から不透明度を計算
    if (depth < nearPint)
    {
        float frontLimit = nearPint - pintLength;
        if (frontLimit < 0.0f)
            frontLimit = 0.0f;
        color.a = min(1.0f, (nearPint - depth) / (nearPint - frontLimit));
    }
    else if (farPint < depth)
    {
        float backLimit = farPint + pintLength;
        color.a = min(1.0f, (depth - farPint) / (backLimit - farPint));
    }
    
    outputImage[DTid.xy] = color;
}