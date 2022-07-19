#include"../engine/Camera.hlsli"
#define threadBlockSize 128

struct Block
{
    float4 color;
    float scale;
    float3 vel;
    float3 offset;
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
    float cullOffset;    // The culling plane offset in homogenous space.
    uint commandCount;    // The number of commands to be processed.
};

StructuredBuffer<Block> block                : register(t0);    // SRV: Wrapped constant buffers
StructuredBuffer<IndirectCommand> inputCommands            : register(t1);    // SRV: Indirect commands
AppendStructuredBuffer<IndirectCommand> outputCommands    : register(u0);    // UAV: Processed indirect commands

[numthreads(threadBlockSize, 1, 1)]
void CSmain(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint index = (groupId.x * threadBlockSize) + groupIndex;
    
    if (index < commandCount)
    {
        float side_h = block[index].scale / 2.0f;
        
        float4 left = float4(block[index].offset, 1);
        left.x -= side_h;
        left = mul(cam.proj, mul(cam.view, left));
        if (cullOffset < left.x)return;   //‰f‚Á‚Ä‚¢‚È‚¢
        
        float4 right = float4(block[index].offset, 1);
        right.x += side_h;
        right = mul(cam.proj, mul(cam.view, right));
        if (right.x < -cullOffset)return;   //‰f‚Á‚Ä‚¢‚È‚¢
        
        float4 top = float4(block[index].offset, 1);
        top.y += side_h;
        top = mul(cam.proj, mul(cam.view, top));
        if (cullOffset < top.y)return;   //‰f‚Á‚Ä‚¢‚È‚¢
        
        float4 bottom = float4(block[index].offset, 1);
        bottom.y -= side_h;
        bottom = mul(cam.proj, mul(cam.view, bottom));
        if (bottom.y < -cullOffset)return;   //‰f‚Á‚Ä‚¢‚È‚¢
        
        outputCommands.Append(inputCommands[index]);
    }
}
