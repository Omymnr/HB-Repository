// TextureManager.h - Gestión de texturas para Direct3D 11
// Convierte sprites de DirectDraw a texturas D3D11 de forma transparente
//
// Este sistema permite que los sprites existentes funcionen con D3D11
// sin modificar significativamente el código de CSprite.

#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <windows.h>
#include <d3d11.h>
#include <map>
#include <vector>

// Forward declarations
class CSprite;
class Direct3D11Renderer;

// Información de un frame de sprite convertido a textura
struct SpriteFrameTexture {
    ID3D11Texture2D* pTexture;
    ID3D11ShaderResourceView* pSRV;
    int width;
    int height;
    int pivotX;     // Offset del pivot
    int pivotY;
    bool valid;
    
    SpriteFrameTexture() : pTexture(NULL), pSRV(NULL), 
                           width(0), height(0), pivotX(0), pivotY(0), valid(false) {}
};

// Cache de un sprite completo (todos sus frames)
struct SpriteTextureCache {
    std::vector<SpriteFrameTexture> frames;
    DWORD lastUsedTime;
    int totalMemory;    // Memoria estimada en bytes
    bool loaded;
    
    SpriteTextureCache() : lastUsedTime(0), totalMemory(0), loaded(false) {}
};

// Clase singleton para gestionar todas las texturas de sprites
class CTextureManager {
public:
    static CTextureManager& GetInstance() {
        static CTextureManager instance;
        return instance;
    }
    
    // Inicializar con el renderer D3D11
    bool Initialize(Direct3D11Renderer* pRenderer);
    void Shutdown();
    
    // Obtener textura de un sprite/frame específico
    // Convierte automáticamente si no existe en cache
    SpriteFrameTexture* GetSpriteTexture(CSprite* pSprite, int frame);
    
    // Precargar todos los frames de un sprite
    bool PreloadSprite(CSprite* pSprite);
    
    // Liberar texturas de un sprite
    void ReleaseSprite(CSprite* pSprite);
    
    // Liberar texturas no usadas (para gestión de memoria)
    void CleanupUnusedTextures(DWORD maxAge = 30000); // 30 segundos por defecto
    
    // Estadísticas
    int GetTotalTextureCount() const;
    int GetTotalMemoryUsage() const;  // En bytes
    
    // Configuración
    void SetMaxMemory(int maxBytes) { m_iMaxMemory = maxBytes; }
    int GetMaxMemory() const { return m_iMaxMemory; }
    
private:
    CTextureManager();
    ~CTextureManager();
    CTextureManager(const CTextureManager&) = delete;
    CTextureManager& operator=(const CTextureManager&) = delete;
    
    // Convertir un frame de sprite DDraw a textura D3D11
    SpriteFrameTexture ConvertSpriteFrame(CSprite* pSprite, int frame);
    
    // Convertir pixel 565 RGB a 8888 BGRA
    DWORD Convert565to8888(WORD pixel, WORD colorKey);
    
    // Generar ID único para un sprite
    UINT64 GetSpriteID(CSprite* pSprite);
    
    // Cache de sprites
    std::map<UINT64, SpriteTextureCache> m_SpriteCache;
    
    // Renderer D3D11
    Direct3D11Renderer* m_pRenderer;
    ID3D11Device* m_pDevice;
    
    // Estado
    bool m_bInitialized;
    int m_iTotalMemory;
    int m_iMaxMemory;       // Límite de memoria (default 512MB)
    int m_iTotalTextures;
};

// Macro de conveniencia
#define g_TextureManager CTextureManager::GetInstance()

#endif // TEXTURE_MANAGER_H
