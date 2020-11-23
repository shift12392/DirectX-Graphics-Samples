


#ifndef RAYTRACINGHLSLCOMPAT_H
#define RAYTRACINGHLSLCOMPAT_H

#ifdef HLSL
#include "HLSLCompat.h"
#endif // HLSL

namespace RayType
{
	enum Enum
	{
		ERadianceRay,
		EShadowRay,
		ECount
	};
}


struct ObjectData
{
	DirectX::XMFLOAT4X4 m_localToWorld;
	DirectX::XMFLOAT4 albedo;
	float reflectanceCoef;
	float diffuseCoef;
	float specularCoef;
	float specularPower;
};

#endif
