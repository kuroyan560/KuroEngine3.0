cbuffer cbuff0 : register(b0)
{
    matrix parallelProjMat; //平行投影行列
};

struct VSOutput
{
    min16int isAlive : ALIVE;
    float4 center : POSITION;
};

VSOutput VSmain(VSOutput input)
{
    return input;
}

struct GSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D<float4> tex : register(t0);
Texture2D<float4> displacementNoiseTex : register(t1);
Texture2D<float4> alphaNoiseTex : register(t2);
SamplerState smp : register(s0);

[maxvertexcount(4)]
void GSmain(
	point VSOutput input[1],
	inout TriangleStream<GSOutput> output
)
{
    if (!input[0].isAlive)
        return;
    
    uint2 texSize;
    tex.GetDimensions(texSize.x, texSize.y);
    
    float width_h = texSize.x / 2.0f;
    float height_h = texSize.y / 2.0f;
    
    GSOutput element;
        
    //左下
    element.pos = input[0].center;
    element.pos.x -= width_h;
    element.pos.y += height_h;
    element.pos = mul(parallelProjMat, element.pos);
    element.uv = float2(0.0f, 1.0f);
    output.Append(element);
    
    //左上
    element.pos = input[0].center;
    element.pos.x -= width_h;
    element.pos.y -= height_h;
    element.pos = mul(parallelProjMat, element.pos);
    element.uv = float2(0.0f, 0.0f);
    output.Append(element);
    
     //右下
    element.pos = input[0].center;
    element.pos.x += width_h;
    element.pos.y += height_h;
    element.pos = mul(parallelProjMat, element.pos);
    element.uv = float2(1.0f, 1.0f);
    output.Append(element);
    
    //右上
    element.pos = input[0].center;
    element.pos.x += width_h;
    element.pos.y -= height_h;
    element.pos = mul(parallelProjMat, element.pos);
    element.uv = float2(1.0f, 0.0f);
    output.Append(element);
}

float4 PSmain(GSOutput input) : SV_TARGET
{
    float displacementNoise = displacementNoiseTex.Sample(smp, input.uv).r;
    displacementNoise = displacementNoise * 2.0f - 1.0f; //0~1から-1~1の範囲に
    
    //中央から外側に向かって
    float2 vec = normalize(input.uv - float2(0.5f, 0.5f));
    
    //ランダムにずれる
    input.uv += vec * displacementNoise;
    
    //通常のテクスチャ
    float4 result = tex.Sample(smp, input.uv);
    
    //アルファノイズ
    float alphaNoise = alphaNoiseTex.Sample(smp, input.uv).r;
    alphaNoise = alphaNoise * 2.0f - 1.0f; //0~1から-1~1の範囲に
    alphaNoise *= 13.0f; //コントラストを上げる
    //alphaNoise = clamp(alphaNoise * 2.0f - 1.0f, 0.0f, 1.0f); //0~1から-1~1の範囲にしてから、負の値を０にする
    result.w *= alphaNoise;
 
    return result;
}

float4 main(float4 pos : POSITION) : SV_POSITION
{
    return pos;
}