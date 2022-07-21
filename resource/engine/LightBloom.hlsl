cbuffer cbuff0 : register(b0)
{
    float3 outputColor;
    float brightThreshold;
}

Texture2D<float4> emissiveMap : register(t0);
RWTexture2D<float4> pixels : register(u0);

[numthreads(32, 32, 1)]
void CSmain( uint2 DTid : SV_DispatchThreadID )
{
    float4 col = emissiveMap[DTid];
    float bright = dot(col.xyz, float3(0.2125f, 0.7154f, 0.0721f)); //明るさ計算
    col.w *= step(brightThreshold, bright);
    
    //出力カラー割合乗算
    col.xyz *= outputColor;
    
    pixels[DTid] = col;
}