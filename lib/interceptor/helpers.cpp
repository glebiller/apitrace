#include "helpers.hpp"

#include <fstream>

void getBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext, ID3D11Buffer* pVertexBuf, std::vector<char>* bufferData) {
    D3D11_BUFFER_DESC bufferDesc = {0};
    pVertexBuf->GetDesc(&bufferDesc);

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

    bufferData->resize(bufferDesc.ByteWidth);
    std::memcpy(bufferData->data(), mappedData.pData, bufferDesc.ByteWidth);

    pImmediateContext->Unmap(pTempBuffer, 0);
    pTempBuffer->Release();
}

void getCurrentVSConstantBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext, std::vector<char>* bufferData) {
    ID3D11Buffer *pVertexBuf = nullptr;
    pImmediateContext->VSGetConstantBuffers(0, 1, &pVertexBuf);
    getBuffer(pDevice, pImmediateContext, pVertexBuf, bufferData);
    pVertexBuf->Release();
}

void getCurrentVertexBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext, UINT stride, std::vector<char>* bufferData) {
    ID3D11Buffer *pVertexBuf = nullptr;
    UINT offset = 0;
    pImmediateContext->IAGetVertexBuffers(0, 1, &pVertexBuf, &stride, &offset);
    getBuffer(pDevice, pImmediateContext, pVertexBuf, bufferData);
    pVertexBuf->Release();
}

void getCurrentIndexBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pImmediateContext, UINT stride,
    std::vector<char>* bufferData) {
    ID3D11Buffer *pVertexBuf = nullptr;
    UINT offset = 0;
    DXGI_FORMAT format = DXGI_FORMAT_R16_UINT; // TODO param
    pImmediateContext->IAGetIndexBuffer(&pVertexBuf, &format, &offset);
    getBuffer(pDevice, pImmediateContext, pVertexBuf, bufferData);
    pVertexBuf->Release();
}


void writeBufferToFile(const std::string& filename, const std::vector<char>& buffer) {
    std::ofstream outfile(filename, std::ofstream::binary);
    outfile.write(buffer.data(), buffer.size());
    outfile.close();
}
