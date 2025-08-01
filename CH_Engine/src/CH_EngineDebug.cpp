
#ifndef CH_ENGINEDEBUG_H
#define CH_ENGINEDEBUG_H

#include "CH_main.h"
#include "CH_scene.h"
#include "CH_camera.h"
#include "CH_texture.h"
#include <stdio.h>
#include <string>

namespace CHDebug {

    // Debug output helpers
#define CH_DEBUG_PRINT(fmt, ...) printf("[CH_DEBUG] " fmt "\n", ##__VA_ARGS__)
#define CH_DEBUG_ERROR(fmt, ...) printf("[CH_ERROR] " fmt "\n", ##__VA_ARGS__)
#define CH_DEBUG_SUCCESS(fmt, ...) printf("[CH_SUCCESS] " fmt "\n", ##__VA_ARGS__)

// HRESULT to string converter
    const char* HResultToString(HRESULT hr)
    {
        switch (hr)
        {
        case S_OK: return "S_OK";
        case E_INVALIDARG: return "E_INVALIDARG";
        case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
        case E_FAIL: return "E_FAIL";
        case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL";
        case DXGI_ERROR_DEVICE_REMOVED: return "DXGI_ERROR_DEVICE_REMOVED";
        case DXGI_ERROR_DEVICE_RESET: return "DXGI_ERROR_DEVICE_RESET";
        case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD:
            return "D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD";
        default:
        {
            static char buffer[32];
            sprintf_s(buffer, "0x%08X", hr);
            return buffer;
        }
        }
    }

    // Validate DirectX 11 objects
    bool ValidateD3D11Objects()
    {
        CH_DEBUG_PRINT("Validating DirectX 11 objects...");

        bool valid = true;

        if (!g_D3DDevice)
        {
            CH_DEBUG_ERROR("g_D3DDevice is NULL");
            valid = false;
        }
        else
        {
            CH_DEBUG_SUCCESS("g_D3DDevice is valid");
        }

        if (!g_D3DContext)
        {
            CH_DEBUG_ERROR("g_D3DContext is NULL");
            valid = false;
        }
        else
        {
            CH_DEBUG_SUCCESS("g_D3DContext is valid");
        }

        if (!g_SwapChain)
        {
            CH_DEBUG_ERROR("g_SwapChain is NULL");
            valid = false;
        }
        else
        {
            CH_DEBUG_SUCCESS("g_SwapChain is valid");
        }

        if (!g_RenderTargetView)
        {
            CH_DEBUG_ERROR("g_RenderTargetView is NULL");
            valid = false;
        }
        else
        {
            CH_DEBUG_SUCCESS("g_RenderTargetView is valid");
        }

        if (!g_DepthStencilView)
        {
            CH_DEBUG_ERROR("g_DepthStencilView is NULL");
            valid = false;
        }
        else
        {
            CH_DEBUG_SUCCESS("g_DepthStencilView is valid");
        }

        return valid;
    }

    // Test basic DirectX operations
    bool TestBasicOperations()
    {
        CH_DEBUG_PRINT("Testing basic DirectX operations...");

        if (!ValidateD3D11Objects())
            return false;

        // Test clear operations
        float clearColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
        g_D3DContext->ClearRenderTargetView(g_RenderTargetView.Get(), clearColor);
        g_D3DContext->ClearDepthStencilView(g_DepthStencilView.Get(),
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
            1.0f, 0);
        CH_DEBUG_SUCCESS("Clear operations completed");

        // Test present
        HRESULT hr = g_SwapChain->Present(0, 0);
        if (FAILED(hr))
        {
            CH_DEBUG_ERROR("Present failed: %s", HResultToString(hr));
            return false;
        }
        CH_DEBUG_SUCCESS("Present operation successful");

        return true;
    }

    // Validate scene data
    bool ValidateScene(CHScene* scene)
    {
        if (!scene)
        {
            CH_DEBUG_ERROR("Scene is NULL");
            return false;
        }

        CH_DEBUG_PRINT("Validating scene data...");

        if (!scene->lpVB)
        {
            CH_DEBUG_ERROR("Scene vertex buffer is NULL");
            return false;
        }

        if (!scene->lpIB)
        {
            CH_DEBUG_ERROR("Scene index buffer is NULL");
            return false;
        }

        if (scene->dwVecCount == 0)
        {
            CH_DEBUG_ERROR("Scene has no vertices");
            return false;
        }

        if (scene->dwTriCount == 0)
        {
            CH_DEBUG_ERROR("Scene has no triangles");
            return false;
        }

        // Check vertex data integrity
        for (DWORD i = 0; i < scene->dwVecCount; i++)
        {
            CHSceneVertex* v = &scene->lpVB[i];

            // Check for NaN or infinite values
            if (!isfinite(v->x) || !isfinite(v->y) || !isfinite(v->z))
            {
                CH_DEBUG_ERROR("Vertex %d has invalid position: (%.3f, %.3f, %.3f)",
                    i, v->x, v->y, v->z);
                return false;
            }

            // Check normal vectors
            float normalLength = sqrtf(v->nx * v->nx + v->ny * v->ny + v->nz * v->nz);
            if (normalLength < 0.1f || normalLength > 10.0f)
            {
                CH_DEBUG_ERROR("Vertex %d has invalid normal: (%.3f, %.3f, %.3f) length=%.3f",
                    i, v->nx, v->ny, v->nz, normalLength);
            }
        }

        // Check index data
        for (DWORD i = 0; i < scene->dwTriCount * 3; i++)
        {
            if (scene->lpIB[i] >= scene->dwVecCount)
            {
                CH_DEBUG_ERROR("Index %d references invalid vertex %d (max: %d)",
                    i, scene->lpIB[i], scene->dwVecCount - 1);
                return false;
            }
        }
    }
}
#endif