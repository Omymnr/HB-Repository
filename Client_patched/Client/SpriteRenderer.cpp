// SpriteRenderer.cpp - Implementación del sistema de renderizado de sprites
// Permite dibujar sprites usando DirectDraw (legacy) o Direct3D11 (moderno)

#include "SpriteRenderer.h"
#include "Direct3D11Renderer.h"
#include "TextureManager.h"
#include "Sprite.h"
#include "DXC_ddraw.h"
#include <stdio.h>

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

CSpriteRenderer::CSpriteRenderer()
    : m_pD3D11(NULL)
    , m_pDDraw(NULL)
    , m_bInitialized(false)
    , m_bUseD3D11(false)
    , m_bInBatch(false)
    , m_iSpritesThisFrame(0)
    , m_iBatchCount(0)
{
    m_SpriteBatch.reserve(MAX_BATCH_SIZE);
}

CSpriteRenderer::~CSpriteRenderer()
{
    Shutdown();
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================

bool CSpriteRenderer::Initialize(Direct3D11Renderer* pD3D11, DXC_ddraw* pDDraw)
{
    if (m_bInitialized) {
        return true;
    }
    
    m_pDDraw = pDDraw;
    m_pD3D11 = pD3D11;
    
    // Si tenemos D3D11, inicializar el TextureManager
    if (m_pD3D11 != NULL) {
        if (!g_TextureManager.Initialize(m_pD3D11)) {
            OutputDebugStringA("SpriteRenderer: Warning - TextureManager no pudo inicializarse\n");
        }
    }
    
    m_bInitialized = true;
    m_bUseD3D11 = (m_pD3D11 != NULL);
    
    char msg[128];
    sprintf(msg, "SpriteRenderer: Inicializado (D3D11: %s)\n", m_bUseD3D11 ? "SI" : "NO");
    OutputDebugStringA(msg);
    
    return true;
}

void CSpriteRenderer::Shutdown()
{
    if (!m_bInitialized) return;
    
    m_SpriteBatch.clear();
    g_TextureManager.Shutdown();
    
    m_pD3D11 = NULL;
    m_pDDraw = NULL;
    m_bInitialized = false;
    
    OutputDebugStringA("SpriteRenderer: Shutdown completado\n");
}

// ============================================================================
// FRAME MANAGEMENT
// ============================================================================

void CSpriteRenderer::BeginFrame()
{
    ResetStats();
    
    // Nota: No llamamos m_pD3D11->BeginFrame() aquí porque
    // RendererBridge ya lo hace. Solo reseteamos estadísticas.
}

void CSpriteRenderer::EndFrame()
{
    // Asegurar que el batch está vacío
    if (m_bInBatch) {
        EndBatch();
    }
    
    // Nota: No llamamos m_pD3D11->EndFrame() aquí porque
    // RendererBridge ya lo hace.
    
    // Limpiar texturas no usadas periódicamente
    static DWORD lastCleanup = 0;
    DWORD now = GetTickCount();
    if (now - lastCleanup > 60000) {  // Cada minuto
        g_TextureManager.CleanupUnusedTextures(60000);
        lastCleanup = now;
    }
}

// ============================================================================
// FUNCIONES DE DIBUJO PRINCIPALES
// ============================================================================

void CSpriteRenderer::DrawSprite(CSprite* pSprite, int x, int y, int frame, DWORD dwTime)
{
    if (pSprite == NULL) return;
    
    if (m_bUseD3D11 && m_pD3D11) {
        DrawSpriteD3D11(pSprite, x, y, frame, SPRITE_BLEND_NORMAL, 1.0f, 0xFFFFFFFF);
    } else {
        // Fallback a DirectDraw
        pSprite->PutSpriteFast(x, y, frame, dwTime);
    }
    
    m_iSpritesThisFrame++;
}

void CSpriteRenderer::DrawSpriteAlpha(CSprite* pSprite, int x, int y, int frame, DWORD dwTime, float alpha)
{
    if (pSprite == NULL) return;
    
    if (m_bUseD3D11 && m_pD3D11) {
        DrawSpriteD3D11(pSprite, x, y, frame, SPRITE_BLEND_ALPHA, alpha, 0xFFFFFFFF);
    } else {
        int alphaDepth = (int)((1.0f - alpha) * 100.0f);
        pSprite->PutTransSprite(x, y, frame, dwTime, alphaDepth);
    }
    
    m_iSpritesThisFrame++;
}

void CSpriteRenderer::DrawSprite25(CSprite* pSprite, int x, int y, int frame, DWORD dwTime)
{
    if (pSprite == NULL) return;
    
    if (m_bUseD3D11 && m_pD3D11) {
        DrawSpriteD3D11(pSprite, x, y, frame, SPRITE_BLEND_ALPHA_25, 0.25f, 0xFFFFFFFF);
    } else {
        pSprite->PutTransSprite25(x, y, frame, dwTime);
    }
    
    m_iSpritesThisFrame++;
}

void CSpriteRenderer::DrawSprite50(CSprite* pSprite, int x, int y, int frame, DWORD dwTime)
{
    if (pSprite == NULL) return;
    
    if (m_bUseD3D11 && m_pD3D11) {
        DrawSpriteD3D11(pSprite, x, y, frame, SPRITE_BLEND_ALPHA_50, 0.5f, 0xFFFFFFFF);
    } else {
        pSprite->PutTransSprite50(x, y, frame, dwTime);
    }
    
    m_iSpritesThisFrame++;
}

void CSpriteRenderer::DrawSprite70(CSprite* pSprite, int x, int y, int frame, DWORD dwTime)
{
    if (pSprite == NULL) return;
    
    if (m_bUseD3D11 && m_pD3D11) {
        DrawSpriteD3D11(pSprite, x, y, frame, SPRITE_BLEND_ALPHA_70, 0.7f, 0xFFFFFFFF);
    } else {
        pSprite->PutTransSprite70(x, y, frame, dwTime);
    }
    
    m_iSpritesThisFrame++;
}

void CSpriteRenderer::DrawSpriteRGB(CSprite* pSprite, int x, int y, int frame,
                                     int red, int green, int blue, DWORD dwTime)
{
    if (pSprite == NULL) return;
    
    if (m_bUseD3D11 && m_pD3D11) {
        DWORD tint = 0xFF000000 | (red << 16) | (green << 8) | blue;
        DrawSpriteD3D11(pSprite, x, y, frame, SPRITE_BLEND_RGB_TINT, 1.0f, tint);
    } else {
        pSprite->PutSpriteRGB(x, y, frame, red, green, blue, dwTime);
    }
    
    m_iSpritesThisFrame++;
}

void CSpriteRenderer::DrawSpriteRGBAlpha(CSprite* pSprite, int x, int y, int frame,
                                          int red, int green, int blue, DWORD dwTime, float alpha)
{
    if (pSprite == NULL) return;
    
    if (m_bUseD3D11 && m_pD3D11) {
        DWORD tint = 0xFF000000 | (red << 16) | (green << 8) | blue;
        DrawSpriteD3D11(pSprite, x, y, frame, SPRITE_BLEND_RGB_TINT, alpha, tint);
    } else {
        pSprite->PutTransSpriteRGB(x, y, frame, red, green, blue, dwTime);
    }
    
    m_iSpritesThisFrame++;
}

void CSpriteRenderer::DrawSpriteShadow(CSprite* pSprite, int x, int y, int frame, DWORD dwTime)
{
    if (pSprite == NULL) return;
    
    if (m_bUseD3D11 && m_pD3D11) {
        DrawSpriteD3D11(pSprite, x, y, frame, SPRITE_BLEND_SHADOW, 0.3f, 0xFF000000);
    } else {
        pSprite->PutShadowSprite(x, y, frame, dwTime);
    }
    
    m_iSpritesThisFrame++;
}

void CSpriteRenderer::DrawSpriteScaled(CSprite* pSprite, int x, int y, int frame,
                                        int width, int height, DWORD dwTime)
{
    if (pSprite == NULL) return;
    
    if (m_bUseD3D11 && m_pD3D11) {
        // TODO: Implementar escalado en D3D11
        // Por ahora, fallback a DDraw
        pSprite->PutSpriteScaled(x, y, frame, width, height, dwTime);
    } else {
        pSprite->PutSpriteScaled(x, y, frame, width, height, dwTime);
    }
    
    m_iSpritesThisFrame++;
}

void CSpriteRenderer::DrawSpriteShift(CSprite* pSprite, int x, int y, int shiftX, int shiftY,
                                       int frame, DWORD dwTime)
{
    if (pSprite == NULL) return;
    
    if (m_bUseD3D11 && m_pD3D11) {
        DrawSpriteD3D11(pSprite, x + shiftX, y + shiftY, frame, SPRITE_BLEND_NORMAL, 1.0f, 0xFFFFFFFF);
    } else {
        pSprite->PutShiftSpriteFast(x, y, shiftX, shiftY, frame, dwTime);
    }
    
    m_iSpritesThisFrame++;
}

// ============================================================================
// SISTEMA DE BATCHING
// ============================================================================

void CSpriteRenderer::BeginBatch()
{
    m_bInBatch = true;
    m_SpriteBatch.clear();
}

void CSpriteRenderer::EndBatch()
{
    if (!m_bInBatch) return;
    
    FlushBatch();
    m_bInBatch = false;
}

void CSpriteRenderer::BatchSprite(CSprite* pSprite, int x, int y, int frame,
                                   SpriteBlendMode mode, float alpha, DWORD tint)
{
    if (pSprite == NULL) return;
    
    SpriteBatchItem item;
    item.pSprite = pSprite;
    item.x = x;
    item.y = y;
    item.frame = frame;
    item.blendMode = mode;
    item.alpha = alpha;
    item.tintColor = tint;
    item.mirror = false;
    
    m_SpriteBatch.push_back(item);
    
    // Auto-flush si el batch está lleno
    if ((int)m_SpriteBatch.size() >= MAX_BATCH_SIZE) {
        FlushBatch();
    }
}

void CSpriteRenderer::FlushBatch()
{
    if (m_SpriteBatch.empty()) return;
    
    if (m_bUseD3D11 && m_pD3D11) {
        DrawBatchD3D11();
    } else {
        // Fallback: dibujar uno por uno con DirectDraw
        for (const auto& item : m_SpriteBatch) {
            switch (item.blendMode) {
                case SPRITE_BLEND_NORMAL:
                    item.pSprite->PutSpriteFast(item.x, item.y, item.frame, GetTickCount());
                    break;
                case SPRITE_BLEND_ALPHA:
                case SPRITE_BLEND_ALPHA_25:
                case SPRITE_BLEND_ALPHA_50:
                case SPRITE_BLEND_ALPHA_70:
                    {
                        int depth = (int)((1.0f - item.alpha) * 100.0f);
                        item.pSprite->PutTransSprite(item.x, item.y, item.frame, GetTickCount(), depth);
                    }
                    break;
                case SPRITE_BLEND_SHADOW:
                    item.pSprite->PutShadowSprite(item.x, item.y, item.frame, GetTickCount());
                    break;
                case SPRITE_BLEND_RGB_TINT:
                    {
                        int r = (item.tintColor >> 16) & 0xFF;
                        int g = (item.tintColor >> 8) & 0xFF;
                        int b = item.tintColor & 0xFF;
                        item.pSprite->PutSpriteRGB(item.x, item.y, item.frame, r, g, b, GetTickCount());
                    }
                    break;
                default:
                    item.pSprite->PutSpriteFast(item.x, item.y, item.frame, GetTickCount());
                    break;
            }
        }
    }
    
    m_iSpritesThisFrame += (int)m_SpriteBatch.size();
    m_iBatchCount++;
    m_SpriteBatch.clear();
}

// ============================================================================
// RENDERIZADO D3D11
// ============================================================================

void CSpriteRenderer::DrawSpriteD3D11(CSprite* pSprite, int x, int y, int frame,
                                       SpriteBlendMode mode, float alpha, DWORD tint)
{
    if (m_pD3D11 == NULL || pSprite == NULL) return;
    
    // Obtener textura del TextureManager
    SpriteFrameTexture* pTex = g_TextureManager.GetSpriteTexture(pSprite, frame);
    if (pTex == NULL || !pTex->valid || pTex->pSRV == NULL) {
        // Fallback a DirectDraw si no se pudo obtener la textura
        if (m_pDDraw != NULL) {
            pSprite->PutSpriteFast(x, y, frame, GetTickCount());
        }
        return;
    }
    
    // Calcular posición final (aplicando pivot)
    int finalX = x + pTex->pivotX;
    int finalY = y + pTex->pivotY;
    
    // Configurar blend state según el modo
    BlendMode d3dBlend = BlendMode::Alpha;
    switch (mode) {
        case SPRITE_BLEND_NORMAL:
            d3dBlend = BlendMode::Alpha;
            alpha = 1.0f;
            break;
        case SPRITE_BLEND_ALPHA:
        case SPRITE_BLEND_ALPHA_25:
        case SPRITE_BLEND_ALPHA_50:
        case SPRITE_BLEND_ALPHA_70:
            d3dBlend = BlendMode::Alpha;
            break;
        case SPRITE_BLEND_ADDITIVE:
            d3dBlend = BlendMode::Additive;
            break;
        case SPRITE_BLEND_SHADOW:
            d3dBlend = BlendMode::Multiply;
            alpha = 0.5f;
            break;
        case SPRITE_BLEND_RGB_TINT:
            d3dBlend = BlendMode::Alpha;
            break;
    }
    
    // Extraer color del tint (normalizado 0-1)
    float r = ((tint >> 16) & 0xFF) / 255.0f;
    float g = ((tint >> 8) & 0xFF) / 255.0f;
    float b = (tint & 0xFF) / 255.0f;
    
    // Dibujar usando el método del renderer
    m_pD3D11->DrawTexturedQuad(pTex->pSRV, finalX, finalY, 
                                pTex->width, pTex->height,
                                alpha, r, g, b, d3dBlend);
}

void CSpriteRenderer::DrawBatchD3D11()
{
    // TODO: Implementar batch rendering optimizado
    // Por ahora, dibujar uno por uno
    for (const auto& item : m_SpriteBatch) {
        DrawSpriteD3D11(item.pSprite, item.x, item.y, item.frame,
                        item.blendMode, item.alpha, item.tintColor);
    }
}
