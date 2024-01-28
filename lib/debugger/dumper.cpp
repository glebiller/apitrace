#include "dumper.hpp"

#include <d3d11state.hpp>
#include <sstream>
#include <directxtex/DirectXTex/DirectXTex.h>
#include <wrl/client.h>

#include "base64.hpp"
#include "image.hpp"

namespace interceptor {
    static void getRenderTargetViewImage(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext,
                             ID3D11RenderTargetView* pRenderTargetView, DirectX::ScratchImage* pDstImage,
                             DXGI_FORMAT* dxgiFormat) {
        if (!pRenderTargetView) {
            return;
        }

        ID3D11Resource* pResource;
        pRenderTargetView->GetResource(&pResource);

        if (pResource) {
            ID3D11Texture2D* pTexture;
            if (HRESULT hr = pResource->QueryInterface<ID3D11Texture2D>(&pTexture); SUCCEEDED(hr)) {
                CaptureTexture(pDevice, pDeviceContext, pTexture, *pDstImage);
            }

            if (pTexture) {
                pTexture->Release();
            }
        }
        if (pResource) {
            pResource->Release();
        }

        /*// TODO: Take the slice in consideration
        UINT MipSlice;
        switch (Desc.ViewDimension) {
            case D3D11_RTV_DIMENSION_BUFFER:
                MipSlice = 0;
                break;
            case D3D11_RTV_DIMENSION_TEXTURE1D:
                MipSlice = Desc.Texture1D.MipSlice;
                break;
            case D3D11_RTV_DIMENSION_TEXTURE1DARRAY:
                MipSlice = Desc.Texture1DArray.MipSlice;
                break;
            case D3D11_RTV_DIMENSION_TEXTURE2D:
                MipSlice = Desc.Texture2D.MipSlice;
                break;
            case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
                MipSlice = Desc.Texture2DArray.MipSlice;
                break;
            case D3D11_RTV_DIMENSION_TEXTURE2DMS:
                MipSlice = 0;
                break;
            case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
                MipSlice = 0;
                break;
            case D3D11_RTV_DIMENSION_TEXTURE3D:
                MipSlice = Desc.Texture3D.MipSlice;
                break;
            case D3D11_RTV_DIMENSION_UNKNOWN:
            default:
                assert(0);
                return nullptr;
        }

        return d3dstate::getSubResourceImage(pDevice, pResource.Get(), Desc.Format, 0, MipSlice);*/
    }

    void CopyToScratchImage(const image::Image* srcImage, DirectX::ScratchImage* dstImage) {
        dstImage->Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, srcImage->width, srcImage->height, 1, 1);
        memcpy(dstImage->GetPixels(), srcImage->pixels, srcImage->sizeInBytes());
        for (size_t x = 0; x < dstImage->GetPixelsSize(); x += 4) {
            dstImage->GetPixels()[x + 3] = 255;
        }
    }

    void ResizeImage(const image::Image* srcImage, DirectX::ScratchImage* dstImage, const int maxWidth) {
        assert(srcImage->channelType == image::TYPE_UNORM8);

        const double aspectRatio = static_cast<double>(srcImage->width) / srcImage->height;
        const auto newHeight = static_cast<unsigned int>(maxWidth / aspectRatio);

        if (srcImage->width < maxWidth || srcImage->height == 0) {
            CopyToScratchImage(srcImage, dstImage);
        } else {
            auto* originalImage = new DirectX::ScratchImage();
            CopyToScratchImage(srcImage, originalImage);
            Resize(originalImage->GetImages(), originalImage->GetImageCount(), originalImage->GetMetadata(), maxWidth, newHeight, DirectX::TEX_FILTER_DEFAULT, *dstImage);
        }
    }

    std::string GetRenderTargetInBase64(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext) {
        if (pDeviceContext->GetType() == D3D11_DEVICE_CONTEXT_DEFERRED) {
            return "";
        }

        ID3D11RenderTargetView* pRenderTargetView;
        ID3D11DepthStencilView* pDepthStencilView;
        pDeviceContext->OMGetRenderTargets(1, &pRenderTargetView, &pDepthStencilView);

        if (!pRenderTargetView) {
            return "no target view";
        }

        DirectX::Blob pngImage;
        auto* pScratchImage = new DirectX::ScratchImage();
        getRenderTargetViewImage(pDevice, pDeviceContext, pRenderTargetView, pScratchImage, nullptr);

        if (pRenderTargetView) {
            pRenderTargetView->Release();
        }
        if (pDepthStencilView) {
            pDepthStencilView->Release();
        }

#ifdef DUMPER_RESIZED_IMAGE
        ResizeImage(image, scratchImage, 512);
#endif
        if (pScratchImage->GetImageCount() > 0) {
            const HRESULT hr = SaveToWICMemory(*pScratchImage->GetImages(), DirectX::WIC_FLAGS_NONE, GetWICCodec(DirectX::WIC_CODEC_JPEG), pngImage);
            assert(SUCCEEDED(hr));
            delete pScratchImage;
        }

        /*if (const image::Image* image = getRenderTargetViewImage(pDevice, pDeviceContext, pRenderTargetViews, nullptr)) {
            auto* scratchImage = new DirectX::ScratchImage();
#ifdef DUMPER_RESIZED_IMAGE
            ResizeImage(image, scratchImage, 512);
#else
            CopyToScratchImage(image, scratchImage);
#endif
            delete image;

            SaveToWICMemory(*scratchImage->GetImages(), DirectX::WIC_FLAGS_NONE, GetWICCodec(DirectX::WIC_CODEC_JPEG), pngImage);
            delete scratchImage;
        }*/

        return  base64::to_base64(std::string(static_cast<const char*>(pngImage.GetBufferPointer()), pngImage.GetBufferSize()));
    }
}
