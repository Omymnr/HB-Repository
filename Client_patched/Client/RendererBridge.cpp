// RendererBridge.cpp
// Implementación del puente entre DXC_ddraw y el nuevo sistema de renderizado

#include "RendererBridge.h"
#include "DirectDrawRenderer.h"
#include "Direct3D11Renderer.h"
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
        // NOTA: D3D11 está implementado pero los sprites aún usan DirectDraw
        // Por ahora, D3D11 solo se puede usar cuando todos los sprites estén migrados
        // Forzamos DirectDraw hasta que la migración esté completa
        
        OutputDebugStringA("RendererBridge: D3D11 seleccionado pero sprites usan DirectDraw\\n");
        OutputDebugStringA("RendererBridge: Usando DirectDraw hasta completar migración de sprites\\n");
        
        // Verificar que D3D11 está disponible (para futuro uso)
        if (CRendererConfig::IsD3D11Available()) {
            OutputDebugStringA("RendererBridge: D3D11 está disponible en este sistema\\n");
        }
        
        // Por ahora, forzar DirectDraw
        requestedType = RENDERER_DIRECTDRAW;
        
        /* 
        // CÓDIGO D3D11 - Descomentar cuando los sprites estén migrados:
        m_pD3D11Renderer = new Direct3D11Renderer();
        if (m_pD3D11Renderer->Initialize(hWnd, config.GetScreenWidth(), config.GetScreenHeight(), 
                                    config.IsFullscreen())) {
            m_pActiveRenderer = m_pD3D11Renderer;
            m_bUsingD3D11 = true;
            success = true;
            
            char msg[256];
            sprintf(msg, "RendererBridge: Inicializado Direct3D 11 (%dx%d)\\n", 
                    config.GetScreenWidth(), config.GetScreenHeight());
            OutputDebugStringA(msg);
        }
        else {
            delete m_pD3D11Renderer;
            m_pD3D11Renderer = nullptr;
            requestedType = RENDERER_DIRECTDRAW;
            OutputDebugStringA("RendererBridge: D3D11 falló, usando DirectDraw\\n");
        }
        */
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
        
        OutputDebugStringA("RendererBridge: Usando DirectDraw (legacy)\n");
    }
    
    if (success) {
        m_bInitialized = true;
    }
    
    return success;
}

void CRendererBridge::Shutdown()
{
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
        }
        
        m_pActiveRenderer = m_pD3D11Renderer;
        m_bUsingD3D11 = true;
        
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
}

void CRendererBridge::EndFrame()
{
    if (m_pActiveRenderer) {
        m_pActiveRenderer->EndFrame();
    }
}

HRESULT CRendererBridge::Flip()
{
    if (m_bUsingD3D11 && m_pD3D11Renderer) {
        // D3D11 usa Present() en EndFrame, así que aquí no hacemos nada especial
        return S_OK;
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
    // Por ahora, siempre usamos el renderer legacy para sprites
    // En fase 2, convertiremos esto para usar D3D11
    
    // TODO: Implementar conversión de CSprite a textura D3D11
    // CSprite* sprite = (CSprite*)pSprite;
    // if (m_bUsingD3D11) {
    //     TextureHandle tex = ConvertSpriteToTexture(sprite, frame);
    //     m_pD3D11Renderer->DrawSprite(tex, x, y, ...);
    // }
    
    // Por ahora, los sprites siguen dibujándose con DDraw
    // El código de Game.cpp llama directamente a CSprite::DrawSprite()
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
