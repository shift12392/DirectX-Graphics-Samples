#include "Common.hlsl"

ByteAddressBuffer g_indices : register(t1);
StructuredBuffer<Vertex> g_vertex : register(t2);

float3 HitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

//BuiltInTriangleIntersectionAttributes 是内置三角形相交函数返回的交点属性
//三角形（A1,A2,A3），BuiltInTriangleIntersectionAttributes::barycentrics.x是点A2的权重，BuiltInTriangleIntersectionAttributes::barycentrics.y是A3的权重。
[shader("closesthit")] 
void ClosestHit_Triangle(inout HitInfo payload, in BuiltInTriangleIntersectionAttributes attrib) 
{
	float3 RayCenter = float3(1 - attrib.barycentrics.x - attrib.barycentrics.y, attrib.barycentrics.x, attrib.barycentrics.y);
	float3 RayHitWorldPos = HitWorldPosition();

	//PrimitiveIndex()
	//对于D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES，这是几何对象内的三角形索引。
	//对于D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS，这是AABB数组中定义几何对象的索引。

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
