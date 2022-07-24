#include"../../engine/Camera.hlsli"
#include"../../engine/Math.hlsli"
#define threadBlockSize 128

struct Particle
{
    float4 m_color;
    float3 m_vel;
    float m_life;
    float3 m_pos;
	int m_lifeSpan;
};

struct IndirectCommand
{
    uint64_t m_cbvAddress[2];
    uint4 m_drawArguments;
};

cbuffer RootConstants : register(b0)
{
    //uint64_t cameraCbvAddress;
    uint maxCommandCount;
};

RWStructuredBuffer<Particle> particles : register(u0);
StructuredBuffer<IndirectCommand> inputCommands            : register(t0);
AppendStructuredBuffer<IndirectCommand> outputCommands    : register(u1);

static const float s_minSpeed = 0.01f;
static const float s_maxSpeed = 2.0f;

[numthreads(threadBlockSize, 1, 1)]
void CSmain(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint index = (groupId.x * threadBlockSize) + groupIndex;
    
    //範囲外
    if (maxCommandCount <= index)
        return;
    
    //パーティクル取得
    Particle pt = particles[index];
    
    //生存してない
    if (pt.m_life <= 0)
        return;
    
    //パーティクルの更新
    pt.m_life -= 1.0f / (float) pt.m_lifeSpan;
    
    float3 vec = normalize(pt.m_vel);
    pt.m_vel = vec * Easing_Exp_Out(1.0f - pt.m_life, 1.0f, s_maxSpeed, s_minSpeed);
    
    pt.m_pos += pt.m_vel;
    particles[index] = pt;
    
    //生存しているパーティクルのみAppend
    IndirectCommand appearCommand = inputCommands[index];
    //appearCommand.m_cbvAddress[0] = cameraCbvAddress;
    outputCommands.Append(appearCommand);
}
