#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <unordered_map>

#include "Graphics.h"
#include "Vertex.h"

class Mesh
{
public:
	Mesh(const char* _name, Vertex* vertices, int _verticesNum, unsigned int* indices, int _indiciesNum);
	Mesh(const char* objFile);
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

	void CreateBuffers(Vertex* vertices, int _verticesNum, unsigned int* indices, int _indiciesNum);
	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);
};

