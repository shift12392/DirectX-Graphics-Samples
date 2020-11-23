#include "Common.hlsl"

[shader("miss")]
void Miss_Radiance(inout HitInfo payload : SV_RayPayload)
{
    payload.colorAndDistance = float4(1.0f, 0.0f, 0.0f, -1.f);
}


[shader("miss")]
void Miss_Shadow(inout HitInfo payload : SV_RayPayload)
{
    payload.colorAndDistance = float4(0.0f, 0.0f, 1.0f, -1.f);
}