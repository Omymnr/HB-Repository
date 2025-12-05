// SpriteRenderer.h - Sistema de renderizado de sprites para D3D11
// Este módulo intercepta las llamadas de dibujo de sprites y las
// redirige al renderer apropiado (DirectDraw o Direct3D11)
//
// USO:
// En lugar de: m_pSprite[id]->PutSpriteFast(x, y, frame, time);
// Usar:        g_SpriteRenderer.DrawSprite(m_pSprite[id], x, y, frame, time);
//
// O simplemente activar el modo de intercepción automática.

#ifndef SPRITE_RENDERER_H
#define SPRITE_RENDERER_H

#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

// Forward declarations
class CSprite;
class Direct3D11Renderer;
class DXC_ddraw;
struct SpriteFrameTexture;

// Modos de blending para sprites
enum SpriteBlendMode {
    SPRITE_BLEND_NORMAL,        // Dibujo normal con color key
    SPRITE_BLEND_ALPHA,         // Alpha blending (transparencia)
    SPRITE_BLEND_ALPHA_25,      // 25% alpha
    SPRITE_BLEND_ALPHA_50,      // 50% alpha
    SPRITE_BLEND_ALPHA_70,      // 70% alpha
    SPRITE_BLEND_ADDITIVE,      // Blending aditivo (para efectos de luz)
    SPRITE_BLEND_SHADOW,        // Para sombras
    SPRITE_BLEND_RGB_TINT       // Con tinte de color
};

// Información de un sprite en el batch
struct SpriteBatchItem {
    CSprite* pSprite;
    int x, y;
    int frame;
    SpriteBlendMode blendMode;
    float alpha;
    DWORD tintColor;    // RGB tint (0xFFFFFFFF = sin tinte)
    bool mirror;
};

// Clase principal de renderizado de sprites
class CSpriteRenderer {
public:
    static CSpriteRenderer& GetInstance() {
        static CSpriteRenderer instance;
        return instance;
    }
    
    // Inicialización
    bool Initialize(Direct3D11Renderer* pD3D11, DXC_ddraw* pDDraw);
    void Shutdown();
    
    // Seleccionar renderer activo
    void SetUseD3D11(bool useD3D11) { m_bUseD3D11 = useD3D11; }
    bool IsUsingD3D11() const { return m_bUseD3D11; }
    
    // ========================================
    // FUNCIONES DE DIBUJO - Equivalentes a CSprite
    // ========================================
    
    // Básico - Equivalente a PutSpriteFast
    void DrawSprite(CSprite* pSprite, int x, int y, int frame, DWORD dwTime);
    
    // Con transparencia - Equivalente a PutTransSprite
    void DrawSpriteAlpha(CSprite* pSprite, int x, int y, int frame, DWORD dwTime, float alpha = 0.3f);
    
    // Transparencias fijas
    void DrawSprite25(CSprite* pSprite, int x, int y, int frame, DWORD dwTime);
    void DrawSprite50(CSprite* pSprite, int x, int y, int frame, DWORD dwTime);
    void DrawSprite70(CSprite* pSprite, int x, int y, int frame, DWORD dwTime);
    
    // Con tinte RGB - Equivalente a PutSpriteRGB/PutTransSpriteRGB
    void DrawSpriteRGB(CSprite* pSprite, int x, int y, int frame, 
                       int red, int green, int blue, DWORD dwTime);
    void DrawSpriteRGBAlpha(CSprite* pSprite, int x, int y, int frame,
                            int red, int green, int blue, DWORD dwTime, float alpha = 0.3f);
    
    // Sombras - Equivalente a PutShadowSprite
    void DrawSpriteShadow(CSprite* pSprite, int x, int y, int frame, DWORD dwTime);
    
    // Escalado - Equivalente a PutSpriteScaled
    void DrawSpriteScaled(CSprite* pSprite, int x, int y, int frame,
                          int width, int height, DWORD dwTime);
    
    // Con offset - Equivalente a PutShiftSpriteFast
    void DrawSpriteShift(CSprite* pSprite, int x, int y, int shiftX, int shiftY,
                         int frame, DWORD dwTime);
    
    // ========================================
    // SISTEMA DE BATCHING (para mejor rendimiento)
    // ========================================
    
    // Comenzar/finalizar batch
    void BeginBatch();
    void EndBatch();
    
    // Agregar sprite al batch actual
    void BatchSprite(CSprite* pSprite, int x, int y, int frame,
                     SpriteBlendMode mode = SPRITE_BLEND_NORMAL,
                     float alpha = 1.0f, DWORD tint = 0xFFFFFFFF);
    
    // Flush del batch (dibujar todo lo acumulado)
    void FlushBatch();
    
    // ========================================
    // FRAME MANAGEMENT
    // ========================================
    
    void BeginFrame();
    void EndFrame();
    
    // ========================================
    // ESTADÍSTICAS
    // ========================================
    
    int GetSpritesDrawnThisFrame() const { return m_iSpritesThisFrame; }
    int GetBatchCount() const { return m_iBatchCount; }
    void ResetStats() { m_iSpritesThisFrame = 0; m_iBatchCount = 0; }
    
private:
    CSpriteRenderer();
    ~CSpriteRenderer();
    CSpriteRenderer(const CSpriteRenderer&) = delete;
    CSpriteRenderer& operator=(const CSpriteRenderer&) = delete;
    
    // Dibujar un sprite con D3D11
    void DrawSpriteD3D11(CSprite* pSprite, int x, int y, int frame,
                         SpriteBlendMode mode, float alpha, DWORD tint);
    
    // Dibujar usando el batch de D3D11
    void DrawBatchD3D11();
    
    // Renderers
    Direct3D11Renderer* m_pD3D11;
    DXC_ddraw* m_pDDraw;
    
    // Estado
    bool m_bInitialized;
    bool m_bUseD3D11;
    bool m_bInBatch;
    
    // Batch de sprites
    std::vector<SpriteBatchItem> m_SpriteBatch;
    static const int MAX_BATCH_SIZE = 1000;
    
    // Estadísticas
    int m_iSpritesThisFrame;
    int m_iBatchCount;
};

// Macro de conveniencia
#define g_SpriteRenderer CSpriteRenderer::GetInstance()

#endif // SPRITE_RENDERER_H
