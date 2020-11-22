// Hit information, aka ray payload
// This sample only carries a shading color and hit distance.
// Note that the payload should be kept as small as possible,
// and that its size must be declared in the corresponding
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobjet.
struct HitInfo
{
  float4 colorAndDistance;
};


struct Ray
{
	float3 origin;
	float3 direction;
};

struct HitLightAttributes
{
	float3 Normal;
};

struct Vertex
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float4 color : COLOR;
	float2 uv1 : TEXCOORD0;
	float2 uv2 : TEXCOORD1;
};

struct SphereLightData
{
	float3   m_worldPos;
	float    m_radius;
	float4   m_color;
};

cbuffer SceneData : register(b0)
{
	float4x4 g_ViewProj;
	float4x4 g_ProjectionToWorld;
	float4   g_CameraPos;
	float4   g_CameraInfo;
	float4   g_AmbientColor;
	float4   g_DirectionalLightDirection;
	float4   g_DirectionalLightColor;
};

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0);




