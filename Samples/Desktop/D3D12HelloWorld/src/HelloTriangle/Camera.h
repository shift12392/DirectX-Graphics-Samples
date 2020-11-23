#pragma once

#include "Common/MathHelper.h"
#include <string>


	class Camera
	{
	public:
		DirectX::XMFLOAT4 m_pos = DirectX::XMFLOAT4(0,0,0,1);
		DirectX::XMFLOAT4X4 m_view = MathHelper::Identity4x4();          //观察矩阵
		DirectX::XMFLOAT4X4 m_projection = MathHelper::Identity4x4();    //透视投影矩阵
		DirectX::XMFLOAT4X4 m_viewProj = MathHelper::Identity4x4();      //观察-透视投影矩阵
		DirectX::XMFLOAT4X4 m_ProjToWorld = MathHelper::Identity4x4();   //齐次剪裁空间到世界空间矩阵
		float m_aspectRatio = 0;
		float m_near = 1.0f;
		float m_far = 10000.0f;
		std::wstring m_name;

		void Update()
		{
			//更新相机
		}

		static std::shared_ptr<Camera> CreateProjectionCamera(const std::wstring &name,float aspectRatio)
		{

			std::shared_ptr<Camera> NewCamera(new Camera());
			NewCamera->m_name = name;
			NewCamera->m_aspectRatio = aspectRatio;
			NewCamera->m_near = 1;


			//计算投影矩阵
			DirectX::XMMATRIX Projection = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspectRatio, NewCamera->m_near, NewCamera->m_far);
			XMStoreFloat4x4(&(NewCamera->m_projection), Projection);

			//设置相机位置和朝向
			DirectX::XMVECTOR Pos = DirectX::XMVectorSet(-300.0f, 300.0f, 300.0f, 1.0f);
			XMStoreFloat4(&NewCamera->m_pos, Pos);
			DirectX::XMVECTOR Target = DirectX::XMVectorZero();
			DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
			DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(Pos, Target, Up);
			XMStoreFloat4x4(&NewCamera->m_view, View);

			//计算观察-透视投影矩阵
			DirectX::XMMATRIX ViewProj = DirectX::XMMatrixMultiply(View, Projection);
			XMStoreFloat4x4(&(NewCamera->m_viewProj), XMMatrixTranspose(ViewProj));

			//计算剪裁空间到世界空间的逆变换
			DirectX::XMVECTOR DetViewProj = DirectX::XMMatrixDeterminant(ViewProj);
			DirectX::XMMATRIX ProjToWorld = DirectX::XMMatrixInverse(&DetViewProj, ViewProj);

			XMStoreFloat4x4(&(NewCamera->m_ProjToWorld), XMMatrixTranspose(ProjToWorld));

			return NewCamera;
		}

		DirectX::XMFLOAT4X4 GetViewProjMatrix() const
		{
			return m_viewProj;
		}

	};




