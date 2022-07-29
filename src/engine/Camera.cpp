#include "Camera.h"
#include"D3D12App.h"
#include"WinApp.h"

void Camera::CameraInfoUpdate()
{
	if (dirty)
	{
		cameraInfo.eye = pos;

		//視点座標
		XMVECTOR eyePosition = XMLoadFloat3((XMFLOAT3*)&cameraInfo.eye);
		//注視点座標
		XMVECTOR targetPosition = XMLoadFloat3((XMFLOAT3*)&target);
		//（仮の）上方向
		XMVECTOR upVector = XMLoadFloat3((XMFLOAT3*)&up);

		//カメラZ軸（視線方向）
		XMVECTOR cameraAxisZ = XMVectorSubtract(targetPosition, eyePosition);

		//０ベクトルだと向きが定まらないので除外
		assert(!XMVector3Equal(cameraAxisZ, XMVectorZero()));
		assert(!XMVector3IsInfinite(cameraAxisZ));
		assert(!XMVector3Equal(upVector, XMVectorZero()));
		assert(!XMVector3IsInfinite(upVector));

		//ベクトルを正規化
		cameraAxisZ = XMVector3Normalize(cameraAxisZ);

		//カメラのX軸（右方向）
		XMVECTOR cameraAxisX;
		//X軸は上方向→Z軸の外積で求まる
		cameraAxisX = XMVector3Cross(upVector, cameraAxisZ);
		//ベクトルを正規化
		cameraAxisX = XMVector3Normalize(cameraAxisX);

		//カメラのY軸（上方向）
		XMVECTOR cameraAxisY;
		//Y軸はZ軸→X軸の外積で求まる
		cameraAxisY = XMVector3Cross(cameraAxisZ, cameraAxisX);

		//カメラ回転行列
		XMMATRIX matCameraRot;
		//カメラ座標系→ワールド座標系の変換行列
		matCameraRot.r[0] = cameraAxisX;
		matCameraRot.r[1] = cameraAxisY;
		matCameraRot.r[2] = cameraAxisZ;
		matCameraRot.r[3] = XMVectorSet(0, 0, 0, 1);

		//転置により逆行列（逆回転）を計算
		cameraInfo.matView = XMMatrixTranspose(matCameraRot);

		//視点座標に-1をかけた座標
		XMVECTOR reverseEyePosition = XMVectorNegate(eyePosition);
		//カメラの位置からワールド原点へのベクトル（カメラ座標系）
		XMVECTOR tX = XMVector3Dot(cameraAxisX, reverseEyePosition);
		XMVECTOR tY = XMVector3Dot(cameraAxisY, reverseEyePosition);
		XMVECTOR tZ = XMVector3Dot(cameraAxisZ, reverseEyePosition);
		//一つのベクトルにまとめる
		XMVECTOR translateion = XMVectorSet(tX.m128_f32[0], tY.m128_f32[1], tZ.m128_f32[2], 1.0f);

		//ビュー行列に平行移動成分を設定
		cameraInfo.matView.r[3] = translateion;

		//全方向ビルボード行列の計算
		cameraInfo.billboardMat = XMMatrixInverse(nullptr, cameraInfo.matView);
		//cameraInfo.billboardMat.r[0] = cameraAxisX;
		//cameraInfo.billboardMat.r[1] = cameraAxisY;
		//cameraInfo.billboardMat.r[2] = cameraAxisZ;
		cameraInfo.billboardMat.r[3] = XMVectorSet(0, 0, 0, 1);

		//Y軸回りビルボード行列の計算
		//カメラX軸、Y軸、Z軸
		XMVECTOR ybillCameraAxisX, ybillCameraAxisY, ybillCameraAxisZ;

		//X軸は共通
		ybillCameraAxisX = cameraAxisX;
		//Y軸はワールド座標系のY軸
		ybillCameraAxisY = XMVector3Normalize(upVector);
		//Z軸はX軸→Y軸の外積で求まる
		ybillCameraAxisZ = XMVector3Cross(cameraAxisX, cameraAxisY);

		//Y軸回りビルボード行列
		cameraInfo.billboardMatY.r[0] = ybillCameraAxisX;
		cameraInfo.billboardMatY.r[1] = ybillCameraAxisY;
		cameraInfo.billboardMatY.r[2] = ybillCameraAxisZ;
		cameraInfo.billboardMatY.r[3] = XMVectorSet(0, 0, 0, 1);

		if (projMatMode == Perspective)
		{
			cameraInfo.matProjection = XMMatrixPerspectiveFovLH(
				angleOfView,								//画角
				aspect,	//アスペクト比
				nearZ, farZ);		//前端、奥端
		}
		else
		{
			cameraInfo.matProjection = DirectX::XMMatrixOrthographicLH(width, height, nearZ, farZ);
		}

		buff->Mapping(&cameraInfo);

		viewInvMat = XMMatrixInverse(nullptr, cameraInfo.matView);

		dirty = false;
	}
}

Camera::Camera(const std::string& Name) : name(Name)
{
	auto initData = ConstData();
	buff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(ConstData), 1, &initData, Name.c_str());
	aspect = WinApp::Instance()->GetAspect();
}

const std::shared_ptr<ConstantBuffer>& Camera::GetBuff()
{
	CameraInfoUpdate();
	return buff;
}

const Vec3<float>Camera::GetForward()
{
	CameraInfoUpdate();
	return Vec3<float>(viewInvMat.r[2].m128_f32[0], viewInvMat.r[2].m128_f32[1], viewInvMat.r[2].m128_f32[2]);
}

const Vec3<float>Camera::GetRight()
{
	CameraInfoUpdate();
	return Vec3<float>(viewInvMat.r[0].m128_f32[0], viewInvMat.r[0].m128_f32[1], viewInvMat.r[0].m128_f32[2]);
}
