#include "helpers.hpp"

void getBufferContent(ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext, ID3D11Buffer* pVertexBuf, std::vector<BYTE>* bufferData) {
    D3D11_BUFFER_DESC bufferDesc;
    pVertexBuf->GetDesc(&bufferDesc);

    // TODO make buffer in state manager.

    D3D11_BUFFER_DESC tempBufferDesc = bufferDesc;
    tempBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    tempBufferDesc.Usage = D3D11_USAGE_STAGING;
    tempBufferDesc.BindFlags = 0;
    tempBufferDesc.MiscFlags = 0;

    ID3D11Buffer *pTempBuffer = nullptr;
    pDevice->CreateBuffer(&tempBufferDesc, nullptr, &pTempBuffer);
    pImmediateContext->CopyResource(pTempBuffer, pVertexBuf);

    D3D11_MAPPED_SUBRESOURCE mappedData;
    pImmediateContext->Map(pTempBuffer, 0, D3D11_MAP_READ, 0, &mappedData);

    bufferData->resize(mappedData.DepthPitch);
    std::memcpy(bufferData->data(), mappedData.pData, mappedData.DepthPitch);

    pImmediateContext->Unmap(pTempBuffer, 0);
    pTempBuffer->Release();
}

template void getBuffersContent<32>(ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext, const std::array<ID3D11Buffer*, 32>& buffersPointer, unsigned int buffersCount, std::vector<std::vector<BYTE>>* buffersData);
template void getBuffersContent<15>(ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext, const std::array<ID3D11Buffer*, 15>& buffersPointer, unsigned int buffersCount, std::vector<std::vector<BYTE>>* buffersData);

template <std::size_t N>
void getBuffersContent(ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext, const std::array<ID3D11Buffer*, N>& buffersPointer, unsigned int buffersCount, std::vector<std::vector<BYTE>>* buffersData) {
    buffersData->resize(buffersCount);
    for (int bufferIndex = 0; bufferIndex < buffersCount; bufferIndex++) {
        getBufferContent(pDevice, pImmediateContext, buffersPointer[bufferIndex], &buffersData->data()[bufferIndex]);
    }
}
