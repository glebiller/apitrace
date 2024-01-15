//
// Created by Kissy on 12/28/2023.
//

#include <DirectXMath.h>
#include <tchar.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "d3d11shader.h"
#include "d3dshader.hpp"

#include "operations.hpp"
#include "vertex_shader.hpp"
#include "vertex_shader_simulator.hpp"
/*
void swap(Vector4f& a, const int ai, Vector4f& b, const int bi) {
    float tmp = a.components[ai];
    a.components[ai] = b.components[bi];
    b.components[bi] = tmp;
}

float dp4(const Vector4f i0, const Vector4f i1) {
    return i0.x() * i1.x() + i0.y() * i1.y() + i0.z() * i1.z() + i0.w() * i1.w();
}
*/
int _tmain(int argc, _TCHAR* argv[])
{
    float ViewportWidth = 1495;
    float ViewportHeight = 821;
    float ratio = ViewportWidth / ViewportHeight;

    //Vector4f v0 = {0, 0, 0, 1};
    DirectX::XMVECTOR v = DirectX::XMVectorSet(0, 0, 0, 1);

    DirectX::XMMATRIX viewSpaceMatrixRH = DirectX::XMMatrixSet(-0.998339056968689, 0.057613905519247055, 0.0, 198.75479125976562,
        -0.05761391296982765, -0.998339056968689, 2.9802322387695312e-08, 112.24898529052734,
        0.0, 0.0, 0.9999998807907104, 10.816791534423828,
        0, 0, 0, 1);

    DirectX::XMMATRIX cb1RH = DirectX::XMMatrixSet(1.6177846193313599, -2.083535832753114e-07, 0.0, 0.0,
    9.256793731537982e-08, 2.3832874298095703, 0.5877881646156311, 0.5877852439880371,
    6.725453971512252e-08, 1.7315595149993896, -0.8090211153030396, -0.8090170621871948,
    -334.29132080078125, -274.5928955078125, -26.215600967407227, -26.115468978881836);

    DirectX::XMVECTOR rh = DirectX::XMVectorSet(0, 0, 0, 0);
    rh = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatX(DirectX::XMVector4Dot(viewSpaceMatrixRH.r[0], v)), cb1RH.r[0], rh);
    rh = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatY(DirectX::XMVector4Dot(viewSpaceMatrixRH.r[1], v)), cb1RH.r[1], rh);
    rh = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatZ(DirectX::XMVector4Dot(viewSpaceMatrixRH.r[2], v)), cb1RH.r[2], rh);
    rh = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatW(v), cb1RH.r[3], rh);

    auto projMatrix = DirectX::XMMatrixSet(ViewportHeight / ratio, 0, 0, 0,
        0, ViewportHeight, 0, 0,
        0, 0, 2, 1,
        0, 0, 1, 0
        );

    auto worldMatrix = XMMatrixTranspose(cb1RH);
    auto tmp = DirectX::XMVector4Transform(v, worldMatrix);
    auto rh2 = DirectX::XMVector4Transform(tmp, viewSpaceMatrixRH);
    auto rh3 = DirectX::XMVector3Transform(rh2, projMatrix);

    /*
    DirectX::XMMATRIX viewSpaceMatrix2D = DirectX::XMMatrixSet(0.00133779, 0, 0, 0,
        0, -0.00243605, 0, 0,
        0, 0, -0.5, 0,
        -1, 1, 0.5, 1);


    DirectX::XMVECTOR v2 = DirectX::XMVectorSet(0.199219, 0.223633, 1236, 242);
    DirectX::XMVECTOR output2d = DirectX::XMVectorSet(0, 0, 0, 0);
    output2d = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatX(v2), viewSpaceMatrix2D.r[0], output2d);
    output2d = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatY(v2), viewSpaceMatrix2D.r[1], output2d);
    output2d = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatZ(v2), viewSpaceMatrix2D.r[2], output2d);
    output2d = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatW(v2), viewSpaceMatrix2D.r[3], output2d);
    */

    DirectX::XMMATRIX viewSpaceMatrixRG = DirectX::XMMatrixSet(0.057614028453826904, 0.9983386397361755, -2.9802322387695312e-08, 194.7510223388672,
        -0.9983386993408203, 0.057614024728536606, 1.862645149230957e-09, 107.75479125976562,
        0.0, 0.0, 0.9999998807907104, 10.735579490661621,
        0, 0, 0, 1);

    DirectX::XMMATRIX cb1RG = DirectX::XMMatrixSet(1.6177846193313599, -2.083535832753114e-07, 0.0, 0.0,
        9.256793731537982e-08, 2.3832874298095703, 0.5877881646156311, 0.5877852439880371,
        6.725453971512252e-08, 1.7315595149993896, -0.8090211153030396, -0.8090170621871948,
        -334.29132080078125, -274.5928955078125, -26.215600967407227, -26.115468978881836);

    DirectX::XMVECTOR rg = DirectX::XMVectorSet(0, 0, 0, 0);
    rg = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatX(DirectX::XMVector4Dot(viewSpaceMatrixRG.r[0], v)), cb1RG.r[0], rg);
    rg = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatY(DirectX::XMVector4Dot(viewSpaceMatrixRG.r[1], v)), cb1RG.r[1], rg);
    rg = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatZ(DirectX::XMVector4Dot(viewSpaceMatrixRG.r[2], v)), cb1RG.r[2], rg);
    rg = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatW(v), cb1RG.r[3], rg);

    /*
    dp4 r1.y, cb0[1].xyzw, v0.xyzw
    mul r2.xyzw, r1.yyyy, cb0[198].xyzw
    dp4 r1.x, cb0[0].xyzw, v0.xyzw
    mad r2.xyzw, r1.xxxx, cb0[197].xyzw, r2.xyzw
    dp4 r1.z, cb0[2].xyzw, v0.xyzw
    mad r2.xyzw, r1.zzzz, cb0[199].xyzw, r2.xyzw
    mad o0.xyzw, v0.wwww, cb0[200].xyzw, r2.xyzw

    // dp4 r1.y, cb0[1].xyzw, v0.xyzw
    r1.y(dp4(cb0_1, v0));
    // dp4 r1.x, cb0[0].xyzw, v0.xyzw
    r1.x(dp4(cb0_0, v0));
    // dp4 r1.z, cb0[2].xyzw, v0.xyzw
    r1.z(dp4(cb0_2, v0));
    // mul r2.xyzw, r1.yyyy, cb0[198].xyzw
    r2.x(r1.y() * cb0_198.x());
    r2.y(r1.y() * cb0_198.y());
    r2.z(r1.y() * cb0_198.z());
    r2.w(r1.y() * cb0_198.w());
    // mad r2.xyzw, r1.xxxx, cb0[197].xyzw, r2.xyzw
    r2.x(r1.x() * cb0_197.x() + r2.x());
    r2.y(r1.x() * cb0_197.y() + r2.y());
    r2.z(r1.x() * cb0_197.z() + r2.z());
    r2.w(r1.x() * cb0_197.w() + r2.w());
    // mad r2.xyzw, r1.zzzz, cb0[199].xyzw, r2.xyzw
    r2.x(r1.z() * cb0_199.x() + r2.x());
    r2.y(r1.z() * cb0_199.y() + r2.y());
    r2.z(r1.z() * cb0_199.z() + r2.z());
    r2.w(r1.z() * cb0_199.w() + r2.w());
    // mad o0.xyzw, v0.wwww, cb0[200].xyzw, r2.xyzw
    pos.x(v0.w() * cb0_200.x() + r2.x());
    pos.y(v0.w() * cb0_200.y() + r2.y());
    pos.z(v0.w() * cb0_200.z() + r2.z());
    pos.w(v0.w() * cb0_200.w() + r2.w());

    ((x*W0 + y*W1 + z*W2 + w*W3)*V0
    + (x*W4 + y*W5 + z*W6 + w*W7)*V1
    + (x*W8 + y*W9 + z*W10 + w*W11)*V2
    + (x*W12 + y*W13 + z*W14 + w*W15)*V3)


    (cb0_0.x * v0.x + cb0_0.y * v0.y + cb0_0.z * v0.z+ cb0_0.w * v0.w) * cb0_197.x() +
    (cb0_1.x * v0.x + cb0_1.y * v0.y + cb0_1.z * v0.z+ cb0_1.w * v0.w) * cb0_198.x() +
    (cb0_2.x * v0.x + cb0_2.y * v0.y + cb0_2.z * v0.z+ cb0_2.w * v0.w) * cb0_199.x() +
    (cb0_3.x * v0.x + cb0_3.y * v0.y + cb0_3.z * v0.z+ cb0_3.w * v0.w) * cb0_200.x()
    */

    float xx, yy;

    xx = rh.m128_f32[0] / rh.m128_f32[3] * (ViewportWidth / 2.0f) + ViewportWidth / 2.0f;
    yy = ViewportHeight / 2.0f - rh.m128_f32[0] / rh.m128_f32[3] * (ViewportHeight / 2.0f); //- or + depends on the game
    std::cout << xx << " " << yy << std::endl;

    xx = rg.m128_f32[0] / rg.m128_f32[3] * (ViewportWidth / 2.0f) + ViewportWidth / 2.0f;
    yy = ViewportHeight / 2.0f - rg.m128_f32[0] / rg.m128_f32[3] * (ViewportHeight / 2.0f); //- or + depends on the game
    std::cout << xx << " " << yy << std::endl;

    return 0;
}

