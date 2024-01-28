#pragma once

#include <d3d11.h>
#include <vector>
#include <array>

void getBufferContent(ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext, ID3D11Buffer* pVertexBuf, std::vector<BYTE>* bufferData);

template <std::size_t N>
void getBuffersContent(ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext, const std::array<ID3D11Buffer*, N>& buffersPointer, unsigned int buffersCount, std::vector<std::vector<BYTE>>* buffersData);
