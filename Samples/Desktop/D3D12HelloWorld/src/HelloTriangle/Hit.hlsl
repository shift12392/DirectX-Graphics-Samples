#include "Common.hlsl"

[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
  //payload.colorAndDistance = float4(1, 1, 0, RayTCurrent());
	payload.colorAndDistance = float4(g_ambientColor.x, g_ambientColor.y, g_ambientColor.z, RayTCurrent());
}
