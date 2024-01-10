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


interceptedInterfaces = [d3d11.IDXGISwapChain, d3d11.ID3D11DeviceContext]
interceptedInterfaceMethods = {
    d3d11.IDXGISwapChain: ["Present"],
    d3d11.ID3D11DeviceContext: ["DrawIndexed", "IASetVertexBuffers", "IASetIndexBuffer", "VSSetConstantBuffers", "PSSetConstantBuffers", "RSSetViewports", "Map", "Unmap"]
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
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "Unmap":
            print(r'        interceptor::stateManager.ReportBeforeUnmap(pResource, Subresource);')
        if interface.hasBase(d3d11.IDXGISwapChain) and method.name == "Present":
            print(r'        interceptor::stateManager.ReportBeforePresent();')

        D3DCommonTracer.invokeMethod(self, interface, base, method)

        if interface.hasBase(d3d11.IDXGISwapChain) and method.name == "Present":
            print(r'        interceptor::stateManager.ReportAfterPresent();')

        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "DrawIndexed":
            print(r'        interceptor::stateManager.ReportDraw(IndexCount, StartIndexLocation, BaseVertexLocation);')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "IASetVertexBuffers":
            print(r'        interceptor::stateManager.ReportIASetVertexBuffers(*pStrides, NumBuffers, ppVertexBuffers);')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "IASetIndexBuffer":
            print(r'        interceptor::stateManager.ReportIASetIndexBuffer(pIndexBuffer);')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "VSSetConstantBuffers":
            print(r'        interceptor::stateManager.ReportAfterVSSetConstantBuffers(NumBuffers, ppConstantBuffers);')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "PSSetConstantBuffers":
            print(r'        interceptor::stateManager.ReportAfterPSSetConstantBuffers(NumBuffers, ppConstantBuffers);')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "RSSetViewports":
            print(r'        interceptor::stateManager.ReportViewport(NumViewports, pViewports);')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "Map":
            print(r'        interceptor::stateManager.ReportMap(pResource, Subresource, MapType, MapFlags, pMappedResource);')
        if interface.hasBase(d3d11.ID3D11DeviceContext) and method.name == "Unmap":
            print(r'        interceptor::stateManager.ReportAfterUnmap(pResource, Subresource);')

    def implementWrapperInterface(self, iface):
        if self.wrapInterface(iface) is not None:
            D3DCommonTracer.implementWrapperInterface(self, iface)
        else:
            self.implementNoopWrapperInterface(iface)


    def enumWrapperInterfaceVariables(self, interface):
        variables = DllTracer.enumWrapperInterfaceVariables(self, interface)

        # Add additional members to track maps
        if interface.hasBase(*self.mapInterfaces):
            variables += [
                ('_MAP_DESC', 'm_MapDesc', None),
                ('MemoryShadow', 'm_MapShadow', None),
            ]
        if interface.hasBase(d3d11.ID3D11DeviceContext):
            variables += [
                ('std::map< std::pair<ID3D11Resource *, UINT>, _MAP_DESC >', 'm_MapDescs', None),
                ('std::map< std::pair<ID3D11Resource *, UINT>, MemoryShadow >', 'm_MapShadows', None),
            ]
        if interface.hasBase(d3d11.ID3D11VideoContext):
            variables += [
                ('std::map<UINT, std::pair<void *, UINT> >', 'm_MapDesc', None),
            ]

        if interface.hasBase(d3d11.ID3D11DeviceContext):
            variables += [
                ('ID3D11Buffer*', 'm_CurrentVSConstantBuffer', None),
            ]

        return variables

    def implementWrapperInterfaceMethodBody(self, interface, base, method):
        wrappedInterface = self.wrapInterface(interface)
        if wrappedInterface is not None and method.name in interceptedInterfaceMethods[wrappedInterface]:
            self.doImplementWrapperInterfaceMethodBody(interface, base, method)
        else:
            D3DCommonTracer.invokeMethod(self, interface, base, method)

    def doImplementWrapperInterfaceMethodBody(self, interface, base, method):
        if method.getArgByName('pInitialData'):
            pDesc1 = method.getArgByName('pDesc1')
            if pDesc1 is not None:
                print(r'    %s pDesc = pDesc1;' % (pDesc1.type,))

        if method.name == "VSSetConstantBuffers":
            print('    if (ppConstantBuffers && NumBuffers) {')
            print('        m_CurrentVSConstantBuffer = ppConstantBuffers[0];')
            print('    }')

        if method.name in ('Map', 'Unmap'):
            # On D3D11 Map/Unmap is not a resource method, but a context method instead.
            resourceArg = method.getArgByName('pResource')
            if resourceArg is None:
                print('    _MAP_DESC & _MapDesc = m_MapDesc;')
                print('    MemoryShadow & _MapShadow = m_MapShadow;')
            else:
                print('    _MAP_DESC & _MapDesc = m_MapDescs[std::pair<%s, UINT>(pResource, Subresource)];' % resourceArg.type)
                print('    MemoryShadow & _MapShadow = m_MapShadows[std::pair<%s, UINT>(pResource, Subresource)];' % resourceArg.type)

        if method.name == 'Unmap':
            print('        if (_MapDesc.Size && _MapDesc.pData) {')
            print('            if (interceptor::stateManager.ShouldShadowMap(pResource, Subresource)) {')
            print('                _MapShadow.update(([](const void* ptr, size_t size) {')
            # TODO only if pResource == currentBuffer
            print('                    interceptor::stateManager.ReportVSConstantBuffersUpdated(ptr, size);')
            print('                }));')
            print('            }')
            print('        }')


        self.interceptedImplementWrapperInterfaceMethodBody(interface, base, method)

        if method.name == 'Map':
            # NOTE: recursive locks are explicitely forbidden
            print('        if (SUCCEEDED(_result)) {')
            print('            _getMapDesc(_this, %s, _MapDesc);' % ', '.join(method.argNames()))
            print('            if (_MapDesc.pData && interceptor::stateManager.ShouldShadowMap(pResource, Subresource)) {')
            if interface.name.startswith('IDXGI'):
                print('                (void)_MapShadow;')
            else:
                print('                bool _discard = MapType == 4 /* D3D1[01]_MAP_WRITE_DISCARD */;')
                print('                _MapShadow.cover(_MapDesc.pData, _MapDesc.Size, _discard);')
            print('            }')
            print('        } else {')
            print('            _MapDesc.pData = NULL;')
            print('            _MapDesc.Size = 0;')
            print('        }')


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
