#pragma once

#include <d3d11.h>
#include <string>
#include <vector>

void getCurrentVSConstantBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext, std::vector<char>* bufferData);
void getCurrentVertexBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext, UINT stride, std::vector<char>* bufferData);
void getCurrentIndexBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext, UINT stride, std::vector<char>* bufferData);
void writeBufferToFile(const std::string& filename, const std::vector<char>& buffer);
