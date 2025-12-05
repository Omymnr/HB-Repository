// RendererBridge.h
// Puente entre el código existente (DXC_ddraw) y el nuevo sistema de renderizado
// Este archivo facilita la migración gradual del cliente
//
// ESTRATEGIA DE INTEGRACIÓN:
// ==========================
// Fase 1: Este archivo actúa como wrapper - todo el código existente sigue funcionando
// Fase 2: Redirigimos llamadas específicas al nuevo renderer (D3D11)
// Fase 3: Migración completa a IRenderer
//
// El código existente usa m_DDraw (DXC_ddraw) directamente.
// Este bridge permite interceptar las llamadas más importantes.

#ifndef RENDERER_BRIDGE_H
#define RENDERER_BRIDGE_H

#include "IRenderer.h"
#include "RendererConfig.h"
#include "DXC_ddraw.h"

// Forward declarations
class DXC_ddraw;
class Direct3D11Renderer;
class DirectDrawRenderer;

// ============================================================================
// CLASE BRIDGE - Encapsula la decisión de qué renderer usar
// ============================================================================

class CRendererBridge
{
public:
    static CRendererBridge& GetInstance() {
        static CRendererBridge instance;
        return instance;
    }
    
    // Inicialización
    bool Initialize(HWND hWnd, DXC_ddraw* pLegacyDDraw);
    void Shutdown();
    
    // Obtener el renderer activo (puede ser wrapper de DDraw o D3D11)
    IRenderer* GetRenderer() { return m_pActiveRenderer; }
    
    // Obtener el renderer legacy para código que aún lo necesita
    DXC_ddraw* GetLegacyDDraw() { return m_pLegacyDDraw; }
    
    // ¿Estamos usando el nuevo renderer D3D11?
    bool IsUsingD3D11() const { return m_bUsingD3D11; }
    
    // Cambiar renderer en runtime (para testing)
    bool SwitchRenderer(RendererType type);
    
    // ========================================
    // FUNCIONES BRIDGE
    // Estas interceptan las llamadas más comunes
    // y las redirigen al renderer apropiado
    // ========================================
    
    // Begin/End frame
    void BeginFrame();
    void EndFrame();
    
    // Flip (presente el frame)
    HRESULT Flip();
    
    // Limpiar back buffer
    void ClearBackBuffer();
    
    // Dibujar sprite (función más llamada)
    // Por ahora, redirige al legacy DDraw
    // En fase 2, convertimos sprites a texturas D3D11
    void DrawSprite(int x, int y, void* pSprite, int frame, DWORD dwTime);
    
    // Texto
    void DrawText(int x, int y, const char* text, DWORD color);
    
    // Rectángulos y primitivas
    void DrawShadowBox(short sX, short sY, short dX, short dY, int iType = 0);
    
    // Resize handler
    void OnResize(int width, int height);
    
    // Alt+Tab / Focus handling
    void OnAppActivate(BOOL bActive);
    
private:
    CRendererBridge();
    ~CRendererBridge();
    CRendererBridge(const CRendererBridge&) = delete;
    CRendererBridge& operator=(const CRendererBridge&) = delete;
    
    // Renderer activo (implementa IRenderer)
    IRenderer* m_pActiveRenderer;
    
    // Puntero al DDraw legacy (siempre válido para compatibilidad)
    DXC_ddraw* m_pLegacyDDraw;
    
    // D3D11 renderer (solo si está activo)
    Direct3D11Renderer* m_pD3D11Renderer;
    
    // DirectDraw wrapper (usa m_pLegacyDDraw internamente)
    DirectDrawRenderer* m_pDDrawWrapper;
    
    // Estado
    bool m_bInitialized;
    bool m_bUsingD3D11;
    HWND m_hWnd;
};

// ============================================================================
// MACROS DE CONVENIENCIA
// Para hacer la transición más fácil en el código existente
// ============================================================================

// Obtener el bridge fácilmente
#define g_RendererBridge CRendererBridge::GetInstance()

// Obtener el renderer activo
#define g_Renderer CRendererBridge::GetInstance().GetRenderer()

// Verificar si estamos en modo D3D11
#define g_IsD3D11 CRendererBridge::GetInstance().IsUsingD3D11()

#endif // RENDERER_BRIDGE_H
