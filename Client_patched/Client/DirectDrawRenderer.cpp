// DirectDrawRenderer.cpp - Implementación DirectDraw del IRenderer
// Wrapper del sistema de renderizado existente (DXC_ddraw)

#include "DirectDrawRenderer.h"
#include "Sprite.h"

// Constructor
DirectDrawRenderer::DirectDrawRenderer()
    : m_pDDraw(NULL)
    , m_hWnd(NULL)
    , m_iWidth(800)
    , m_iHeight(600)
    , m_bFullscreen(FALSE)
    , m_bInitialized(FALSE)
    , m_bVSync(TRUE)
    , m_hCurrentFont(NULL)
{
    SetRect(&m_rcClip, 0, 0, 800, 600);
}

// Destructor
DirectDrawRenderer::~DirectDrawRenderer()
{
    Shutdown();
}

// ===== INICIALIZACIÓN =====

BOOL DirectDrawRenderer::Initialize(HWND hWnd, int width, int height, BOOL fullscreen)
{
    if (m_bInitialized) {
        Shutdown();
    }
    
    m_hWnd = hWnd;
    m_iWidth = width;
    m_iHeight = height;
    m_bFullscreen = fullscreen;
    
    // Crear instancia de DXC_ddraw
    m_pDDraw = new DXC_ddraw();
    if (m_pDDraw == NULL) {
        return FALSE;
    }
    
    // Configurar modo
    m_pDDraw->SetFullMode(fullscreen);
    
    // Inicializar DirectDraw
    if (!m_pDDraw->bInit(hWnd)) {
        delete m_pDDraw;
        m_pDDraw = NULL;
        return FALSE;
    }
    
    // Configurar clipping inicial
    SetRect(&m_rcClip, 0, 0, width, height);
    
    m_bInitialized = TRUE;
    return TRUE;
}

void DirectDrawRenderer::Shutdown()
{
    if (m_pDDraw != NULL) {
        delete m_pDDraw;
        m_pDDraw = NULL;
    }
    m_bInitialized = FALSE;
}

BOOL DirectDrawRenderer::SetDisplayMode(int width, int height, BOOL fullscreen)
{
    // Para cambiar el modo, necesitamos reinicializar
    if (m_bInitialized) {
        m_pDDraw->SetFullMode(fullscreen);
        m_pDDraw->ChangeDisplayMode(m_hWnd);
        m_iWidth = width;
        m_iHeight = height;
        m_bFullscreen = fullscreen;
        SetRect(&m_rcClip, 0, 0, width, height);
        return TRUE;
    }
    return FALSE;
}

// ===== FRAME MANAGEMENT =====

void DirectDrawRenderer::BeginFrame()
{
    if (!m_bInitialized || m_pDDraw == NULL) return;
    
    // Limpiar el backbuffer
    m_pDDraw->ClearBackB4();
}

HRESULT DirectDrawRenderer::EndFrame()
{
    if (!m_bInitialized || m_pDDraw == NULL) return E_FAIL;
    
    // Flip/Present
    return m_pDDraw->iFlip();
}

// ===== DRAWING PRIMITIVES =====

void DirectDrawRenderer::Clear(COLORREF color)
{
    if (!m_bInitialized || m_pDDraw == NULL) return;
    m_pDDraw->ClearBackB4();
}

void DirectDrawRenderer::FillRect(int x1, int y1, int x2, int y2, COLORREF color)
{
    if (!m_bInitialized || m_pDDraw == NULL) return;
    
    // Obtener DC y dibujar
    m_pDDraw->_GetBackBufferDC();
    if (m_pDDraw->m_hDC) {
        HBRUSH hBrush = CreateSolidBrush(color);
        RECT rc = {x1, y1, x2, y2};
        ::FillRect(m_pDDraw->m_hDC, &rc, hBrush);
        DeleteObject(hBrush);
    }
    m_pDDraw->_ReleaseBackBufferDC();
}

void DirectDrawRenderer::DrawRect(int x1, int y1, int x2, int y2, COLORREF color)
{
    if (!m_bInitialized || m_pDDraw == NULL) return;
    
    m_pDDraw->_GetBackBufferDC();
    if (m_pDDraw->m_hDC) {
        HPEN hPen = CreatePen(PS_SOLID, 1, color);
        HPEN hOldPen = (HPEN)SelectObject(m_pDDraw->m_hDC, hPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(m_pDDraw->m_hDC, GetStockObject(NULL_BRUSH));
        
        Rectangle(m_pDDraw->m_hDC, x1, y1, x2, y2);
        
        SelectObject(m_pDDraw->m_hDC, hOldPen);
        SelectObject(m_pDDraw->m_hDC, hOldBrush);
        DeleteObject(hPen);
    }
    m_pDDraw->_ReleaseBackBufferDC();
}

void DirectDrawRenderer::DrawLine(int x1, int y1, int x2, int y2, COLORREF color)
{
    if (!m_bInitialized || m_pDDraw == NULL) return;
    
    m_pDDraw->_GetBackBufferDC();
    if (m_pDDraw->m_hDC) {
        HPEN hPen = CreatePen(PS_SOLID, 1, color);
        HPEN hOldPen = (HPEN)SelectObject(m_pDDraw->m_hDC, hPen);
        
        MoveToEx(m_pDDraw->m_hDC, x1, y1, NULL);
        LineTo(m_pDDraw->m_hDC, x2, y2);
        
        SelectObject(m_pDDraw->m_hDC, hOldPen);
        DeleteObject(hPen);
    }
    m_pDDraw->_ReleaseBackBufferDC();
}

void DirectDrawRenderer::PutPixel(int x, int y, COLORREF color)
{
    if (!m_bInitialized || m_pDDraw == NULL) return;
    
    BYTE r = GetRValue(color);
    BYTE g = GetGValue(color);
    BYTE b = GetBValue(color);
    
    // Convertir a 565 RGB (16-bit)
    WORD wR = (r >> 3) & 0x1F;
    WORD wG = (g >> 2) & 0x3F;
    WORD wB = (b >> 3) & 0x1F;
    
    m_pDDraw->PutPixel((short)x, (short)y, wR, wG, wB);
}

// ===== TEXT RENDERING =====

void DirectDrawRenderer::DrawText(int x, int y, const char* text, COLORREF color)
{
    if (!m_bInitialized || m_pDDraw == NULL || text == NULL) return;
    
    m_pDDraw->TextOut(x, y, (char*)text, color);
}

void DirectDrawRenderer::DrawTextRect(RECT* rect, const char* text, COLORREF color, UINT format)
{
    if (!m_bInitialized || m_pDDraw == NULL || rect == NULL || text == NULL) return;
    
    m_pDDraw->DrawText(rect, text, color);
}

void DirectDrawRenderer::SetFont(HFONT hFont)
{
    m_hCurrentFont = hFont;
    // El DXC_ddraw tiene su propia fuente interna
}

// ===== SPRITE RENDERING =====

void DirectDrawRenderer::DrawSprite(int x, int y, CSprite* sprite, int frame, BOOL mirror)
{
    if (!m_bInitialized || m_pDDraw == NULL || sprite == NULL) return;
    
    // El sprite tiene su propio método de dibujo que usa el DXC_ddraw
    // Esta función será llamada desde el código del juego que ya tiene 
    // la lógica de sprite rendering integrada
    
    // Por ahora, esta es una interfaz placeholder - el juego actual
    // llama directamente a los métodos del sprite
}

void DirectDrawRenderer::DrawSpriteAlpha(int x, int y, CSprite* sprite, int frame, float alpha, BOOL mirror)
{
    if (!m_bInitialized || m_pDDraw == NULL || sprite == NULL) return;
    
    // El sistema actual usa tablas de lookup para alpha blending
    // Ver m_lTransG50, m_lTransRB50, etc. en DXC_ddraw
    
    // Por ahora placeholder
}

void DirectDrawRenderer::DrawSpriteBlend(int x, int y, CSprite* sprite, int frame, BlendMode mode, BOOL mirror)
{
    if (!m_bInitialized || sprite == NULL) return;
    
    // Placeholder para blend modes
    DrawSprite(x, y, sprite, frame, mirror);
}

void DirectDrawRenderer::DrawSpriteScaled(int x, int y, CSprite* sprite, int frame, float scaleX, float scaleY, BOOL mirror)
{
    if (!m_bInitialized || sprite == NULL) return;
    
    // El sistema DirectDraw no tiene scaling nativo eficiente
    // Esto será implementado propiamente en Direct3D11
    DrawSprite(x, y, sprite, frame, mirror);
}

void DirectDrawRenderer::DrawSpriteTinted(int x, int y, CSprite* sprite, int frame, COLORREF tint, BOOL mirror)
{
    if (!m_bInitialized || sprite == NULL) return;
    
    // Placeholder - tinting requiere procesamiento por pixel
    DrawSprite(x, y, sprite, frame, mirror);
}

// ===== SURFACE/TEXTURE OPERATIONS =====

void* DirectDrawRenderer::CreateOffscreenSurface(int width, int height)
{
    if (!m_bInitialized || m_pDDraw == NULL) return NULL;
    
    return (void*)m_pDDraw->pCreateOffScreenSurface((WORD)width, (WORD)height);
}

void DirectDrawRenderer::DestroyOffscreenSurface(void* surface)
{
    if (surface != NULL) {
        IDirectDrawSurface7* pSurface = (IDirectDrawSurface7*)surface;
        pSurface->Release();
    }
}

void DirectDrawRenderer::SetRenderTarget(void* surface)
{
    // DirectDraw no tiene concepto de render target intercambiable
    // como Direct3D. El backbuffer es siempre el destino.
    // Esta función es un placeholder para compatibilidad.
}

void DirectDrawRenderer::Blit(void* srcSurface, RECT* srcRect, int dstX, int dstY)
{
    if (!m_bInitialized || m_pDDraw == NULL || srcSurface == NULL) return;
    
    IDirectDrawSurface7* pSrc = (IDirectDrawSurface7*)srcSurface;
    
    RECT dstRect;
    if (srcRect != NULL) {
        int width = srcRect->right - srcRect->left;
        int height = srcRect->bottom - srcRect->top;
        SetRect(&dstRect, dstX, dstY, dstX + width, dstY + height);
    } else {
        // Asumir tamaño completo
        DDSURFACEDESC2 desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.dwSize = sizeof(desc);
        pSrc->GetSurfaceDesc(&desc);
        SetRect(&dstRect, dstX, dstY, dstX + (int)desc.dwWidth, dstY + (int)desc.dwHeight);
    }
    
    m_pDDraw->m_lpBackB4->Blt(&dstRect, pSrc, srcRect, DDBLT_WAIT, NULL);
}

// ===== CLIPPING =====

void DirectDrawRenderer::SetClipRect(RECT* rect)
{
    if (rect != NULL) {
        CopyRect(&m_rcClip, rect);
        if (m_pDDraw != NULL) {
            CopyRect(&m_pDDraw->m_rcClipArea, rect);
        }
    }
}

void DirectDrawRenderer::GetClipRect(RECT* rect)
{
    if (rect != NULL) {
        CopyRect(rect, &m_rcClip);
    }
}

void DirectDrawRenderer::ResetClipRect()
{
    SetRect(&m_rcClip, 0, 0, m_iWidth, m_iHeight);
    if (m_pDDraw != NULL) {
        SetRect(&m_pDDraw->m_rcClipArea, 0, 0, m_iWidth, m_iHeight);
    }
}

// ===== SPECIAL EFFECTS =====

void DirectDrawRenderer::DrawShadowBox(int x1, int y1, int x2, int y2, int intensity)
{
    if (!m_bInitialized || m_pDDraw == NULL) return;
    
    m_pDDraw->DrawShadowBox((short)x1, (short)y1, (short)x2, (short)y2, intensity);
}

void DirectDrawRenderer::DrawItemShadowBox(int x1, int y1, int x2, int y2, int type)
{
    if (!m_bInitialized || m_pDDraw == NULL) return;
    
    m_pDDraw->DrawItemShadowBox((short)x1, (short)y1, (short)x2, (short)y2, type);
}

// ===== SCREENSHOTS =====

BOOL DirectDrawRenderer::SaveScreenshot(const char* filename)
{
    if (!m_bInitialized || m_pDDraw == NULL || filename == NULL) return FALSE;
    
    return m_pDDraw->Screenshot(filename, m_pDDraw->m_lpBackB4);
}

// ===== FACTORY FUNCTION =====

// Include para D3D11 renderer
#include "Direct3D11Renderer.h"

IRenderer* CreateRenderer(int rendererType)
{
    // rendererType:
    // 0 = Auto-detect (intenta D3D11, fallback a DirectDraw)
    // 1 = DirectDraw (legacy) - Forzar compatibilidad
    // 2 = Direct3D11 (moderno) - Forzar D3D11
    
    switch (rendererType) {
        case 1: // DirectDraw (legacy)
            return new DirectDrawRenderer();
            
        case 2: // Direct3D11 (moderno)
            return new Direct3D11Renderer();
            
        case 0: // Auto-detect
        default:
            // Intentar D3D11 primero
            {
                Direct3D11Renderer* d3d11 = new Direct3D11Renderer();
                // Verificamos si D3D11 está disponible intentando crear el device
                // Si falla, usamos DirectDraw
                // Por ahora, defaulteamos a DirectDraw para seguridad
                // hasta que D3D11 esté completamente testeado
                delete d3d11;
                return new DirectDrawRenderer();
            }
    }
}

void DestroyRenderer(IRenderer* renderer)
{
    if (renderer != NULL) {
        renderer->Shutdown();
        delete renderer;
    }
}
