#include "Common.hlsl"

ByteAddressBuffer g_indices : register(t1);
StructuredBuffer<Vertex> g_vertex : register(t2);

float3 HitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

//BuiltInTriangleIntersectionAttributes �������������ཻ�������صĽ�������
//�����Σ�A1,A2,A3����BuiltInTriangleIntersectionAttributes::barycentrics.x�ǵ�A2��Ȩ�أ�BuiltInTriangleIntersectionAttributes::barycentrics.y��A3��Ȩ�ء�
[shader("closesthit")] 
void ClosestHit_Triangle(inout HitInfo payload, in BuiltInTriangleIntersectionAttributes attrib) 
{
	float3 RayCenter = float3(1 - attrib.barycentrics.x - attrib.barycentrics.y, attrib.barycentrics.x, attrib.barycentrics.y);
	float3 RayHitWorldPos = HitWorldPosition();

	//PrimitiveIndex()
	//����D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES�����Ǽ��ζ����ڵ�������������
	//����D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS������AABB�����ж��弸�ζ����������

	//�õ����е������ε�����
	uint IndexSizeInBytes = 4;           
	uint IndicesPerTriangle = 3;
	uint TriangleIndexStride = IndexSizeInBytes * IndicesPerTriangle;
	uint BaseIndex = PrimitiveIndex() * TriangleIndexStride;
	const uint3 Indices = g_indices.Load3(BaseIndex);
	
	//������е����編��
	float3 ANormal = g_vertex[Indices[0]].normal;
	float3 BNormal = g_vertex[Indices[1]].normal;
	float3 CNormal = g_vertex[Indices[2]].normal;
	float3 RayHitWorldNormal = ANormal * RayCenter.x + BNormal * RayCenter.y + CNormal * RayCenter.z;

	//�õ����е㶥����ɫ�Ĳ�ֵ��
	float4 AColor = g_vertex[Indices[0]].color;
	float4 BColor = g_vertex[Indices[1]].color;
	float4 CColor = g_vertex[Indices[2]].color;
	float3 RayHitColor = AColor.xyz * RayCenter.x + BColor.xyz * RayCenter.y + CColor.xyz * RayCenter.z;

	//�ʲ����Ҷ���
	float3 LightDir = -g_DirectionalLightDirection.xyz;
	float fNDotL = max(0, dot(LightDir , RayHitWorldNormal));

	//������������ɫ
	float3 FinalColor = RayHitColor * g_DirectionalLightColor.xyz * fNDotL;

	payload.colorAndDistance = float4(FinalColor.x, FinalColor.y, FinalColor.z, RayTCurrent());
}
