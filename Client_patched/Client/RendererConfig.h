// RendererConfig.h
// Configuración del sistema de renderizado
// Parte del proyecto de modernización del cliente

#ifndef RENDERER_CONFIG_H
#define RENDERER_CONFIG_H

#include <windows.h>
#include <string>

// Tipos de renderer disponibles
enum RendererType {
    RENDERER_AUTO = 0,      // Auto-detectar mejor opción
    RENDERER_DIRECTDRAW = 1, // DirectDraw 7 (legacy, máxima compatibilidad)
    RENDERER_DIRECT3D11 = 2  // Direct3D 11 (moderno, Windows 7+)
};

// Modos de escalado
enum ScaleMode {
    SCALE_POINT = 0,        // Escalado por punto (pixelado)
    SCALE_BILINEAR = 1,     // Escalado bilinear (suave)
    SCALE_INTEGER = 2       // Escalado entero (pixel-perfect)
};

// Configuración de video
struct VideoSettings {
    int width;              // Ancho de pantalla
    int height;             // Alto de pantalla
    bool fullscreen;        // Pantalla completa
    bool vsync;             // Sincronización vertical
    RendererType renderer;  // Tipo de renderer
    int quality;            // Calidad gráfica (0-3)
    ScaleMode scaleMode;    // Modo de escalado
    
    // Configuración de audio
    int soundVolume;        // Volumen de efectos de sonido (0-100)
    int musicVolume;        // Volumen de música (0-100)
    
    // Valores por defecto - modernos para hardware actual
    VideoSettings() :
        width(1920),         // Full HD por defecto
        height(1080),
        fullscreen(true),
        vsync(true),
        renderer(RENDERER_DIRECT3D11),  // D3D11 por defecto
        quality(2),
        scaleMode(SCALE_BILINEAR),
        soundVolume(100),
        musicVolume(100) {}
};

// Clase para manejar la configuración del renderer
class CRendererConfig
{
public:
    static CRendererConfig& GetInstance() {
        static CRendererConfig instance;
        return instance;
    }
    
    // Cargar/Guardar configuración
    bool LoadFromFile(const char* filename);
    bool SaveToFile(const char* filename);
    
    // Cargar desde registro de Windows
    bool LoadFromRegistry();
    bool SaveToRegistry();
    
    // Cargar desde argumentos de línea de comandos
    bool LoadFromCommandLine(const char* cmdLine);
    
    // Obtener/Establecer settings
    VideoSettings& GetVideoSettings() { return m_videoSettings; }
    void SetVideoSettings(const VideoSettings& settings) { m_videoSettings = settings; }
    
    // Helpers
    RendererType GetRendererType() const { return m_videoSettings.renderer; }
    void SetRendererType(RendererType type) { m_videoSettings.renderer = type; }
    
    int GetScreenWidth() const { return m_videoSettings.width; }
    int GetScreenHeight() const { return m_videoSettings.height; }
    void SetResolution(int width, int height) {
        m_videoSettings.width = width;
        m_videoSettings.height = height;
    }
    
    bool IsFullscreen() const { return m_videoSettings.fullscreen; }
    void SetFullscreen(bool fs) { m_videoSettings.fullscreen = fs; }
    
    bool IsVsyncEnabled() const { return m_videoSettings.vsync; }
    void SetVsync(bool vsync) { m_videoSettings.vsync = vsync; }
    
    ScaleMode GetScaleMode() const { return m_videoSettings.scaleMode; }
    void SetScaleMode(ScaleMode mode) { m_videoSettings.scaleMode = mode; }
    
    // Audio settings
    int GetSoundVolume() const { return m_videoSettings.soundVolume; }
    void SetSoundVolume(int vol) { m_videoSettings.soundVolume = (vol < 0 ? 0 : (vol > 100 ? 100 : vol)); }
    
    int GetMusicVolume() const { return m_videoSettings.musicVolume; }
    void SetMusicVolume(int vol) { m_videoSettings.musicVolume = (vol < 0 ? 0 : (vol > 100 ? 100 : vol)); }
    
    // Detectar capacidades del sistema
    static bool IsD3D11Available();
    static bool IsDirectDrawAvailable();
    static RendererType DetectBestRenderer();
    
    // Obtener lista de resoluciones soportadas
    struct Resolution {
        int width;
        int height;
    };
    static int GetSupportedResolutions(Resolution* outResolutions, int maxCount);
    
private:
    CRendererConfig() {}
    ~CRendererConfig() {}
    CRendererConfig(const CRendererConfig&) = delete;
    CRendererConfig& operator=(const CRendererConfig&) = delete;
    
    VideoSettings m_videoSettings;
    
    // Constantes de registro
    static const char* REGISTRY_KEY;
    static const char* REG_RENDERER;
    static const char* REG_WIDTH;
    static const char* REG_HEIGHT;
    static const char* REG_FULLSCREEN;
    static const char* REG_VSYNC;
};

#endif // RENDERER_CONFIG_H
