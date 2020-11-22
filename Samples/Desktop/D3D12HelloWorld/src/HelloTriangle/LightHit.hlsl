

#include "Common.hlsl"

ConstantBuffer<SphereLightData> g_sphereLightData :  register(b1);



bool IsInRange(in float val, in float min, in float max)
{
	return (val >= min && val <= max);
}

bool IsCulled(in Ray ray, in float3 hitSurfaceNormal)
{
	float rayDirectionNormalDot = dot(ray.direction, hitSurfaceNormal);

	bool isCulled = ((RayFlags() & RAY_FLAG_CULL_BACK_FACING_TRIANGLES) && (rayDirectionNormalDot > 0)) || ((RayFlags() & RAY_FLAG_CULL_FRONT_FACING_TRIANGLES) && (rayDirectionNormalDot < 0));

	return isCulled;
}

bool IsAValidHit(in Ray ray, in float thit, in float3 hitSurfaceNormal)
{
	return IsInRange(thit, RayTMin(), RayTCurrent()) && !IsCulled(ray, hitSurfaceNormal);
}


// Calculate a normal for a hit point on a sphere.
float3 CalculateNormalForARaySphereHit(in Ray ray, in float thit, float3 center)
{
	float3 hitPosition = ray.origin + thit * ray.direction;
	return normalize(hitPosition - center);
}

//计算一元二次方程 a * X * X + bX + c = 0 的根
//返回true则有根，返回false则没有根
bool SolveQuadratic(in float a, in float b, in float c, out float x0, out float x1)
{
	float minT = 0.0f;
	float maxT = 0.0f;

	float discr = b * b - 4 * a * c;
	if (discr < 0) return false;
	else if (discr == 0) x0 = x1 = -0.5 * b / a;
	else {
		float q = (b > 0) ?
			-0.5 * (b + sqrt(discr)) :
			-0.5 * (b - sqrt(discr));
		minT = x0 = q / a;
		maxT = x1 = c / q;
	}
	if (x0 > x1)
	{
		minT = x1;
		maxT = x0;
	}

	x0 = minT;
	x1 = maxT;

	return true;
}

bool SphereHitTest(out HitLightAttributes attr,out float thit, in Ray ray, in float3 worldCenter,in float radius)
{
	float3 L = ray.origin - worldCenter;
	float a = dot(ray.direction, ray.direction);
	float b = 2 * dot(ray.direction, L);
	float c = dot(L, L) - radius * radius;

	float tMin, tMax;
	if (!SolveQuadratic(a, b, c, tMin, tMax))    
		return false; //光线不与球相交

	if (tMin < RayTMin())
	{
		if (tMax < RayTMin())
			return false; //球的光线原点的后面

		//光线原点在球中。
		attr.Normal = CalculateNormalForARaySphereHit(ray, tMin, worldCenter);
		if (IsAValidHit(ray, tMin, attr.Normal))
		{
			thit = tMin;
			return true;
		}
	}
	else
	{
		attr.Normal = CalculateNormalForARaySphereHit(ray, tMin, worldCenter);
		if (IsAValidHit(ray, tMin, attr.Normal))
		{
			thit = tMin;
			return true;
		}

		attr.Normal = CalculateNormalForARaySphereHit(ray, tMax, worldCenter);
		if (IsAValidHit(ray, tMax, attr.Normal))
		{
			thit = tMax;
			return true;
		}
	}
	
	return false;
}

[shader("closesthit")]
void ClosestHit_SphereLight(inout HitInfo payload, in HitLightAttributes attrib)
{
	float4 LightColor = g_sphereLightData.m_color;
	payload.colorAndDistance = float4(LightColor.x, LightColor.y, LightColor.z,RayTCurrent());
}

[shader("intersection")]
void Intersection_SphereLight()
{
	Ray ray;
	ray.origin = WorldRayOrigin();
	ray.direction = WorldRayDirection();

	HitLightAttributes HitAttr;
	float tCurrent;

	if (SphereHitTest(HitAttr, tCurrent, ray, g_sphereLightData.m_worldPos, g_sphereLightData.m_radius))
	{
		//属性的最大大小为32个字节，定义为D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES。
		ReportHit(tCurrent, /*hitKind*/0, HitAttr);
	}
}