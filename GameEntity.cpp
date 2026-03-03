#include "GameEntity.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> _mesh, std::shared_ptr<Material> _material)
{
	transform = std::make_shared<Transform>();
	mesh = _mesh;
	material = _material;
}

std::shared_ptr<Mesh> GameEntity::GetMesh()
{
	return mesh;
}

std::shared_ptr<Transform> GameEntity::GetTransform()
{
	return transform;
}

std::shared_ptr<Material> GameEntity::GetMaterial()
{
	return material;
}

void GameEntity::SetMaterial(std::shared_ptr<Material> _material)
{
	material = _material;
}

void GameEntity::Draw()
{
	Graphics::Context->VSSetShader(material->GetVertexShader().Get(), 0, 0);
	Graphics::Context->PSSetShader(material->GetPixelShader().Get(), 0, 0);
	mesh->Draw();
}
