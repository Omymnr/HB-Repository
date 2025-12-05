// RendererBridge.cpp
// Implementación del puente entre DXC_ddraw y el nuevo sistema de renderizado

#include "RendererBridge.h"
#include "DirectDrawRenderer.h"
#include "Direct3D11Renderer.h"
#include "SpriteRenderer.h"
#include "TextureManager.h"
#include "Sprite.h"
#include <stdio.h>

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

CRendererBridge::CRendererBridge()
    : m_pActiveRenderer(nullptr)
    , m_pLegacyDDraw(nullptr)
    , m_pD3D11Renderer(nullptr)
    , m_pDDrawWrapper(nullptr)
    , m_bInitialized(false)
    , m_bUsingD3D11(false)
    , m_hWnd(nullptr)
{
}

CRendererBridge::~CRendererBridge()
{
    Shutdown();
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================

bool CRendererBridge::Initialize(HWND hWnd, DXC_ddraw* pLegacyDDraw)
{
    if (m_bInitialized) {
        return true;
    }
    
    m_hWnd = hWnd;
    m_pLegacyDDraw = pLegacyDDraw;
    
    // Obtener configuración
    CRendererConfig& config = CRendererConfig::GetInstance();
    
    // Intentar cargar configuración del registro o archivo
    if (!config.LoadFromRegistry()) {
        config.LoadFromFile("video.cfg");
    }
    
    RendererType requestedType = config.GetRendererType();
    
    // Si es AUTO, detectar el mejor renderer
    if (requestedType == RENDERER_AUTO) {
        requestedType = CRendererConfig::DetectBestRenderer();
    }
    
    // Intentar inicializar el renderer solicitado
    bool success = false;
    
    if (requestedType == RENDERER_DIRECT3D11) {
        // Intentar inicializar D3D11
        OutputDebugStringA("RendererBridge: Intentando inicializar Direct3D 11...\n");
        
        if (CRendererConfig::IsD3D11Available()) {
            m_pD3D11Renderer = new Direct3D11Renderer();
            if (m_pD3D11Renderer->Initialize(hWnd, config.GetScreenWidth(), config.GetScreenHeight(), 
                                        config.IsFullscreen())) {
                m_pActiveRenderer = m_pD3D11Renderer;
                m_bUsingD3D11 = true;
                success = true;
                
                // Inicializar el TextureManager para D3D11
                CTextureManager::GetInstance().Initialize(m_pD3D11Renderer);
                
                // Inicializar el SpriteRenderer para D3D11
                CSpriteRenderer::GetInstance().Initialize(m_pD3D11Renderer, m_pLegacyDDraw);
                CSpriteRenderer::GetInstance().SetUseD3D11(true);
                
                char msg[256];
                sprintf(msg, "RendererBridge: Direct3D 11 inicializado (%dx%d)\n", 
                        config.GetScreenWidth(), config.GetScreenHeight());
                OutputDebugStringA(msg);
            }
            else {
                delete m_pD3D11Renderer;
                m_pD3D11Renderer = nullptr;
                requestedType = RENDERER_DIRECTDRAW;
                OutputDebugStringA("RendererBridge: D3D11 fallo, usando DirectDraw\n");
            }
        }
        else {
            OutputDebugStringA("RendererBridge: D3D11 no disponible, usando DirectDraw\n");
            requestedType = RENDERER_DIRECTDRAW;
        }
    }
    
    if (requestedType == RENDERER_DIRECTDRAW && !success) {
        // Usar DirectDraw (el legacy ya está inicializado por CGame)
        // Creamos un wrapper que usa m_pLegacyDDraw
        m_pDDrawWrapper = new DirectDrawRenderer();
        m_pDDrawWrapper->SetLegacyDDraw(m_pLegacyDDraw);
        
        // No llamamos Init() porque el DXC_ddraw ya está inicializado
        // El wrapper simplemente delega las llamadas
        
        m_pActiveRenderer = m_pDDrawWrapper;
        m_bUsingD3D11 = false;
        success = true;
        
        // Inicializar SpriteRenderer en modo DirectDraw
        CSpriteRenderer::GetInstance().Initialize(nullptr, m_pLegacyDDraw);
        CSpriteRenderer::GetInstance().SetUseD3D11(false);
        
        OutputDebugStringA("RendererBridge: Usando DirectDraw (legacy)\n");
    }
    
    if (success) {
        m_bInitialized = true;
    }
    
    return success;
}

void CRendererBridge::Shutdown()
{
    // Liberar SpriteRenderer primero
    CSpriteRenderer::GetInstance().Shutdown();
    
    // Liberar TextureManager
    CTextureManager::GetInstance().Shutdown();
    
    if (m_pD3D11Renderer) {
        m_pD3D11Renderer->Shutdown();
        delete m_pD3D11Renderer;
        m_pD3D11Renderer = nullptr;
    }
    
    if (m_pDDrawWrapper) {
        // No llamamos Shutdown() porque no somos dueños del DXC_ddraw
        delete m_pDDrawWrapper;
        m_pDDrawWrapper = nullptr;
    }
    
    m_pActiveRenderer = nullptr;
    m_pLegacyDDraw = nullptr;
    m_bInitialized = false;
}

// ============================================================================
// CAMBIO DE RENDERER EN RUNTIME
// ============================================================================

bool CRendererBridge::SwitchRenderer(RendererType type)
{
    // Esta función permite cambiar de renderer en runtime
    // Útil para testing y configuración
    
    if (!m_bInitialized) {
        return false;
    }
    
    // Si ya estamos usando el tipo solicitado, no hacer nada
    if ((type == RENDERER_DIRECT3D11 && m_bUsingD3D11) ||
        (type == RENDERER_DIRECTDRAW && !m_bUsingD3D11)) {
        return true;
    }
    
    CRendererConfig& config = CRendererConfig::GetInstance();
    
    if (type == RENDERER_DIRECT3D11) {
        // Cambiar a D3D11
        if (!m_pD3D11Renderer) {
            m_pD3D11Renderer = new Direct3D11Renderer();
            if (!m_pD3D11Renderer->Initialize(m_hWnd, config.GetScreenWidth(), 
                                         config.GetScreenHeight(), config.IsFullscreen())) {
                delete m_pD3D11Renderer;
                m_pD3D11Renderer = nullptr;
                return false;
            }
            
            // Inicializar TextureManager para el nuevo dispositivo D3D11
            CTextureManager::GetInstance().Shutdown();
            CTextureManager::GetInstance().Initialize(m_pD3D11Renderer);
        }
        
        m_pActiveRenderer = m_pD3D11Renderer;
        m_bUsingD3D11 = true;
        
        // Actualizar SpriteRenderer
        CSpriteRenderer::GetInstance().Initialize(m_pD3D11Renderer, m_pLegacyDDraw);
        CSpriteRenderer::GetInstance().SetUseD3D11(true);
        
        OutputDebugStringA("RendererBridge: Cambiado a Direct3D 11\n");
    }
    else {
        // Cambiar a DirectDraw
        if (!m_pDDrawWrapper && m_pLegacyDDraw) {
            m_pDDrawWrapper = new DirectDrawRenderer();
            m_pDDrawWrapper->SetLegacyDDraw(m_pLegacyDDraw);
        }
        
        if (m_pDDrawWrapper) {
            m_pActiveRenderer = m_pDDrawWrapper;
            m_bUsingD3D11 = false;
            
            // Actualizar SpriteRenderer
            CSpriteRenderer::GetInstance().SetUseD3D11(false);
            
            OutputDebugStringA("RendererBridge: Cambiado a DirectDraw\n");
        }
        else {
            return false;
        }
    }
    
    // Guardar preferencia
    config.SetRendererType(type);
    config.SaveToRegistry();
    
    return true;
}

// ============================================================================
// FUNCIONES BRIDGE
// ============================================================================

void CRendererBridge::BeginFrame()
{
    if (m_pActiveRenderer) {
        m_pActiveRenderer->BeginFrame();
    }
    
    // Iniciar frame en SpriteRenderer
    CSpriteRenderer::GetInstance().BeginFrame();
}

void CRendererBridge::EndFrame()
{
    // Finalizar frame en SpriteRenderer (flush any pending sprites)
    CSpriteRenderer::GetInstance().EndFrame();
    
    if (m_pActiveRenderer) {
        m_pActiveRenderer->EndFrame();
    }
}

HRESULT CRendererBridge::Flip()
{
    if (m_bUsingD3D11 && m_pD3D11Renderer && m_pLegacyDDraw) {
        // MODO HÍBRIDO: Los sprites se dibujan en DirectDraw
        // Copiamos el backbuffer de DirectDraw a D3D11 y presentamos
        
        // Obtener acceso al backbuffer de DirectDraw
        LPDIRECTDRAWSURFACE7 lpBackBuffer = m_pLegacyDDraw->m_lpBackB4;
        if (lpBackBuffer) {
            DDSURFACEDESC2 ddsd;
            ZeroMemory(&ddsd, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            
            HRESULT hr = lpBackBuffer->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_READONLY, NULL);
            if (SUCCEEDED(hr)) {
                // Copiar backbuffer a D3D11 y presentar
                // No usamos color key para el backbuffer porque queremos todos los píxeles
                m_pD3D11Renderer->PresentDirectDrawBackBuffer(
                    ddsd.lpSurface,
                    ddsd.lPitch,
                    ddsd.dwWidth,
                    ddsd.dwHeight,
                    0xFFFF  // Color key inválido para que nada sea transparente
                );
                
                lpBackBuffer->Unlock(NULL);
                return S_OK;
            }
            else {
                char msg[128];
                sprintf(msg, "RendererBridge::Flip - Lock failed: 0x%08X\n", hr);
                OutputDebugStringA(msg);
            }
        }
        
        // Fallback: D3D11 solo usa Present()
        return m_pD3D11Renderer->EndFrame();
    }
    else if (m_pLegacyDDraw) {
        // DirectDraw usa iFlip()
        return m_pLegacyDDraw->iFlip();
    }
    
    return E_FAIL;
}

void CRendererBridge::ClearBackBuffer()
{
    if (m_bUsingD3D11 && m_pD3D11Renderer) {
        // D3D11 limpia automáticamente en BeginFrame
    }
    else if (m_pLegacyDDraw) {
        m_pLegacyDDraw->ClearBackB4();
    }
}

void CRendererBridge::DrawSprite(int x, int y, void* pSprite, int frame, DWORD dwTime)
{
    // Usar SpriteRenderer para dibujar
    // El SpriteRenderer automáticamente usa D3D11 o DirectDraw según configuración
    CSprite* sprite = (CSprite*)pSprite;
    if (sprite) {
        CSpriteRenderer::GetInstance().DrawSprite(sprite, x, y, frame, dwTime);
    }
}

void CRendererBridge::DrawText(int x, int y, const char* text, DWORD color)
{
    if (m_pActiveRenderer) {
        m_pActiveRenderer->DrawText(x, y, text, color);
    }
}

void CRendererBridge::DrawShadowBox(short sX, short sY, short dX, short dY, int iType)
{
    if (m_pLegacyDDraw) {
        m_pLegacyDDraw->DrawShadowBox(sX, sY, dX, dY, iType);
    }
}

void CRendererBridge::OnResize(int width, int height)
{
    if (m_bUsingD3D11 && m_pD3D11Renderer) {
        m_pD3D11Renderer->OnResize(width, height);
    }
    
    // Actualizar configuración
    CRendererConfig& config = CRendererConfig::GetInstance();
    config.SetResolution(width, height);
}
