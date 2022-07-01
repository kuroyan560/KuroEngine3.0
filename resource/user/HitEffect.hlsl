cbuffer cbuff0 : register(b0)
{
    matrix parallelProjMat; //平行投影行列
};

struct VSOutput
{
    min16int isAlive : ALIVE;
    float4 center : POSITION;
    float blur : BLUR;
    float scale : SCALE;
    float uvOffset : UV_OFFSET;
    float circleThickness : CIRCLE_THICKNESS;
    float circleRadius : CIRCLE_RADIUS;
};

VSOutput VSmain(VSOutput input)
{
    return input;
}

struct GSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float blur : BLUR;
    float uvOffset : UV_OFFSET;
    float circleThickness : CIRCLE_THICKNESS;
    float circleRadius : CIRCLE_RADIUS;
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
    
    float width_h = (texSize.x * input[0].scale) / 2.0f;
    float height_h = (texSize.y * input[0].scale) / 2.0f;
    //float width_h = texSize.x / 2.0f;
    //float height_h = texSize.y / 2.0f;
    
    GSOutput element;
    element.blur = input[0].blur;
    element.uvOffset = input[0].uvOffset;
        
    element.circleThickness = input[0].circleThickness;
    element.circleRadius = input[0].circleRadius;
    
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

static float _circleThicknes;
static float _circleRadius;

float4 GetCirclePixel(float2 uv)
{
    float dist = length(uv - float2(0.5f, 0.5f));
    float differ = abs(dist - _circleRadius);
    return float4(1, 1, 1, 1) * step(differ, _circleThicknes / 2.0f);
}

float4 GetPixelColor(float2 uv)
{
    float displacementNoise = displacementNoiseTex.Sample(smp, uv).r;
    displacementNoise = displacementNoise * 2.0f - 1.0f; //0~1から-1~1の範囲に
    
    //中央から外側に向かって
    float2 toOutVec = uv - float2(0.5f, 0.5f);
    
    //ランダムにずれる
    uv += normalize(toOutVec) * displacementNoise;
    
    //通常のテクスチャ
    //float4 result = tex.Sample(smp, uv);
    float4 result = GetCirclePixel(uv);
    
    //アルファノイズ
    float alphaNoise = alphaNoiseTex.Sample(smp, uv).r;
    alphaNoise = alphaNoise * 2.0f - 1.0f; //0~1から-1~1の範囲に
    alphaNoise *= 9.0f; //コントラストを上げる
    result.w *= alphaNoise;
    return result;
}

static const float2 VIEW_PORT_OFFSET = (float2(0.5f, 0.5f) / float2(1280.0f, 720.0f));
float4 PSmain(GSOutput input) : SV_TARGET
{

    
    input.uv += float2(0, VIEW_PORT_OFFSET.y);
    float2 toOutVec = input.uv - float2(0.5f, 0.5f);    //中心から外側へ向かうUVベクトル
    float len = length(toOutVec);
    toOutVec = normalize(toOutVec);
    input.uv += -toOutVec * input.uvOffset;
    
    _circleRadius = input.circleRadius + 1.0f * input.uvOffset;
    _circleThicknes = input.circleThickness;
    
    //ブラーのためにいくつかのピクセルをサンプリング
    float2 offset = toOutVec * VIEW_PORT_OFFSET;
    offset *= (len * input.blur);
    float4 result = GetPixelColor(input.uv) * 0.19f;
    result += GetPixelColor(input.uv + offset         ) * 0.17f;
    result += GetPixelColor(input.uv + offset * 2.0f) * 0.15f;
    result += GetPixelColor(input.uv + offset * 3.0f) * 0.13f;
    result += GetPixelColor(input.uv + offset * 4.0f) * 0.11f;
    result += GetPixelColor(input.uv + offset * 5.0f) * 0.09f;
    result += GetPixelColor(input.uv + offset * 6.0f) * 0.07f;
    result += GetPixelColor(input.uv + offset * 7.0f) * 0.05f;
    result += GetPixelColor(input.uv + offset * 8.0f) * 0.04f;
    result += GetPixelColor(input.uv + offset * 9.0f) * 0.01f;
    
    //明るくする
    result *= 2.0f;
    
    //色を青っぽく（アルファ値が高いと青白く）
    result.xyz = lerp(float3(0.33f, 0.1f, 0.73f), float3(0.65f, 0.64f, 0.94f), result.w);
    return result;
}

float4 main(float4 pos : POSITION) : SV_POSITION
{
    return pos;
}