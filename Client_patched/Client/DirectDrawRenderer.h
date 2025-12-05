// DirectDrawRenderer.h - Implementación DirectDraw del IRenderer
// Wrapper del sistema de renderizado existente (DXC_ddraw)
// Mantiene compatibilidad total con el código actual

#ifndef DIRECTDRAWRENDERER_H
#define DIRECTDRAWRENDERER_H

#include "IRenderer.h"
#include "DXC_ddraw.h"

class CSprite;

class DirectDrawRenderer : public IRenderer {
private:
    DXC_ddraw* m_pDDraw;        // Instancia del DirectDraw original
    HWND m_hWnd;                // Handle de ventana
    int m_iWidth;               // Ancho del render
    int m_iHeight;              // Alto del render
    BOOL m_bFullscreen;         // Modo pantalla completa
    BOOL m_bInitialized;        // Estado de inicialización
    BOOL m_bVSync;              // VSync habilitado
    RECT m_rcClip;              // Región de clipping actual
    HFONT m_hCurrentFont;       // Fuente actual para texto
    
public:
    DirectDrawRenderer();
    virtual ~DirectDrawRenderer();
    
    // ===== INICIALIZACIÓN =====
    BOOL Initialize(HWND hWnd, int width, int height, BOOL fullscreen) override;
    void Shutdown() override;
    BOOL SetDisplayMode(int width, int height, BOOL fullscreen) override;
    
    // ===== FRAME MANAGEMENT =====
    void BeginFrame() override;
    HRESULT EndFrame() override;
    
    // ===== DRAWING PRIMITIVES =====
    void Clear(COLORREF color = RGB(0, 0, 0)) override;
    void FillRect(int x1, int y1, int x2, int y2, COLORREF color) override;
    void DrawRect(int x1, int y1, int x2, int y2, COLORREF color) override;
    void DrawLine(int x1, int y1, int x2, int y2, COLORREF color) override;
    void PutPixel(int x, int y, COLORREF color) override;
    
    // ===== TEXT RENDERING =====
    void DrawText(int x, int y, const char* text, COLORREF color) override;
    void DrawTextRect(RECT* rect, const char* text, COLORREF color, UINT format = DT_LEFT) override;
    void SetFont(HFONT hFont) override;
    
    // ===== SPRITE RENDERING =====
    void DrawSprite(int x, int y, CSprite* sprite, int frame, BOOL mirror = FALSE) override;
    void DrawSpriteAlpha(int x, int y, CSprite* sprite, int frame, float alpha, BOOL mirror = FALSE) override;
    void DrawSpriteBlend(int x, int y, CSprite* sprite, int frame, BlendMode mode, BOOL mirror = FALSE) override;
    void DrawSpriteScaled(int x, int y, CSprite* sprite, int frame, float scaleX, float scaleY, BOOL mirror = FALSE) override;
    void DrawSpriteTinted(int x, int y, CSprite* sprite, int frame, COLORREF tint, BOOL mirror = FALSE) override;
    
    // ===== SURFACE/TEXTURE OPERATIONS =====
    void* CreateOffscreenSurface(int width, int height) override;
    void DestroyOffscreenSurface(void* surface) override;
    void SetRenderTarget(void* surface) override;
    void Blit(void* srcSurface, RECT* srcRect, int dstX, int dstY) override;
    
    // ===== CLIPPING =====
    void SetClipRect(RECT* rect) override;
    void GetClipRect(RECT* rect) override;
    void ResetClipRect() override;
    
    // ===== SPECIAL EFFECTS =====
    void DrawShadowBox(int x1, int y1, int x2, int y2, int intensity = 0) override;
    void DrawItemShadowBox(int x1, int y1, int x2, int y2, int type = 0) override;
    
    // ===== STATE QUERIES =====
    int GetWidth() const override { return m_iWidth; }
    int GetHeight() const override { return m_iHeight; }
    BOOL IsFullscreen() const override { return m_bFullscreen; }
    BOOL IsInitialized() const override { return m_bInitialized; }
    
    // ===== SCREENSHOTS =====
    BOOL SaveScreenshot(const char* filename) override;
    
    // ===== VSYNC =====
    void SetVSync(BOOL enabled) override { m_bVSync = enabled; }
    BOOL GetVSync() const override { return m_bVSync; }
    
    // ===== ACCESO AL DDRAW ORIGINAL (para compatibilidad) =====
    DXC_ddraw* GetDDraw() { return m_pDDraw; }
    
    // ===== LEGACY SUPPORT =====
    // Permite usar un DXC_ddraw existente (ya inicializado)
    // Usado por RendererBridge para evitar doble inicialización
    void SetLegacyDDraw(DXC_ddraw* pDDraw) {
        m_pDDraw = pDDraw;
        if (pDDraw) {
            m_iWidth = pDDraw->res_x;
            m_iHeight = pDDraw->res_y;
            m_bFullscreen = pDDraw->m_bFullMode;
            m_bInitialized = pDDraw->m_init;
        }
    }
};

#endif // DIRECTDRAWRENDERER_H
