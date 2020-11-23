#pragma once

#include "Common/MathHelper.h"
#include <string>


	class Camera
	{
	public:
		DirectX::XMFLOAT4 m_pos = DirectX::XMFLOAT4(0,0,0,1);
		DirectX::XMFLOAT4X4 m_view = MathHelper::Identity4x4();          //�۲����
		DirectX::XMFLOAT4X4 m_projection = MathHelper::Identity4x4();    //͸��ͶӰ����
		DirectX::XMFLOAT4X4 m_viewProj = MathHelper::Identity4x4();      //�۲�-͸��ͶӰ����
		DirectX::XMFLOAT4X4 m_ProjToWorld = MathHelper::Identity4x4();   //��μ��ÿռ䵽����ռ����
		float m_aspectRatio = 0;
		float m_near = 1.0f;
		float m_far = 10000.0f;
		std::wstring m_name;

		void Update()
		{
			//�������
		}

		static std::shared_ptr<Camera> CreateProjectionCamera(const std::wstring &name,float aspectRatio)
		{

			std::shared_ptr<Camera> NewCamera(new Camera());
			NewCamera->m_name = name;
			NewCamera->m_aspectRatio = aspectRatio;
			NewCamera->m_near = 1;


			//����ͶӰ����
			DirectX::XMMATRIX Projection = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspectRatio, NewCamera->m_near, NewCamera->m_far);
			XMStoreFloat4x4(&(NewCamera->m_projection), Projection);

			//�������λ�úͳ���
			DirectX::XMVECTOR Pos = DirectX::XMVectorSet(-300.0f, 300.0f, 300.0f, 1.0f);
			XMStoreFloat4(&NewCamera->m_pos, Pos);
			DirectX::XMVECTOR Target = DirectX::XMVectorZero();
			DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
			DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(Pos, Target, Up);
			XMStoreFloat4x4(&NewCamera->m_view, View);

			//����۲�-͸��ͶӰ����
			DirectX::XMMATRIX ViewProj = DirectX::XMMatrixMultiply(View, Projection);
			XMStoreFloat4x4(&(NewCamera->m_viewProj), XMMatrixTranspose(ViewProj));

			//������ÿռ䵽����ռ����任
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




