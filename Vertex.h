#pragma once

struct SimpleVertex
{
	DirectX::XMFLOAT3 Position;
};

//Vertex struct for colors. We do not handle textures.
struct Vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT4 color; //w-value is opacity
	DirectX::XMFLOAT3 normal;
};