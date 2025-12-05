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
    OutputDebugStringA("[HB-INIT] ============================================\\n");
    OutputDebugStringA("[HB-INIT] RendererBridge::Initialize() INICIANDO\\n");
    OutputDebugStringA("[HB-INIT] ============================================\\n");
    
    if (m_bInitialized) {
        OutputDebugStringA("[HB-INIT] Ya estaba inicializado, retornando\\n");
        return true;
    }
    
    m_hWnd = hWnd;
    m_pLegacyDDraw = pLegacyDDraw;
    
    char msg[256];
    sprintf(msg, "[HB-INIT] hWnd=%p, pLegacyDDraw=%p\\n", hWnd, pLegacyDDraw);
    OutputDebugStringA(msg);
    
    // Obtener configuración
    CRendererConfig& config = CRendererConfig::GetInstance();
    
    // Intentar cargar configuración del registro o archivo
    if (!config.LoadFromRegistry()) {
        OutputDebugStringA("[HB-INIT] No se encontro config en registro, cargando video.cfg\\n");
        config.LoadFromFile("video.cfg");
    }
    
    RendererType requestedType = config.GetRendererType();
    sprintf(msg, "[HB-INIT] Renderer solicitado: %d (0=Auto, 1=DDraw, 2=D3D11Hybrid, 3=D3D11Native)\\n", requestedType);
    OutputDebugStringA(msg);
    
    sprintf(msg, "[HB-INIT] Config: %dx%d, Fullscreen=%d, VSync=%d\\n", 
            config.GetScreenWidth(), config.GetScreenHeight(), 
            config.IsFullscreen(), config.IsVsyncEnabled());
    OutputDebugStringA(msg);
    
    // Si es AUTO, detectar el mejor renderer
    if (requestedType == RENDERER_AUTO) {
        requestedType = CRendererConfig::DetectBestRenderer();
        sprintf(msg, "[HB-INIT] AUTO detectado como: %d\\n", requestedType);
        OutputDebugStringA(msg);
    }
    
    // Intentar inicializar el renderer solicitado
    bool success = false;
    
    if (requestedType == RENDERER_DIRECT3D11) {
        // Intentar inicializar D3D11
        OutputDebugStringA("[HB-INIT] Intentando inicializar Direct3D 11 (modo HIBRIDO)...\\n");
        
        if (CRendererConfig::IsD3D11Available()) {
            OutputDebugStringA("[HB-INIT] D3D11 disponible, creando renderer...\\n");
            m_pD3D11Renderer = new Direct3D11Renderer();
            if (m_pD3D11Renderer->Initialize(hWnd, config.GetScreenWidth(), config.GetScreenHeight(), 
                                        config.IsFullscreen())) {
                m_pActiveRenderer = m_pD3D11Renderer;
                m_bUsingD3D11 = true;
                success = true;
                
                // Aplicar configuración de VSync del archivo de configuración
                m_pD3D11Renderer->SetVSync(config.IsVsyncEnabled());
                
                // Inicializar el TextureManager para D3D11
                CTextureManager::GetInstance().Initialize(m_pD3D11Renderer);
                
                // Inicializar el SpriteRenderer para D3D11
                CSpriteRenderer::GetInstance().Initialize(m_pD3D11Renderer, m_pLegacyDDraw);
                CSpriteRenderer::GetInstance().SetUseD3D11(true);
                
                sprintf(msg, "[HB-INIT] *** D3D11 HIBRIDO INICIALIZADO OK *** %dx%d, VSync=%d\\n", 
                        config.GetScreenWidth(), config.GetScreenHeight(), config.IsVsyncEnabled());
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
    static int frameCount = 0;
    static bool bLoggedOnce = false;
    frameCount++;
    
    // Log cada 300 frames (aproximadamente cada 5 segundos a 60fps)
    bool bLogThisFrame = (frameCount % 300 == 0);
    
    if (m_bUsingD3D11 && m_pD3D11Renderer && m_pLegacyDDraw) {
        // MODO HÍBRIDO: Los sprites se dibujan en DirectDraw
        // Copiamos el backbuffer de DirectDraw a D3D11 y presentamos
        
        // Log primera vez
        if (!bLoggedOnce) {
            OutputDebugStringA("[HB-FLIP] Modo HIBRIDO activo (DDraw+D3D11)\n");
            bLoggedOnce = true;
        }
        
        // Verificar si D3D11 tiene device lost
        if (m_pD3D11Renderer->IsDeviceLost()) {
            if (bLogThisFrame) OutputDebugStringA("[HB-FLIP] D3D11 Device Lost - saltando frame\n");
            return S_OK;
        }
        
        // Obtener acceso al backbuffer de DirectDraw
        LPDIRECTDRAWSURFACE7 lpBackBuffer = m_pLegacyDDraw->m_lpBackB4;
        if (lpBackBuffer == NULL) {
            OutputDebugStringA("[HB-FLIP] ERROR: lpBackBuffer es NULL\n");
            return S_OK;
        }
        
        // Verificar si la superficie está perdida
        HRESULT hrTest = lpBackBuffer->IsLost();
        if (hrTest == DDERR_SURFACELOST) {
            OutputDebugStringA("[HB-FLIP] DDraw surface lost - restaurando...\n");
            HRESULT hrRestore = lpBackBuffer->Restore();
            char msg[128];
            sprintf(msg, "[HB-FLIP] BackBuffer Restore: 0x%08X\n", hrRestore);
            OutputDebugStringA(msg);
            
            if (m_pLegacyDDraw->m_lpFrontB4) {
                hrRestore = m_pLegacyDDraw->m_lpFrontB4->Restore();
                sprintf(msg, "[HB-FLIP] FrontBuffer Restore: 0x%08X\n", hrRestore);
                OutputDebugStringA(msg);
            }
            return S_OK;
        }
        
        DDSURFACEDESC2 ddsd;
        ZeroMemory(&ddsd, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        
        HRESULT hr = lpBackBuffer->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_READONLY, NULL);
        if (SUCCEEDED(hr)) {
            if (bLogThisFrame) {
                char msg[256];
                sprintf(msg, "[HB-FLIP] Frame %d: Lock OK, %dx%d, pitch=%d\n", 
                        frameCount, ddsd.dwWidth, ddsd.dwHeight, ddsd.lPitch);
                OutputDebugStringA(msg);
            }
            
            // Copiar backbuffer a D3D11 y presentar
            m_pD3D11Renderer->PresentDirectDrawBackBuffer(
                ddsd.lpSurface,
                ddsd.lPitch,
                ddsd.dwWidth,
                ddsd.dwHeight,
                0xFFFF
            );
            
            lpBackBuffer->Unlock(NULL);
            return S_OK;
        }
        else if (hr == DDERR_SURFACELOST) {
            OutputDebugStringA("[HB-FLIP] Surface lost durante Lock - restaurando...\n");
            lpBackBuffer->Restore();
            return S_OK;
        }
        else {
            char msg[128];
            sprintf(msg, "[HB-FLIP] ERROR: Lock fallo con 0x%08X\n", hr);
            OutputDebugStringA(msg);
            return S_OK;
        }
    }
    else if (m_pLegacyDDraw) {
        // DirectDraw usa iFlip()
        if (!bLoggedOnce) {
            OutputDebugStringA("[HB-FLIP] Modo DirectDraw puro\n");
            bLoggedOnce = true;
        }
        return m_pLegacyDDraw->iFlip();
    }
    
    OutputDebugStringA("[HB-FLIP] ERROR: Sin renderer activo\n");
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

void CRendererBridge::OnAppActivate(BOOL bActive)
{
    char msg[256];
    sprintf(msg, "[HB-ACTIVATE] OnAppActivate(%s) - UsingD3D11=%d, pD3D11=%p\n", 
            bActive ? "TRUE" : "FALSE", m_bUsingD3D11, m_pD3D11Renderer);
    OutputDebugStringA(msg);
    
    if (m_bUsingD3D11 && m_pD3D11Renderer) {
        // Notificar al renderer D3D11 sobre el cambio de foco
        m_pD3D11Renderer->OnAppActivate(bActive);
        
        if (bActive) {
            OutputDebugStringA("[HB-ACTIVATE] D3D11 notificado - APP ACTIVA\n");
        } else {
            OutputDebugStringA("[HB-ACTIVATE] D3D11 notificado - APP INACTIVA\n");
        }
    } else {
        OutputDebugStringA("[HB-ACTIVATE] No usando D3D11 o renderer NULL\n");
    }
}
