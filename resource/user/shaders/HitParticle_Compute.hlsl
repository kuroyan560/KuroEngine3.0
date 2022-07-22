#include"../..//engine/Camera.hlsli"
#define threadBlockSize 128

struct Particle
{
    float4 color;
    float scale;
    float3 vel;
    float3 pos;
    float pad;
};

struct IndirectCommand
{
    uint64_t cbvAddress[2];
    uint4 drawArguments;
};

cbuffer Camera : register(b0)
{
    Camera cam;
}

cbuffer RootConstants : register(b1)
{
    uint commandCount;
};

StructuredBuffer<Particle> particles : register(t0);
StructuredBuffer<IndirectCommand> inputCommands            : register(t1);
AppendStructuredBuffer<IndirectCommand> outputCommands    : register(u0);

[numthreads(threadBlockSize, 1, 1)]
void CSmain(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint index = (groupId.x * threadBlockSize) + groupIndex;
    
    if (index < commandCount)
    {

        
        outputCommands.Append(inputCommands[index]);
    }
}
