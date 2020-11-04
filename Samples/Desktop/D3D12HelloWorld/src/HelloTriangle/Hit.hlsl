#include "Common.hlsl"

ByteAddressBuffer g_indices : register(t1);
StructuredBuffer<Vertex> g_vertex : register(t2);

float3 HitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}


[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
	float3 RayCenter = float3(1 - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
	float3 RayHitWorldPos = HitWorldPosition();

	//得到击中的三角形的索引
	uint IndexSizeInBytes = 4;           
	uint IndicesPerTriangle = 3;
	uint TriangleIndexStride = IndexSizeInBytes * IndicesPerTriangle;
	uint BaseIndex = PrimitiveIndex() * TriangleIndexStride;
	const uint3 Indices = g_indices.Load3(BaseIndex);
	
	//计算击中点世界法线
	float3 ANormal = g_vertex[Indices[0]].normal;
	float3 BNormal = g_vertex[Indices[1]].normal;
	float3 CNormal = g_vertex[Indices[2]].normal;
	float3 RayHitWorldNormal = ANormal * RayCenter.x + BNormal * RayCenter.y + CNormal * RayCenter.z;

	//得到击中点顶点颜色的插值。
	float4 AColor = g_vertex[Indices[0]].color;
	float4 BColor = g_vertex[Indices[1]].color;
	float4 CColor = g_vertex[Indices[2]].color;
	float3 RayHitColor = AColor.xyz * RayCenter.x + BColor.xyz * RayCenter.y + CColor.xyz * RayCenter.z;

	//朗伯余弦定律
	float3 LightDir = -g_DirectionalLightDirection.xyz;
	float fNDotL = max(0, dot(LightDir , RayHitWorldNormal));

	//计算漫反射颜色
	float3 FinalColor = RayHitColor * g_DirectionalLightColor.xyz * fNDotL;

	payload.colorAndDistance = float4(FinalColor.x, FinalColor.y, FinalColor.z, RayTCurrent());
}
