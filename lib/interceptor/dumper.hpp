#include <string>

#include "d3d11imports.hpp"

namespace interceptor {

    std::string GetRenderTargetInBase64(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext);

}
