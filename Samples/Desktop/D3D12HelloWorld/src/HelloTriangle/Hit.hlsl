#include "Common.hlsl"

ByteAddressBuffer g_indices : register(t1);
StructuredBuffer<Vertex> g_vertex : register(t2);

[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
	float3 RayCenter = float3(1 - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

	uint IndexSizeInBytes = 4;           
	uint IndicesPerTriangle = 3;
	uint TriangleIndexStride = IndexSizeInBytes * IndicesPerTriangle;

	uint BaseIndex = PrimitiveIndex() * TriangleIndexStride;
	const uint3 Indices = g_indices.Load3(BaseIndex);
	
	float4 AColor = g_vertex[Indices[0]].color;
	float4 BColor = g_vertex[Indices[1]].color;
	float4 CColor = g_vertex[Indices[2]].color;

	float3 FinalColor = AColor.xyz * RayCenter.x + BColor.xyz * RayCenter.y + CColor.xyz * RayCenter.z;

	payload.colorAndDistance = float4(FinalColor.x, FinalColor.y, FinalColor.z, RayTCurrent());
}
