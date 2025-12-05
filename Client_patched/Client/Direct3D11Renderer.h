// Direct3D11Renderer.h - Implementación Direct3D 11 del IRenderer
// Renderer moderno para Windows 10/11 con soporte completo de GPU
//
// Ventajas sobre DirectDraw:
// - Hardware acceleration nativa
// - Soporte para cualquier resolución
// - Alpha blending nativo (32-bit color)
// - VSync y triple buffering
// - Shaders para efectos especiales
// - Compatible con todas las GPUs modernas

#ifndef DIRECT3D11RENDERER_H
#define DIRECT3D11RENDERER_H

#include "IRenderer.h"

// Direct3D 11 includes
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// DirectWrite for text (mejor que GDI)
#include <dwrite.h>
#include <d2d1.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d2d1.lib")

#include <map>
#include <vector>

using namespace DirectX;

class CSprite;

// Estructura de vértice para sprites
struct SpriteVertex {
    XMFLOAT3 position;  // x, y, z
    XMFLOAT2 texCoord;  // u, v
    XMFLOAT4 color;     // r, g, b, a (para tinting)
};

// Estructura para una textura cacheada
struct CachedTexture {
    ID3D11Texture2D* pTexture;
    ID3D11ShaderResourceView* pSRV;
    int width;
    int height;
};

// Constantes del shader
struct ShaderConstants {
    XMMATRIX projection;    // Matriz de proyección ortográfica
    XMFLOAT4 globalTint;    // Tinte global
    float alpha;            // Alpha global
    float padding[3];       // Padding para alineación de 16 bytes
};

class Direct3D11Renderer : public IRenderer {
private:
    // ===== DISPOSITIVO D3D11 =====
    ID3D11Device* m_pDevice;
    ID3D11DeviceContext* m_pContext;
    IDXGISwapChain* m_pSwapChain;
    ID3D11RenderTargetView* m_pRenderTargetView;
    ID3D11DepthStencilView* m_pDepthStencilView;
    ID3D11Texture2D* m_pDepthStencil;
    
    // ===== RECURSOS DE RENDERING =====
    ID3D11Buffer* m_pVertexBuffer;
    ID3D11Buffer* m_pIndexBuffer;
    ID3D11Buffer* m_pConstantBuffer;
    ID3D11InputLayout* m_pInputLayout;
    ID3D11VertexShader* m_pVertexShader;
    ID3D11PixelShader* m_pPixelShader;
    ID3D11SamplerState* m_pSamplerPoint;      // Para sprites pixel-art
    ID3D11SamplerState* m_pSamplerLinear;     // Para UI escalada
    
    // ===== BLEND STATES =====
    ID3D11BlendState* m_pBlendStateAlpha;     // Alpha blending normal
    ID3D11BlendState* m_pBlendStateAdditive;  // Aditivo (para efectos de luz)
    ID3D11BlendState* m_pBlendStateMultiply;  // Multiplicativo (para sombras)
    ID3D11BlendState* m_pBlendStateOpaque;    // Sin blending
    
    // ===== RASTERIZER STATE =====
    ID3D11RasterizerState* m_pRasterizerState;
    
    // ===== DIRECT2D/DIRECTWRITE (para texto) =====
    ID2D1Factory* m_pD2DFactory;
    ID2D1RenderTarget* m_pD2DRenderTarget;
    IDWriteFactory* m_pDWriteFactory;
    IDWriteTextFormat* m_pTextFormat;
    ID2D1SolidColorBrush* m_pTextBrush;
    
    // ===== CACHE DE TEXTURAS =====
    std::map<int, CachedTexture> m_TextureCache;    // Cache por sprite ID
    std::map<void*, CachedTexture> m_SurfaceCache;  // Cache para superficies off-screen
    
    // ===== ESTADO =====
    HWND m_hWnd;
    int m_iWidth;
    int m_iHeight;
    int m_iGameWidth;       // Resolución del juego (800)
    int m_iGameHeight;      // Resolución del juego (600)
    float m_fScaleX;        // Factor de escala X
    float m_fScaleY;        // Factor de escala Y
    BOOL m_bFullscreen;
    BOOL m_bInitialized;
    BOOL m_bVSync;
    RECT m_rcClip;
    D3D11_VIEWPORT m_Viewport;
    
    // ===== BATCH DE SPRITES =====
    std::vector<SpriteVertex> m_SpriteBatch;
    ID3D11ShaderResourceView* m_pCurrentTexture;
    BlendMode m_CurrentBlendMode;
    static const int MAX_BATCH_SIZE = 1000;
    
    // ===== MÉTODOS PRIVADOS =====
    BOOL CreateDevice();
    BOOL CreateSwapChain();
    BOOL CreateRenderTarget();
    BOOL CreateDepthStencil();
    BOOL CreateShaders();
    BOOL CreateBuffers();
    BOOL CreateBlendStates();
    BOOL CreateSamplers();
    BOOL CreateRasterizerState();
    BOOL InitDirect2D();
    
    void FlushBatch();
    ID3D11ShaderResourceView* GetSpriteTexture(CSprite* sprite, int frame);
    CachedTexture ConvertSpriteToTexture(CSprite* sprite, int frame);
    void UpdateProjectionMatrix();
    
public:
    Direct3D11Renderer();
    virtual ~Direct3D11Renderer();
    
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
    
    // ===== WINDOW RESIZE =====
    void OnResize(int width, int height);
    
    // ===== MÉTODOS ESPECÍFICOS D3D11 =====
    
    // Dibujar textura directamente (usado por SpriteRenderer)
    void DrawTexturedQuad(ID3D11ShaderResourceView* pSRV, 
                          int x, int y, int width, int height,
                          float alpha = 1.0f, 
                          float r = 1.0f, float g = 1.0f, float b = 1.0f,
                          BlendMode blend = BlendMode::Alpha);
    
    // Convertir coordenadas de juego a pantalla
    void GameToScreen(int& x, int& y);
    
    // Convertir coordenadas de pantalla a juego (para mouse)
    void ScreenToGame(int& x, int& y);
    
    // Obtener device D3D11 (para uso avanzado)
    ID3D11Device* GetDevice() { return m_pDevice; }
    ID3D11DeviceContext* GetContext() { return m_pContext; }
    
    // ===== MODO HÍBRIDO (DirectDraw + D3D11) =====
    // Permite usar DirectDraw para renderizar sprites mientras D3D11 presenta
    
    // Copiar backbuffer de DirectDraw a textura D3D11 y presentar
    void PresentDirectDrawBackBuffer(void* pBackBufferData, int pitch, int width, int height, WORD colorKey);
    
    // Crear textura para el backbuffer híbrido
    BOOL CreateHybridBackBuffer(int width, int height);
    
    // Actualizar textura del backbuffer desde DirectDraw
    void UpdateHybridBackBuffer(WORD* pPixels, int pitch, int width, int height, WORD colorKey);
    
private:
    // Textura para modo híbrido (backbuffer de DirectDraw)
    ID3D11Texture2D* m_pHybridTexture;
    ID3D11ShaderResourceView* m_pHybridSRV;
    BOOL m_bHybridMode;
};

#endif // DIRECT3D11RENDERER_H
