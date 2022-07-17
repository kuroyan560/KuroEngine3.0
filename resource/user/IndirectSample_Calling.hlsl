#include"../engine/Camera.hlsli"
#define threadBlockSize 128

struct Block
{
    float4 color;
    float scale;
    float3 vel;
    float3 offset;
};

struct IndirectCommand
{
    uint2 cbvAddress[2];
    uint4 drawArguments;
};

cbuffer Camera : register(b0)
{
    Camera cam;
}

cbuffer RootConstants : register(b1)
{
    float xOffset;        // Half the width of the triangles.
    float zOffset;        // The z offset for the triangle vertices.
    float cullOffset;    // The culling plane offset in homogenous space.
    float commandCount;    // The number of commands to be processed.
};

StructuredBuffer<Block> block                : register(t0);    // SRV: Wrapped constant buffers
StructuredBuffer<IndirectCommand> inputCommands            : register(t1);    // SRV: Indirect commands
AppendStructuredBuffer<IndirectCommand> outputCommands    : register(u0);    // UAV: Processed indirect commands

[numthreads(threadBlockSize, 1, 1)]
void CSmain(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    // Each thread of the CS operates on one of the indirect commands.
    uint index = (groupId.x * threadBlockSize) + groupIndex;

    // Don't attempt to access commands that don't exist if more threads are allocated
    // than commands.
    if (index < commandCount)
    {
        // Project the left and right bounds of the triangle into homogenous space.
        float4 left = float4(-xOffset, 0.0f, zOffset, 1.0f) + float4(block[index].offset, 1);
        left = mul(cam.proj, mul(cam.view, left));
        left /= left.w;

        float4 right = float4(xOffset, 0.0f, zOffset, 1.0f) + float4(block[index].offset, 1);
        right = mul(cam.proj, mul(cam.view, right));
        right /= right.w;

        // Only draw triangles that are within the culling space.
        if (-cullOffset < right.x && left.x < cullOffset)
        {
            outputCommands.Append(inputCommands[index]);
        }
    }
}
