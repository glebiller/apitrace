import sys
from dlltrace import DllTracer
from dxgitrace import D3DCommonTracer
from trace import getWrapperInterfaceName
from specs import stdapi
from specs.stdapi import API
from specs import dxgi
from specs import d3d10
from specs import d3d11
from specs import dcomp
from specs import d3d9


interceptedInterfaces = [d3d11.IDXGISwapChain, d3d11.ID3D11Device, d3d11.ID3D11DeviceContext]
interceptedInterfaceMethods = {
    d3d11.IDXGISwapChain: ["Present"],
    d3d11.ID3D11Device: ["CreateInputLayout"],
    d3d11.ID3D11DeviceContext: ["DrawIndexed", "IASetInputLayout", "IASetVertexBuffers", "IASetIndexBuffer", "VSSetConstantBuffers", "PSSetConstantBuffers", "PSSetShaderResources", "RSSetViewports"]
}

class D3DInterceptor(D3DCommonTracer):

    def wrapInterface(self, iface):
        for baseInterface in interceptedInterfaces:
            if iface.hasBase(baseInterface):
                return baseInterface
        return None

    def invokeFunction(self, function):
        D3DCommonTracer.invokeFunction(self, function)

        if function.name == "D3D11CreateDeviceAndSwapChain":
            print(r'        interceptor::stateManager.SetContext(*ppSwapChain, pSwapChainDesc, *ppDevice, *ppImmediateContext);')

    def invokeMethod(self, interface, base, method):
        if interface.hasBase(d3d11.IDXGISwapChain) and method.name == "Present":
            print(r'        interceptor::stateManager.ReportBeforePresent();')
            print(r'        debugger::debugger.Display();')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "DrawIndexed":
            print(r'        bool draw = interceptor::stateManager.ReportBeforeDrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);')
            print(r'        if (draw) {')
        
        D3DCommonTracer.invokeMethod(self, interface, base, method)

        if interface.hasBase(d3d11.ID3D11Device) and method.name == "CreateInputLayout":
            print(r'        interceptor::stateManager.ReportAfterCreateInputLayout(pInputElementDescs, NumElements, pShaderBytecodeWithInputSignature, BytecodeLength, *ppInputLayout);')

        if interface.hasBase(d3d11.IDXGISwapChain) and method.name == "Present":
            print(r'        interceptor::stateManager.ReportAfterPresent();')

        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "DrawIndexed":
            print(r'        }')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "IASetInputLayout":
            print(r'        interceptor::stateManager.ReportIASetInputLayout(pInputLayout);')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "IASetVertexBuffers":
            print(r'        interceptor::stateManager.ReportIASetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, *pStrides, *pOffsets);')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "IASetIndexBuffer":
            print(r'        interceptor::stateManager.ReportIASetIndexBuffer(pIndexBuffer, Format, Offset);')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "VSSetConstantBuffers":
            print(r'        interceptor::stateManager.ReportAfterVSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "PSSetConstantBuffers":
            print(r'        interceptor::stateManager.ReportAfterPSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "PSSetShaderResources":
            print(r'        interceptor::stateManager.ReportAfterPSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "RSSetViewports":
            print(r'        interceptor::stateManager.ReportAfterRSSetViewports(NumViewports, pViewports);')

    def implementWrapperInterface(self, iface):
        if self.wrapInterface(iface) is not None:
            D3DCommonTracer.implementWrapperInterface(self, iface)
        else:
            self.implementNoopWrapperInterface(iface)

    def implementWrapperInterfaceMethodBody(self, interface, base, method):
        wrappedInterface = self.wrapInterface(interface)
        if wrappedInterface is not None and method.name in interceptedInterfaceMethods[wrappedInterface]:
            self.interceptedImplementWrapperInterfaceMethodBody(interface, base, method)
        else:
            D3DCommonTracer.invokeMethod(self, interface, base, method)


    def interceptedImplementWrapperInterfaceMethodBody(self, interface, base, method):
        assert not method.internal

        self.invokeMethod(interface, base, method)

        print('    if (%s) {' % self.wasFunctionSuccessful(method))
        for arg in method.args:
            if arg.output:
                self.wrapArg(method, arg)
        print('    }')

        if method.type is not stdapi.Void:
            self.wrapRet(method, "_result")

    def traceFunctionImplBody(self, function):
        self.invokeFunction(function)
        if not function.internal:
            print('    if (%s) {' % self.wasFunctionSuccessful(function))
            for arg in function.args:
                if arg.output:
                    self.wrapArg(function, arg)
            print('    }')
            if function.type is not stdapi.Void:
                self.wrapRet(function, "_result")

    def implementNoopWrapperInterface(self, iface):
        self.interface = iface

        wrapperInterfaceName = getWrapperInterfaceName(iface)

        # Private constructor
        print('%s::%s(%s * pInstance) {' % (wrapperInterfaceName, wrapperInterfaceName, iface.name))
        for type, name, value in self.enumWrapperInterfaceVariables(iface):
            if value is not None:
                print('    %s = %s;' % (name, value))
        print('}')
        print()

        # Public constructor
        print('%s *%s::_create(const char *entryName, %s * pInstance) {' % (wrapperInterfaceName, wrapperInterfaceName, iface.name))
        print(r'    Wrap%s *pWrapper = new Wrap%s(pInstance);' % (iface.name, iface.name))
        print(r'    g_WrappedObjects[pInstance] = pWrapper;')
        print(r'    return pWrapper;')
        print('}')
        print()

        baseMethods = list(iface.iterBaseMethods())
        for base, method in baseMethods:
            self.base = base
            self.implementWrapperInterfaceMethod(iface, base, method)

        print()

        # Wrap pointer
        ifaces = self.api.getAllInterfaces()
        print(r'void')
        print(r'%s::_wrap(const char *entryName, %s **ppObj) {' % (wrapperInterfaceName, iface.name))
        print(r'}')
        print()

        # Unwrap pointer
        print(r'void')
        print(r'%s::_unwrap(const char *entryName, %s **ppObj) {' % (wrapperInterfaceName, iface.name))
        print(r'}')
        print()

if __name__ == '__main__':
    print(r'#include "guids_defs.hpp"')
    print()
    print(r'#include "trace_writer_noop.hpp"')
    print(r'#include "state_manager.hpp"')
    print(r'#include "debugger.hpp"')
    print(r'#include "os.hpp"')
    print()
    print(r'#include "dxgitrace.hpp"')
    print()

    # TODO: Expose this via a runtime option
    print('#define FORCE_D3D_FEATURE_LEVEL_11_0 0')
    print()

    print(r'namespace trace {')
    print(r'    NoopWriter localWriter;')
    print(r'    void fakeMemcpy(const void *ptr, size_t size) {}')
    print(r'}')
    print()

    api = API()
    api.addModule(dxgi.dxgi)
    api.addModule(d3d10.d3d10)
    api.addModule(d3d10.d3d10_1)
    api.addModule(d3d11.d3d11)
    api.addModule(dcomp.dcomp)
    api.addModule(d3d9.d3dperf)

    interceptor = D3DInterceptor()
    interceptor.traceApi(api)
