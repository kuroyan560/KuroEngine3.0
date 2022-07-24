#include"../..//engine/Camera.hlsli"
#define threadBlockSize 128

struct Particle
{
    float4 m_color;
    float m_scale;
    float3 m_vel;
    float3 m_pos;
    int m_life;
    int m_lifeSpan;
    int pad[3];
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
    if (!pt.m_life)
        return;
    
    pt.m_life--;
    pt.m_pos += pt.m_vel;
    
    //カメラのGPUアドレスをアタッチして Append
    IndirectCommand appearCommand = inputCommands[index];
    //appearCommand.m_cbvAddress[0] = cameraCbvAddress;
    outputCommands.Append(appearCommand);
}
