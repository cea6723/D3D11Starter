#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <string>

#include "Graphics.h"
#include "Vertex.h"

class Mesh
{
public:
	Mesh(const char* _name, Vertex* vertices, int _verticesNum, unsigned int* indices, int _indiciesNum);
	~Mesh();

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();

	int GetVertexCount();
	int GetIndexCount();

	const char* GetName();

	void Draw();
private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	int verticesNum;
	int indiciesNum;

	const char* name;
};

