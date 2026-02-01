#include "Mesh.h"

Mesh::Mesh(const char* _name, Vertex* vertices, int _verticesNum, unsigned int* indices, int _indiciesNum)
{
	verticesNum = _verticesNum;
	indiciesNum = _indiciesNum;

	name = _name;

	// Create a VERTEX BUFFER
	{
		// Describe buffer
		D3D11_BUFFER_DESC vbd = {};
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Vertex) * verticesNum;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;

		// Create the proper struct to hold the initial vertex data
		D3D11_SUBRESOURCE_DATA initialVertexData = {};
		initialVertexData.pSysMem = vertices; // pSysMem = Pointer to System Memory

		Graphics::Device->CreateBuffer(&vbd, &initialVertexData, vertexBuffer.GetAddressOf());
	}
	// Create an INDEX BUFFER
	{
		// Describe buffer
		D3D11_BUFFER_DESC ibd = {};
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(unsigned int) * indiciesNum;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		ibd.StructureByteStride = 0;

		// Specify the initial data
		D3D11_SUBRESOURCE_DATA initialIndexData = {};
		initialIndexData.pSysMem = indices; // pSysMem = Pointer to System Memory

		// Actually create the buffer with the initial data
		Graphics::Device->CreateBuffer(&ibd, &initialIndexData, indexBuffer.GetAddressOf());
	}
}

Mesh::~Mesh()
{
	// Since you’ll be using smart pointers, the destructor won’t have anything to do (it can be empty).
}

/// <summary>
/// Returns the vertex buffer ComPtr
/// </summary>
Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetVertexBuffer()
{
	return vertexBuffer;
}

/// <summary>
/// Returns the index buffer ComPtr
/// </summary>
Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetIndexBuffer()
{
	return indexBuffer;
}

/// <summary>
/// Returns the number of vertices this mesh contains
/// </summary>
int Mesh::GetVertexCount()
{
	return verticesNum;
}

/// <summary>
/// Returns the number of indices this mesh contains
/// </summary>
int Mesh::GetIndexCount()
{
	return indiciesNum;
}

const char* Mesh::GetName()
{
	return name;
}

/// <summary>
/// Sets the buffers and draws using the correct number of indices
/// </summary>
void Mesh::Draw()
{
	// Set buffers in the input assembler (IA) stage
//  - Do this ONCE PER OBJECT, since each object may have different geometry
//  - For this demo, this step *could* simply be done once during Init()
//  - However, this needs to be done between EACH DrawIndexed() call
//     when drawing different geometry, so it's here as an example
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	Graphics::Context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	Graphics::Context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Tell Direct3D to draw
	//  - Begins the rendering pipeline on the GPU
	//  - Do this ONCE PER OBJECT you intend to draw
	//  - This will use all currently set Direct3D resources (shaders, buffers, etc)
	//  - DrawIndexed() uses the currently set INDEX BUFFER to look up corresponding
	//     vertices in the currently set VERTEX BUFFER
	Graphics::Context->DrawIndexed(
		indiciesNum,     // The number of indices to use (we could draw a subset if we wanted)
		0,     // Offset to the first index we want to use
		0);    // Offset to add to each index when looking up vertices
}
