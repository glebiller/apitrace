#include "dumper.hpp"

#include <d3d11state.hpp>
#include <sstream>
#include <directxtex/DirectXTex/DirectXTex.h>
#include <wrl/client.h>

#include "base64.hpp"
#include "image.hpp"

namespace interceptor {
    static image::Image* getRenderTargetViewImage(ID3D11DeviceContext* pDevice,
                             ID3D11RenderTargetView* pRenderTargetView,
                             DXGI_FORMAT* dxgiFormat) {
        if (!pRenderTargetView) {
            return nullptr;
        }

        Microsoft::WRL::ComPtr<ID3D11Resource> pResource;
        pRenderTargetView->GetResource(&pResource);
        assert(pResource);

        D3D11_RENDER_TARGET_VIEW_DESC Desc;
        pRenderTargetView->GetDesc(&Desc);

        if (dxgiFormat) {
            *dxgiFormat = Desc.Format;
        }

        // TODO: Take the slice in consideration
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

        return d3dstate::getSubResourceImage(pDevice, pResource.Get(), Desc.Format, 0, MipSlice);
    }

    DirectX::ScratchImage* ResizeImage(const image::Image* srcImage, const int maxWidth) {
        assert(srcImage->channelType == image::TYPE_UNORM8);

        const double aspectRatio = static_cast<double>(srcImage->width) / srcImage->height;
        const auto newHeight = static_cast<unsigned int>(maxWidth / aspectRatio);

        auto* originalImage = new DirectX::ScratchImage();
        originalImage->Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, srcImage->width, srcImage->height, 1, 1);
        memcpy(originalImage->GetPixels(), srcImage->pixels, srcImage->sizeInBytes());
        for (size_t x = 0; x < originalImage->GetPixelsSize(); x += 4) {
            originalImage->GetPixels()[x + 3] = 255;
        }

        if (srcImage->width < maxWidth || srcImage->height == 0) {
            return originalImage;
        }
        auto* resizedScratch = new DirectX::ScratchImage();
        Resize(originalImage->GetImages(), originalImage->GetImageCount(), originalImage->GetMetadata(), maxWidth, newHeight, DirectX::TEX_FILTER_DEFAULT, *resizedScratch);
        return resizedScratch;
    }

    std::string GetRenderTargetInBase64(ID3D11DeviceContext* pDevice) {
        if (pDevice->GetType() == D3D11_DEVICE_CONTEXT_DEFERRED) {
            return base64::to_base64("");
        }

        ID3D11RenderTargetView* pRenderTargetViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
        ID3D11DepthStencilView* pDepthStencilView;
        pDevice->OMGetRenderTargets(ARRAYSIZE(pRenderTargetViews), pRenderTargetViews,
                                    &pDepthStencilView);

        for (UINT i = 0; i < ARRAYSIZE(pRenderTargetViews); ++i) {
            if (!pRenderTargetViews[i]) {
                continue;
            }

            DXGI_FORMAT dxgiFormat;
            if (const image::Image* image = getRenderTargetViewImage(pDevice, pRenderTargetViews[i],
                                                                     &dxgiFormat)) {
#ifdef DUMPER_RESIZED_IMAGE
                const DirectX::ScratchImage* resizedImage = ResizeImage(image, 512);
                DirectX::Blob pngImage;
                SaveToWICMemory(*resizedImage->GetImages(), DirectX::WIC_FLAGS_NONE,
                GetWICCodec(DirectX::WIC_CODEC_JPEG), pngImage);
                delete image;
                pRenderTargetViews[i]->Release();
                return  base64::to_base64(std::string(static_cast<const char*>(pngImage.GetBufferPointer()), pngImage.GetBufferSize()));
#else
                std::stringstream pngImage;
                image->writePNG(pngImage, true);
                delete image;
                pRenderTargetViews[i]->Release();
                return base64::to_base64(pngImage.str());
#endif
            }
            pRenderTargetViews[i]->Release();
        }

        return "";
    }
}
