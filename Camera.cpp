#include "Camera.h"

using namespace DirectX;
using namespace Input;

Camera::Camera(float _aspectRatio, DirectX::XMFLOAT3 position, float _fov)
{
	transform = std::make_shared<Transform>();
	transform->MoveAbsolute(position);

	XMStoreFloat4x4(&viewMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&projMatrix, XMMatrixIdentity());

	fov = _fov;
	aspectRatio = _aspectRatio;
	nearDist = 0.1f;
	farDist = 1000.0f;
	movementSpeed = 5.0f;
	mouseLookSpeed = 0.01f;
	isPerspective = true;

	UpdateViewMatrix();
	UpdateProjMatrix(aspectRatio);
}

DirectX::XMFLOAT4X4 Camera::GetViewMatrix()
{
	return viewMatrix;
}

DirectX::XMFLOAT4X4 Camera::GetProjMatrix()
{
	return projMatrix;
}

DirectX::XMFLOAT3 Camera::GetPosition()
{
	return transform->GetPosition();
}

float Camera::GetFOV()
{
	return fov;
}

void Camera::UpdateViewMatrix()
{
	XMFLOAT3 pos = transform->GetPosition();
	XMFLOAT3 dir = transform->GetForward();

	XMVECTOR posVec = XMLoadFloat3(&pos);
	XMVECTOR dirVec = XMLoadFloat3(&dir);
	XMVECTOR upVec = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMStoreFloat4x4(&viewMatrix, XMMatrixLookToLH(posVec, dirVec, upVec));
}

void Camera::UpdateProjMatrix(float aspectRatio)
{
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(fov, aspectRatio, nearDist, farDist));
}

void Camera::Update(float dt)
{
	float moveDist = movementSpeed * dt;

	if (KeyDown('W'))
	{
		transform->MoveRelative(0.0f, 0.0f, moveDist);
	}
	if (KeyDown('S'))
	{
		transform->MoveRelative(0.0f, 0.0f, -moveDist);
	}
	if (KeyDown('A'))
	{
		transform->MoveRelative(-moveDist, 0.0f, 0.0f);
	}
	if (KeyDown('D'))
	{
		transform->MoveRelative(moveDist, 0.0f, 0.0f);
	}
	if (KeyDown('E'))
	{
		transform->MoveRelative(0.0f, moveDist, 0.0f);
	}
	if (KeyDown('Q'))
	{
		transform->MoveRelative(0.0f, -moveDist, 0.0f);
	}

	if (Input::MouseLeftDown())
	{
		int cursorMovementX = Input::GetMouseXDelta();
		int cursorMovementY = Input::GetMouseYDelta();

		transform->Rotate(float(cursorMovementY * mouseLookSpeed), float(cursorMovementX * mouseLookSpeed), 0.0f);

		if (transform->GetPitchYawRoll().x > XM_PIDIV2)
		{
			transform->SetRotation(XM_PIDIV2, transform->GetPitchYawRoll().y, transform->GetPitchYawRoll().z);
		}
		else if (transform->GetPitchYawRoll().x < -XM_PIDIV2)
		{
			transform->SetRotation(-XM_PIDIV2, transform->GetPitchYawRoll().y, transform->GetPitchYawRoll().z);
		}

	}

	UpdateViewMatrix();
}
