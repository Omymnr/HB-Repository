// IRenderer.h - Interfaz abstracta de renderizado
// Parte del sistema de modernización de Helbreath Client
//
// Esta interfaz permite cambiar entre DirectDraw (legacy) y Direct3D11 (moderno)
// sin modificar el código del juego.

#ifndef IRENDERER_H
#define IRENDERER_H

#include <windows.h>

// Forward declarations
class CSprite;

// Tipos de blend para sprites
enum class BlendMode {
    None,           // Sin blending (opaco)
    Alpha,          // Alpha blending normal
    Additive,       // Blending aditivo (para luces/efectos)
    Multiply        // Blending multiplicativo (para sombras)
};

// Interfaz abstracta del renderer
class IRenderer {
public:
    virtual ~IRenderer() {}
    
    // ===== INICIALIZACIÓN =====
    
    // Inicializar el renderer
    // hWnd: Handle de la ventana
    // width, height: Resolución del juego (normalmente 800x600)
    // fullscreen: TRUE para pantalla completa, FALSE para ventana
    virtual BOOL Initialize(HWND hWnd, int width, int height, BOOL fullscreen) = 0;
    
    // Liberar recursos
    virtual void Shutdown() = 0;
    
    // Cambiar modo de pantalla
    virtual BOOL SetDisplayMode(int width, int height, BOOL fullscreen) = 0;
    
    // ===== FRAME MANAGEMENT =====
    
    // Comenzar un nuevo frame (limpiar buffers, etc.)
    virtual void BeginFrame() = 0;
    
    // Finalizar frame y presentar en pantalla
    virtual HRESULT EndFrame() = 0;
    
    // ===== DRAWING PRIMITIVES =====
    
    // Limpiar el backbuffer con un color
    virtual void Clear(COLORREF color = RGB(0, 0, 0)) = 0;
    
    // Dibujar un rectángulo relleno
    virtual void FillRect(int x1, int y1, int x2, int y2, COLORREF color) = 0;
    
    // Dibujar un rectángulo con borde
    virtual void DrawRect(int x1, int y1, int x2, int y2, COLORREF color) = 0;
    
    // Dibujar una línea
    virtual void DrawLine(int x1, int y1, int x2, int y2, COLORREF color) = 0;
    
    // Dibujar un pixel
    virtual void PutPixel(int x, int y, COLORREF color) = 0;
    
    // ===== TEXT RENDERING =====
    
    // Dibujar texto
    virtual void DrawText(int x, int y, const char* text, COLORREF color) = 0;
    
    // Dibujar texto en un rectángulo con alineación
    virtual void DrawTextRect(RECT* rect, const char* text, COLORREF color, UINT format = DT_LEFT) = 0;
    
    // Establecer fuente actual
    virtual void SetFont(HFONT hFont) = 0;
    
    // ===== SPRITE RENDERING =====
    
    // Dibujar un sprite normal
    virtual void DrawSprite(int x, int y, CSprite* sprite, int frame, BOOL mirror = FALSE) = 0;
    
    // Dibujar un sprite con transparencia (alpha 0.0-1.0)
    virtual void DrawSpriteAlpha(int x, int y, CSprite* sprite, int frame, float alpha, BOOL mirror = FALSE) = 0;
    
    // Dibujar un sprite con blend mode específico
    virtual void DrawSpriteBlend(int x, int y, CSprite* sprite, int frame, BlendMode mode, BOOL mirror = FALSE) = 0;
    
    // Dibujar un sprite escalado
    virtual void DrawSpriteScaled(int x, int y, CSprite* sprite, int frame, float scaleX, float scaleY, BOOL mirror = FALSE) = 0;
    
    // Dibujar un sprite con tinte de color
    virtual void DrawSpriteTinted(int x, int y, CSprite* sprite, int frame, COLORREF tint, BOOL mirror = FALSE) = 0;
    
    // ===== SURFACE/TEXTURE OPERATIONS =====
    
    // Crear una superficie off-screen (para pre-rendering)
    virtual void* CreateOffscreenSurface(int width, int height) = 0;
    
    // Destruir una superficie off-screen
    virtual void DestroyOffscreenSurface(void* surface) = 0;
    
    // Establecer superficie de destino (NULL para backbuffer)
    virtual void SetRenderTarget(void* surface) = 0;
    
    // Copiar de una superficie a otra (o al backbuffer)
    virtual void Blit(void* srcSurface, RECT* srcRect, int dstX, int dstY) = 0;
    
    // ===== CLIPPING =====
    
    // Establecer región de clipping
    virtual void SetClipRect(RECT* rect) = 0;
    
    // Obtener región de clipping actual
    virtual void GetClipRect(RECT* rect) = 0;
    
    // Resetear clipping a toda la pantalla
    virtual void ResetClipRect() = 0;
    
    // ===== SPECIAL EFFECTS (para compatibilidad con el juego actual) =====
    
    // Dibujar caja de sombra (efecto de oscurecimiento)
    virtual void DrawShadowBox(int x1, int y1, int x2, int y2, int intensity = 0) = 0;
    
    // Dibujar caja de item (efecto especial para items)
    virtual void DrawItemShadowBox(int x1, int y1, int x2, int y2, int type = 0) = 0;
    
    // ===== STATE QUERIES =====
    
    // Obtener dimensiones del render target actual
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    
    // Verificar si está en modo pantalla completa
    virtual BOOL IsFullscreen() const = 0;
    
    // Verificar si el renderer está inicializado correctamente
    virtual BOOL IsInitialized() const = 0;
    
    // ===== SCREENSHOTS =====
    
    // Capturar screenshot
    virtual BOOL SaveScreenshot(const char* filename) = 0;
    
    // ===== VSYNC =====
    
    // Habilitar/deshabilitar VSync
    virtual void SetVSync(BOOL enabled) = 0;
    virtual BOOL GetVSync() const = 0;
};

// Factory function para crear el renderer apropiado
// rendererType: 0 = Auto-detect, 1 = DirectDraw (legacy), 2 = Direct3D11
IRenderer* CreateRenderer(int rendererType = 0);

// Destruir el renderer
void DestroyRenderer(IRenderer* renderer);

#endif // IRENDERER_H
