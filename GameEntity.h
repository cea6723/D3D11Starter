#pragma once

#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>

#include "Transform.h"
#include "Mesh.h"
#include "Material.h"

class GameEntity
{
private:
	std::shared_ptr<Transform> transform;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;

public:
	GameEntity(std::shared_ptr<Mesh> _mesh, std::shared_ptr<Material> _material);

	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Transform> GetTransform();
	std::shared_ptr<Material> GetMaterial();

	void SetMaterial(std::shared_ptr<Material> _material);

	void Draw();
};

