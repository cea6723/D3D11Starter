#pragma once

#include "Transform.h"
#include "Input.h"

#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>

class Camera
{
private:
	std::shared_ptr<Transform> transform;

	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projMatrix;

	float fov;
	float aspectRatio;
	float nearDist;
	float farDist;
	float movementSpeed;
	float mouseLookSpeed;

	bool isPerspective;

public:
	Camera(float _aspectRatio, DirectX::XMFLOAT3 position, float _fov);

	DirectX::XMFLOAT4X4 GetViewMatrix();
	DirectX::XMFLOAT4X4 GetProjMatrix();
	DirectX::XMFLOAT3 GetPosition();
	float GetFOV();

	void UpdateViewMatrix();
	void UpdateProjMatrix(float aspectRatio);
	void Update(float dt);

};

