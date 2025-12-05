// TextureManager.cpp - Implementación del gestor de texturas D3D11
// Convierte sprites DirectDraw a texturas Direct3D11 de forma eficiente

#include "TextureManager.h"
#include "Direct3D11Renderer.h"
#include "Sprite.h"
#include "DXC_ddraw.h"
#include <stdio.h>

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

CTextureManager::CTextureManager()
    : m_pRenderer(NULL)
    , m_pDevice(NULL)
    , m_bInitialized(false)
    , m_iTotalMemory(0)
    , m_iMaxMemory(512 * 1024 * 1024)  // 512 MB por defecto
    , m_iTotalTextures(0)
{
}

CTextureManager::~CTextureManager()
{
    Shutdown();
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================

bool CTextureManager::Initialize(Direct3D11Renderer* pRenderer)
{
    if (m_bInitialized) {
        return true;
    }
    
    if (pRenderer == NULL) {
        return false;
    }
    
    m_pRenderer = pRenderer;
    m_pDevice = pRenderer->GetDevice();
    
    if (m_pDevice == NULL) {
        return false;
    }
    
    m_bInitialized = true;
    
    OutputDebugStringA("TextureManager: Inicializado correctamente\n");
    return true;
}

void CTextureManager::Shutdown()
{
    if (!m_bInitialized) return;
    
    // Liberar todas las texturas
    for (auto& pair : m_SpriteCache) {
        SpriteTextureCache& cache = pair.second;
        for (auto& frame : cache.frames) {
            if (frame.pSRV) {
                frame.pSRV->Release();
                frame.pSRV = NULL;
            }
            if (frame.pTexture) {
                frame.pTexture->Release();
                frame.pTexture = NULL;
            }
        }
        cache.frames.clear();
    }
    m_SpriteCache.clear();
    
    m_pRenderer = NULL;
    m_pDevice = NULL;
    m_bInitialized = false;
    m_iTotalMemory = 0;
    m_iTotalTextures = 0;
    
    OutputDebugStringA("TextureManager: Shutdown completado\n");
}

// ============================================================================
// OBTENER TEXTURA DE SPRITE
// ============================================================================

SpriteFrameTexture* CTextureManager::GetSpriteTexture(CSprite* pSprite, int frame)
{
    if (!m_bInitialized || pSprite == NULL) {
        return NULL;
    }
    
    if (frame < 0 || frame >= pSprite->m_iTotalFrame) {
        return NULL;
    }
    
    // Obtener ID único del sprite
    UINT64 spriteID = GetSpriteID(pSprite);
    
    // Buscar en cache
    auto it = m_SpriteCache.find(spriteID);
    
    if (it != m_SpriteCache.end()) {
        // Sprite está en cache
        SpriteTextureCache& cache = it->second;
        cache.lastUsedTime = GetTickCount();
        
        // Verificar si el frame específico está cargado
        if (frame < (int)cache.frames.size() && cache.frames[frame].valid) {
            return &cache.frames[frame];
        }
        
        // Frame no está cargado, convertirlo
        if (frame >= (int)cache.frames.size()) {
            cache.frames.resize(frame + 1);
        }
        
        cache.frames[frame] = ConvertSpriteFrame(pSprite, frame);
        if (cache.frames[frame].valid) {
            return &cache.frames[frame];
        }
        return NULL;
    }
    
    // Sprite no está en cache, crear entrada
    SpriteTextureCache newCache;
    newCache.frames.resize(pSprite->m_iTotalFrame);
    newCache.lastUsedTime = GetTickCount();
    newCache.loaded = false;
    
    // Convertir el frame solicitado
    newCache.frames[frame] = ConvertSpriteFrame(pSprite, frame);
    
    // Guardar en cache
    m_SpriteCache[spriteID] = newCache;
    
    if (m_SpriteCache[spriteID].frames[frame].valid) {
        return &m_SpriteCache[spriteID].frames[frame];
    }
    
    return NULL;
}

// ============================================================================
// PRECARGAR SPRITE COMPLETO
// ============================================================================

bool CTextureManager::PreloadSprite(CSprite* pSprite)
{
    if (!m_bInitialized || pSprite == NULL) {
        return false;
    }
    
    UINT64 spriteID = GetSpriteID(pSprite);
    
    // Verificar si ya está completamente cargado
    auto it = m_SpriteCache.find(spriteID);
    if (it != m_SpriteCache.end() && it->second.loaded) {
        it->second.lastUsedTime = GetTickCount();
        return true;
    }
    
    // Crear o actualizar cache
    SpriteTextureCache& cache = m_SpriteCache[spriteID];
    cache.frames.resize(pSprite->m_iTotalFrame);
    cache.lastUsedTime = GetTickCount();
    
    // Convertir todos los frames
    int successCount = 0;
    for (int i = 0; i < pSprite->m_iTotalFrame; i++) {
        if (!cache.frames[i].valid) {
            cache.frames[i] = ConvertSpriteFrame(pSprite, i);
            if (cache.frames[i].valid) {
                successCount++;
            }
        } else {
            successCount++;
        }
    }
    
    cache.loaded = (successCount == pSprite->m_iTotalFrame);
    
    return cache.loaded;
}

// ============================================================================
// LIBERAR SPRITE
// ============================================================================

void CTextureManager::ReleaseSprite(CSprite* pSprite)
{
    if (pSprite == NULL) return;
    
    UINT64 spriteID = GetSpriteID(pSprite);
    
    auto it = m_SpriteCache.find(spriteID);
    if (it != m_SpriteCache.end()) {
        SpriteTextureCache& cache = it->second;
        
        for (auto& frame : cache.frames) {
            if (frame.pSRV) {
                frame.pSRV->Release();
                m_iTotalTextures--;
            }
            if (frame.pTexture) {
                frame.pTexture->Release();
            }
            m_iTotalMemory -= frame.width * frame.height * 4;
        }
        
        m_SpriteCache.erase(it);
    }
}

// ============================================================================
// LIMPIEZA DE TEXTURAS NO USADAS
// ============================================================================

void CTextureManager::CleanupUnusedTextures(DWORD maxAge)
{
    DWORD currentTime = GetTickCount();
    std::vector<UINT64> toRemove;
    
    for (auto& pair : m_SpriteCache) {
        if (currentTime - pair.second.lastUsedTime > maxAge) {
            toRemove.push_back(pair.first);
        }
    }
    
    for (UINT64 id : toRemove) {
        SpriteTextureCache& cache = m_SpriteCache[id];
        
        for (auto& frame : cache.frames) {
            if (frame.pSRV) {
                frame.pSRV->Release();
                m_iTotalTextures--;
            }
            if (frame.pTexture) {
                frame.pTexture->Release();
            }
            m_iTotalMemory -= frame.width * frame.height * 4;
        }
        
        m_SpriteCache.erase(id);
    }
    
    if (toRemove.size() > 0) {
        char msg[128];
        sprintf(msg, "TextureManager: Liberadas %d texturas no usadas\n", (int)toRemove.size());
        OutputDebugStringA(msg);
    }
}

// ============================================================================
// CONVERSIÓN DE SPRITE A TEXTURA D3D11
// ============================================================================

SpriteFrameTexture CTextureManager::ConvertSpriteFrame(CSprite* pSprite, int frame)
{
    SpriteFrameTexture result;
    result.valid = false;
    
    if (pSprite == NULL || frame < 0 || frame >= pSprite->m_iTotalFrame) {
        return result;
    }
    
    // Asegurar que el sprite tiene su superficie DirectDraw cargada
    if (pSprite->m_bIsSurfaceEmpty || pSprite->m_lpSurface == NULL) {
        // Intentar cargar el sprite
        if (!pSprite->_iOpenSprite()) {
            OutputDebugStringA("TextureManager: No se pudo cargar superficie del sprite\n");
            return result;
        }
    }
    
    // Obtener información del frame
    int width = pSprite->m_stBrush[frame].szx;
    int height = pSprite->m_stBrush[frame].szy;
    int srcX = pSprite->m_stBrush[frame].sx;
    int srcY = pSprite->m_stBrush[frame].sy;
    
    if (width <= 0 || height <= 0) {
        return result;
    }
    
    // Bloquear la superficie DirectDraw para leer los pixels
    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    
    HRESULT hr = pSprite->m_lpSurface->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_READONLY, NULL);
    if (FAILED(hr)) {
        OutputDebugStringA("TextureManager: No se pudo bloquear superficie DDraw\n");
        return result;
    }
    
    // Obtener datos
    WORD* surfaceData = (WORD*)ddsd.lpSurface;
    int pitch = ddsd.lPitch / 2;  // Pitch en WORDs (16-bit)
    WORD colorKey = pSprite->m_wColorKey;
    
    // Crear buffer de pixels 32-bit BGRA
    DWORD* pixelData = new DWORD[width * height];
    
    for (int y = 0; y < height; y++) {
        WORD* srcRow = surfaceData + (srcY + y) * pitch + srcX;
        DWORD* dstRow = pixelData + y * width;
        
        for (int x = 0; x < width; x++) {
            dstRow[x] = Convert565to8888(srcRow[x], colorKey);
        }
    }
    
    pSprite->m_lpSurface->Unlock(NULL);
    
    // Crear textura D3D11
    D3D11_TEXTURE2D_DESC td;
    ZeroMemory(&td, sizeof(td));
    td.Width = width;
    td.Height = height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    
    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(initData));
    initData.pSysMem = pixelData;
    initData.SysMemPitch = width * sizeof(DWORD);
    
    ID3D11Texture2D* pTexture = NULL;
    hr = m_pDevice->CreateTexture2D(&td, &initData, &pTexture);
    
    delete[] pixelData;
    
    if (FAILED(hr)) {
        char msg[128];
        sprintf(msg, "TextureManager: Error creando textura: 0x%08X\n", hr);
        OutputDebugStringA(msg);
        return result;
    }
    
    // Crear shader resource view
    ID3D11ShaderResourceView* pSRV = NULL;
    hr = m_pDevice->CreateShaderResourceView(pTexture, NULL, &pSRV);
    
    if (FAILED(hr)) {
        pTexture->Release();
        return result;
    }
    
    // Llenar resultado
    result.pTexture = pTexture;
    result.pSRV = pSRV;
    result.width = width;
    result.height = height;
    result.pivotX = pSprite->m_stBrush[frame].pvx;
    result.pivotY = pSprite->m_stBrush[frame].pvy;
    result.valid = true;
    
    // Actualizar estadísticas
    m_iTotalMemory += width * height * 4;
    m_iTotalTextures++;
    
    return result;
}

// ============================================================================
// CONVERSIÓN DE COLOR 565 A 8888
// ============================================================================

DWORD CTextureManager::Convert565to8888(WORD pixel, WORD colorKey)
{
    // Si es el color key, retornar transparente
    if (pixel == colorKey) {
        return 0x00000000;
    }
    
    // Extraer componentes RGB del formato 565
    // Formato: RRRRRGGGGGGBBBBB
    BYTE r = ((pixel >> 11) & 0x1F);
    BYTE g = ((pixel >> 5) & 0x3F);
    BYTE b = (pixel & 0x1F);
    
    // Expandir a 8 bits
    // R: 5 bits -> 8 bits (multiplicar por 8.225... ≈ *8 + *0.25)
    r = (r << 3) | (r >> 2);
    // G: 6 bits -> 8 bits (multiplicar por 4.047... ≈ *4 + *0.0625)
    g = (g << 2) | (g >> 4);
    // B: 5 bits -> 8 bits
    b = (b << 3) | (b >> 2);
    
    // Formato BGRA (DirectX usa este orden)
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

// ============================================================================
// GENERAR ID ÚNICO DE SPRITE
// ============================================================================

UINT64 CTextureManager::GetSpriteID(CSprite* pSprite)
{
    // Usamos la dirección de memoria del sprite como ID único
    // Esto funciona porque cada CSprite tiene una dirección única
    return (UINT64)(uintptr_t)pSprite;
}

// ============================================================================
// ESTADÍSTICAS
// ============================================================================

int CTextureManager::GetTotalTextureCount() const
{
    return m_iTotalTextures;
}

int CTextureManager::GetTotalMemoryUsage() const
{
    return m_iTotalMemory;
}
