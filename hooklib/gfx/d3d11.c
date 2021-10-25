#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hook/com-proxy.h"
#include "hook/table.h"

#include "hooklib/config.h"
#include "hooklib/dll.h"
#include "hooklib/gfx/gfx.h"

#include "util/dprintf.h"

typedef HRESULT (WINAPI *D3D11CreateDevice_t)(
        IDXGIAdapter *pAdapter,
        D3D_DRIVER_TYPE DriverType,
        HMODULE Software,
        UINT Flags,
        const D3D_FEATURE_LEVEL *ppFeatureLevels,
        UINT FeatureLevels,
        UINT SDKVersion,
        ID3D11Device **ppDevice,
        D3D_FEATURE_LEVEL *pFeatureLevel,
        ID3D11DeviceContext **ppImmediateContext);
typedef HRESULT (WINAPI *D3D11CreateDeviceAndSwapChain_t)(
        IDXGIAdapter *pAdapter,
        D3D_DRIVER_TYPE DriverType,
        HMODULE Software,
        UINT Flags,
        const D3D_FEATURE_LEVEL *ppFeatureLevels,
        UINT FeatureLevels,
        UINT SDKVersion,
        const DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
        IDXGISwapChain **ppSwapChain,
        ID3D11Device **ppDevice,
        D3D_FEATURE_LEVEL *pFeatureLevel,
        ID3D11DeviceContext **ppImmediateContext);

static struct gfx_config gfx_config;
static D3D11CreateDevice_t next_D3D11CreateDevice;
static D3D11CreateDeviceAndSwapChain_t next_D3D11CreateDeviceAndSwapChain;

static const struct hook_symbol d3d11_hooks[] = {
    {
        .name = "D3D11CreateDevice",
        .patch = D3D11CreateDevice,
        .link = (void **) &next_D3D11CreateDevice,
    }, {
        .name = "D3D11CreateDeviceAndSwapChain",
        .patch = D3D11CreateDeviceAndSwapChain,
        .link = (void **) &next_D3D11CreateDeviceAndSwapChain,
    },
};

void gfx_d3d11_hook_init(const struct gfx_config *cfg, HINSTANCE self)
{
    HMODULE d3d11;

    assert(cfg != NULL);

    if (!cfg->enable) {
        return;
    }

    memcpy(&gfx_config, cfg, sizeof(*cfg));
    hook_table_apply(NULL, "d3d11.dll", d3d11_hooks, _countof(d3d11_hooks));

    if (next_D3D11CreateDevice == NULL || next_D3D11CreateDeviceAndSwapChain == NULL) {
        d3d11 = LoadLibraryW(L"d3d11.dll");

        if (d3d11 == NULL) {
            dprintf("D3D11: d3d11.dll not found or failed initialization\n");

            goto fail;
        }

        if (next_D3D11CreateDevice == NULL) {
            next_D3D11CreateDevice = (D3D11CreateDevice_t) GetProcAddress(
                    d3d11,
                    "D3D11CreateDevice");
        }
        if (next_D3D11CreateDeviceAndSwapChain == NULL) {
            next_D3D11CreateDeviceAndSwapChain = (D3D11CreateDeviceAndSwapChain_t) GetProcAddress(
                    d3d11,
                    "D3D11CreateDeviceAndSwapChain");
        }

        if (next_D3D11CreateDevice == NULL) {
            dprintf("D3D11: D3D11CreateDevice not found in loaded d3d11.dll\n");

            goto fail;
        }
        if (next_D3D11CreateDeviceAndSwapChain == NULL) {
            dprintf("D3D11: D3D11CreateDeviceAndSwapChain not found in loaded d3d11.dll\n");

            goto fail;
        }
    }

    if (self != NULL) {
        dll_hook_push(self, L"d3d11.dll");
    }

    return;

fail:
    if (d3d11 != NULL) {
        FreeLibrary(d3d11);
    }
}

HRESULT WINAPI D3D11CreateDevice(
        IDXGIAdapter *pAdapter,
        D3D_DRIVER_TYPE DriverType,
        HMODULE Software,
        UINT Flags,
        const D3D_FEATURE_LEVEL *ppFeatureLevels,
        UINT FeatureLevels,
        UINT SDKVersion,
        ID3D11Device **ppDevice,
        D3D_FEATURE_LEVEL *pFeatureLevel,
        ID3D11DeviceContext **ppImmediateContext)
{
    dprintf("D3D11: D3D11CreateDevice hook hit\n");

    return next_D3D11CreateDevice(
            pAdapter,
            DriverType,
            Software,
            Flags,
            ppFeatureLevels,
            FeatureLevels,
            SDKVersion,
            ppDevice,
            pFeatureLevel,
            ppImmediateContext);
}

HRESULT WINAPI D3D11CreateDeviceAndSwapChain(
        IDXGIAdapter *pAdapter,
        D3D_DRIVER_TYPE DriverType,
        HMODULE Software,
        UINT Flags,
        const D3D_FEATURE_LEVEL *ppFeatureLevels,
        UINT FeatureLevels,
        UINT SDKVersion,
        const DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
        IDXGISwapChain **ppSwapChain,
        ID3D11Device **ppDevice,
        D3D_FEATURE_LEVEL *pFeatureLevel,
        ID3D11DeviceContext **ppImmediateContext)
{
    DXGI_SWAP_CHAIN_DESC *desc;
    HWND hwnd;
    LONG width;
    LONG height;

    dprintf("D3D11: D3D11CreateDeviceAndSwapChain hook hit\n");

    desc = (DXGI_SWAP_CHAIN_DESC *) pSwapChainDesc;

    if (desc != NULL) {
        desc->Windowed = gfx_config.windowed;

        hwnd = desc->OutputWindow;
        width = desc->BufferDesc.Width;
        height = desc->BufferDesc.Height;
    } else {
        hwnd = NULL;
        width = 0;
        height = 0;
    }

    if (hwnd != NULL) {
        /*
        * Ensure window is maximized to avoid a Windows 10 issue where a
        * fullscreen swap chain is not created because the window is minimized
        * at the time of creation.
        */
        ShowWindow(hwnd, SW_RESTORE);

        if (!gfx_config.framed && width > 0 && height > 0) {
            dprintf("DXGI: Resizing window to %ldx%ld\n", width, height);

            SetWindowLongPtrW(hwnd, GWL_STYLE, WS_POPUP);
            SetWindowLongPtrW(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST);

            SetWindowPos(
                    hwnd,
                    HWND_TOP,
                    0,
                    0,
                    width,
                    height,
                    SWP_FRAMECHANGED | SWP_NOSENDCHANGING);

            ShowWindow(hwnd, SW_SHOWMAXIMIZED);
        }
    }

    return next_D3D11CreateDeviceAndSwapChain(
            pAdapter,
            DriverType,
            Software,
            Flags,
            ppFeatureLevels,
            FeatureLevels,
            SDKVersion,
            pSwapChainDesc,
            ppSwapChain,
            ppDevice,
            pFeatureLevel,
            ppImmediateContext);
}

