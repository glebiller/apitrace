#pragma once

#include <d3d11.h>
#include <json/json.hpp>

static D3D11_BUFFER_DESC EMPTY_BUFFER_DESC = D3D11_BUFFER_DESC{};

inline bool operator==(const D3D11_BUFFER_DESC& lhs, const D3D11_BUFFER_DESC& rhs) {
    return lhs.ByteWidth == rhs.ByteWidth &&
        lhs.StructureByteStride == rhs.StructureByteStride;
}

struct ExecutionInfo {
    int frame;
    unsigned int drawCall;
};

struct Viewport {
    float width;
    float height;
    float topLeftX;
    float topLeftY;

    bool operator==(const Viewport& other) const {
        constexpr float epsilon = 0.00001f;
        return std::abs(width - other.width) < epsilon
            && std::abs(height - other.height) < epsilon
            && std::abs(topLeftX - other.topLeftX) < epsilon
            && std::abs(topLeftY - other.topLeftY) < epsilon;
    }

    bool operator<(const Viewport& other) const {
        return std::tie(width, height, topLeftX, topLeftY) <
            std::tie(other.width, other.height, other.topLeftX, other.topLeftY);
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Viewport, width, height, topLeftX, topLeftY);
};
