#pragma once

#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>

#include "Transform.h"
#include "Mesh.h"

class GameEntity
{
private:
	std::shared_ptr<Transform> transform;
	std::shared_ptr<Mesh> mesh;

public:
	GameEntity(std::shared_ptr<Mesh> _mesh);

	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Transform> GetTransform();

	void Draw();
};

