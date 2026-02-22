#include "Camera.h"

using namespace DirectX;
using namespace Input;

Camera::Camera(float _aspectRatio, DirectX::XMFLOAT3 position)
{
    transform = std::make_shared<Transform>();
    transform->MoveAbsolute(position);

    XMStoreFloat4x4(&viewMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&projMatrix, XMMatrixIdentity());

    fov = XM_PIDIV2;
    aspectRatio = _aspectRatio;
    nearDist = 0.1f;
    farDist = 1000.0f;
    movementSpeed = 1.0f;
    mouseLookSpeed = 1.0f;
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

void Camera::UpdateViewMatrix()
{
    XMVECTOR pos = XMLoadFloat3(&transform->GetPosition());
    XMVECTOR dir = XMLoadFloat3(&transform->GetForward());
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMStoreFloat4x4(&viewMatrix, XMMatrixLookToLH(pos, dir, up));
}

void Camera::UpdateProjMatrix(float aspectRatio)
{
    XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(fov, aspectRatio, nearDist, farDist));
}

void Camera::Update(float dt)
{

}
