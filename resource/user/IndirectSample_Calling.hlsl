#include"../engine/Camera.hlsli"
#define threadBlockSize 128

struct Block
{
    matrix proj;
    matrix view;
    float4 color;
    float scale;
    float3 vel;
    float3 offset;
};

struct IndirectCommand
{
    uint64_t cbvAddress[2];
    //uint64_t cbvAddress;
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
        //float4 left = float4(-xOffset, 0.0f, zOffset, 1.0f) + float4(block[index].offset, 1);
        //left /= left.w;
        //left = mul(cam.proj, mul(cam.view, left));

        //float4 right = float4(xOffset, 0.0f, zOffset, 1.0f) + float4(block[index].offset, 1);
        //right /= right.w;
        //right = mul(cam.proj, mul(cam.view, right));

        //if (-cullOffset < right.x && left.x < cullOffset)
        //{
            //outputCommands.Append(inputCommands[index]);
        //}
        
        //if (block[index].offset.y < 0.0f)
        //    if (0.0f < block[index].offset.y)
        //{
            outputCommands.Append(inputCommands[index]);
        //}
    }
}
