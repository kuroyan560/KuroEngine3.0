
cbuffer cbuff0 : register(b0)
{
    int2 imgNum;
    int2 splitSize;
};

Texture2D<float4>sourceImg : register(t0);
RWTexture2D<float4> splitPixels : register(u0);

[numthreads(8, 8, 1)]
void CSmain( uint2 DTid : SV_DispatchThreadID )
{
    uint2 offset = imgNum * splitSize + DTid;
    splitPixels[DTid] = sourceImg[offset];
}