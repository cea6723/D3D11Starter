#include "GameEntity.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> _mesh)
{
	transform = std::make_shared<Transform>();
	mesh = _mesh;
}

std::shared_ptr<Mesh> GameEntity::GetMesh()
{
	return mesh;
}

std::shared_ptr<Transform> GameEntity::GetTransform()
{
	return transform;
}

void GameEntity::Draw()
{
	mesh->Draw();
}
