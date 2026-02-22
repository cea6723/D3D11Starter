#include "Transform.h"
using namespace DirectX;


Transform::Transform()
{
	position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

	right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	forward = XMFLOAT3(0.0f, 0.0f, 1.0f);

	XMStoreFloat4x4(&world, XMMatrixIdentity());
	XMStoreFloat4x4(&worldInverseTranspose, XMMatrixIdentity());

	isDirty = false;
}

void Transform::SetPosition(float x, float y, float z)
{
	SetPosition(XMFLOAT3(x, y, z));
}

void Transform::SetPosition(DirectX::XMFLOAT3 _position)
{
	position = _position;

	isDirty = true;
}

void Transform::SetRotation(float pitch, float yaw, float roll)
{
	SetRotation(XMFLOAT3(pitch, yaw, roll));
}

void Transform::SetRotation(DirectX::XMFLOAT3 _rotation)
{
	rotation = _rotation;
	UpdateVectors();

	isDirty = true;
}

void Transform::SetScale(float x, float y, float z)
{
	SetScale(XMFLOAT3(x, y, z));
}

void Transform::SetScale(DirectX::XMFLOAT3 _scale)
{
	scale = _scale;

	isDirty = true;
}

DirectX::XMFLOAT3 Transform::GetPosition()
{
	return position;
}

DirectX::XMFLOAT3 Transform::GetPitchYawRoll()
{
	return rotation;
}

DirectX::XMFLOAT3 Transform::GetScale()
{
	return scale;
}

DirectX::XMFLOAT3 Transform::GetRight()
{
	return right;
}

DirectX::XMFLOAT3 Transform::GetUp()
{
	return up;
}

DirectX::XMFLOAT3 Transform::GetForward()
{
	return forward;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	if (isDirty)
	{
		XMMATRIX translationMat = XMMatrixTranslation(position.x, position.y, position.z);
		XMMATRIX rotationMat = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
		XMMATRIX scaleMat = XMMatrixScaling(scale.x, scale.y, scale.z);

		XMMATRIX worldMat = scaleMat * rotationMat * translationMat;

		XMStoreFloat4x4(&world, worldMat);
		XMStoreFloat4x4(&worldInverseTranspose, XMMatrixInverse(0, XMMatrixTranspose(worldMat)));

		isDirty = false;
	}

	return world;
}

DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
	return worldInverseTranspose;
}

void Transform::MoveAbsolute(float x, float y, float z)
{
	MoveAbsolute(XMFLOAT3(x, y, z));
}

void Transform::MoveAbsolute(DirectX::XMFLOAT3 offset)
{
	XMVECTOR posVec = XMLoadFloat3(&position);
	XMVECTOR offsetVec = XMLoadFloat3(&offset);
	posVec += offsetVec;
	XMStoreFloat3(&position, posVec);

	isDirty = true;
}

void Transform::MoveRelative(float x, float y, float z)
{
	MoveRelative(XMFLOAT3(x, y, z));
}

void Transform::MoveRelative(DirectX::XMFLOAT3 offset)
{
	XMVECTOR rot = XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMVECTOR offsetVec = XMLoadFloat3(&offset);
	XMVECTOR posVec = XMLoadFloat3(&position);

	offsetVec = XMVector3Rotate(offsetVec, rot);
	posVec += offsetVec;
	XMStoreFloat3(&position, posVec);

	isDirty = true;
}

void Transform::Rotate(float pitch, float yaw, float roll)
{
	Rotate(XMFLOAT3(pitch, yaw, roll));
}

void Transform::Rotate(DirectX::XMFLOAT3 _rotation)
{
	XMVECTOR rotVec = XMLoadFloat3(&rotation);
	XMVECTOR pYRVec = XMLoadFloat3(&_rotation);
	rotVec += pYRVec;
	XMStoreFloat3(&rotation, rotVec);

	UpdateVectors();

	isDirty = true;
}

void Transform::Scale(float x, float y, float z)
{
	Scale(XMFLOAT3(x, y, z));
}

void Transform::Scale(DirectX::XMFLOAT3 _scale)
{
	XMVECTOR scaleVec = XMLoadFloat3(&scale);
	XMVECTOR scalarVec = XMLoadFloat3(&_scale);
	scaleVec *= scalarVec;
	XMStoreFloat3(&scale, scaleVec);

	isDirty = true;
}

void Transform::UpdateVectors()
{
	XMVECTOR rot = XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMVECTOR rightVec = XMLoadFloat3(&right);
	XMVECTOR upVec = XMLoadFloat3(&up);
	XMVECTOR forwardVec = XMLoadFloat3(&forward);

	rightVec = XMVector3Rotate(rightVec, rot);
	upVec = XMVector3Rotate(upVec, rot);
	forwardVec = XMVector3Rotate(forwardVec, rot);

	XMStoreFloat3(&right, rightVec);
	XMStoreFloat3(&up, upVec);
	XMStoreFloat3(&forward, forwardVec);
}