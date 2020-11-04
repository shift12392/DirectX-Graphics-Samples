//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

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

cbuffer ObjectData : register(b1)
{
	float4x4 g_loaclToWorld;
}

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float4 color : COLOR;
	float2 uv1 : TEXCOORD0;
	float2 uv2 : TEXCOORD1;
};

struct PSInput
{
	float4 position : SV_POSITION;    //注：这个是在齐次剪裁空间中的位置，必须是float4类型，因为第4个分量有特别的意思，
	float3 WorldPos : POSITION;
	float3 Normal : NORMAL;
	float4 color : COLOR;
};

PSInput VSMain(VSInput input)
{
	PSInput result;

	float4 WorldPos = mul(float4(input.position.x, input.position.y, input.position.z, 1.0f), g_loaclToWorld);


	result.position = mul(WorldPos, g_ViewProj);
	result.color = input.color;
	result.WorldPos = WorldPos.xyz;
	result.Normal = input.normal;

	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
