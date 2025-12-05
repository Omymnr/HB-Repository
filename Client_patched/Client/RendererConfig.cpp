// RendererConfig.cpp
// Implementación de la configuración del sistema de renderizado

#include "RendererConfig.h"
#include <d3d11.h>
#include <ddraw.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Constantes de registro
const char* CRendererConfig::REGISTRY_KEY = "Software\\Siementech\\Helbreath Xtreme\\Video";
const char* CRendererConfig::REG_RENDERER = "Renderer";
const char* CRendererConfig::REG_WIDTH = "Width";
const char* CRendererConfig::REG_HEIGHT = "Height";
const char* CRendererConfig::REG_FULLSCREEN = "Fullscreen";
const char* CRendererConfig::REG_VSYNC = "VSync";

// ===== HELPER: Parsear argumento numérico de la línea de comandos =====
static bool GetCommandLineArg(const char* cmdLine, const char* argName, int* outValue)
{
    if (!cmdLine || !argName || !outValue) return false;
    
    char searchPattern[64];
    sprintf(searchPattern, "-%s", argName);
    
    const char* found = strstr(cmdLine, searchPattern);
    if (!found) {
        // Intentar con /
        sprintf(searchPattern, "/%s", argName);
        found = strstr(cmdLine, searchPattern);
    }
    
    if (!found) return false;
    
    // Avanzar al valor
    const char* valueStart = found + strlen(searchPattern);
    while (*valueStart == ' ' || *valueStart == '=' || *valueStart == ':') valueStart++;
    
    if (*valueStart == '\0' || *valueStart == '-' || *valueStart == '/') return false;
    
    *outValue = atoi(valueStart);
    return true;
}

// ===== CARGAR DESDE LÍNEA DE COMANDOS =====
bool CRendererConfig::LoadFromCommandLine(const char* cmdLine)
{
    if (!cmdLine || strlen(cmdLine) == 0) {
        OutputDebugStringA("[HB-CONFIG] LoadFromCommandLine: linea de comandos vacia\n");
        return false;
    }
    
    char msg[512];
    sprintf(msg, "[HB-CONFIG] LoadFromCommandLine('%s')\n", cmdLine);
    OutputDebugStringA(msg);
    
    bool foundAny = false;
    int value;
    
    // Parsear -renderer X
    if (GetCommandLineArg(cmdLine, "renderer", &value)) {
        m_videoSettings.renderer = (RendererType)value;
        sprintf(msg, "[HB-CONFIG] CLI Renderer = %d\n", value);
        OutputDebugStringA(msg);
        foundAny = true;
    }
    
    // Parsear -width X
    if (GetCommandLineArg(cmdLine, "width", &value)) {
        m_videoSettings.width = value;
        sprintf(msg, "[HB-CONFIG] CLI Width = %d\n", value);
        OutputDebugStringA(msg);
        foundAny = true;
    }
    
    // Parsear -height X
    if (GetCommandLineArg(cmdLine, "height", &value)) {
        m_videoSettings.height = value;
        sprintf(msg, "[HB-CONFIG] CLI Height = %d\n", value);
        OutputDebugStringA(msg);
        foundAny = true;
    }
    
    // Parsear -vsync X
    if (GetCommandLineArg(cmdLine, "vsync", &value)) {
        m_videoSettings.vsync = (value != 0);
        sprintf(msg, "[HB-CONFIG] CLI VSync = %d\n", value);
        OutputDebugStringA(msg);
        foundAny = true;
    }
    
    // Parsear -scalemode X
    if (GetCommandLineArg(cmdLine, "scalemode", &value)) {
        m_videoSettings.scaleMode = (ScaleMode)value;
        sprintf(msg, "[HB-CONFIG] CLI ScaleMode = %d\n", value);
        OutputDebugStringA(msg);
        foundAny = true;
    }
    
    // Parsear -fullscreen X
    if (GetCommandLineArg(cmdLine, "fullscreen", &value)) {
        m_videoSettings.fullscreen = (value != 0);
        sprintf(msg, "[HB-CONFIG] CLI Fullscreen = %d\n", value);
        OutputDebugStringA(msg);
        foundAny = true;
    }
    
    if (foundAny) {
        sprintf(msg, "[HB-CONFIG] CLI Result: %dx%d, Renderer=%d, VSync=%d, ScaleMode=%d\n",
                m_videoSettings.width, m_videoSettings.height,
                m_videoSettings.renderer, m_videoSettings.vsync, m_videoSettings.scaleMode);
        OutputDebugStringA(msg);
    }
    
    return foundAny;
}

// ===== CARGAR/GUARDAR ARCHIVO =====

bool CRendererConfig::LoadFromFile(const char* filename)
{
    char msg[256];
    sprintf(msg, "[HB-CONFIG] LoadFromFile('%s')\\n", filename);
    OutputDebugStringA(msg);
    
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        OutputDebugStringA("[HB-CONFIG] ERROR: No se pudo abrir el archivo\\n");
        return false;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        // Ignorar comentarios y líneas vacías
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        
        char key[64], value[64];
        if (sscanf(line, "%63[^=]=%63s", key, value) == 2) {
            // Limpiar espacios
            char* k = key;
            while (*k == ' ') k++;
            char* v = value;
            while (*v == ' ') v++;
            
            // Eliminar espacios finales del key
            char* end = k + strlen(k) - 1;
            while (end > k && (*end == ' ' || *end == '\t')) {
                *end = '\0';
                end--;
            }
            
            if (strcmp(k, "Renderer") == 0) {
                m_videoSettings.renderer = (RendererType)atoi(v);
                sprintf(msg, "[HB-CONFIG] Renderer = %d\\n", m_videoSettings.renderer);
                OutputDebugStringA(msg);
            }
            else if (strcmp(k, "ScreenWidth") == 0 || strcmp(k, "Width") == 0) {
                m_videoSettings.width = atoi(v);
                sprintf(msg, "[HB-CONFIG] Width = %d\\n", m_videoSettings.width);
                OutputDebugStringA(msg);
            }
            else if (strcmp(k, "ScreenHeight") == 0 || strcmp(k, "Height") == 0) {
                m_videoSettings.height = atoi(v);
                sprintf(msg, "[HB-CONFIG] Height = %d\\n", m_videoSettings.height);
                OutputDebugStringA(msg);
            }
            else if (strcmp(k, "Fullscreen") == 0) {
                m_videoSettings.fullscreen = (atoi(v) != 0);
                sprintf(msg, "[HB-CONFIG] Fullscreen = %d\\n", m_videoSettings.fullscreen);
                OutputDebugStringA(msg);
            }
            else if (strcmp(k, "VSync") == 0) {
                m_videoSettings.vsync = (atoi(v) != 0);
                sprintf(msg, "[HB-CONFIG] VSync = %d\\n", m_videoSettings.vsync);
                OutputDebugStringA(msg);
            }
            else if (strcmp(k, "Quality") == 0) {
                m_videoSettings.quality = atoi(v);
            }
            else if (strcmp(k, "ScaleMode") == 0) {
                m_videoSettings.scaleMode = (ScaleMode)atoi(v);
                sprintf(msg, "[HB-CONFIG] ScaleMode = %d\\n", m_videoSettings.scaleMode);
                OutputDebugStringA(msg);
            }
            else if (strcmp(k, "MaintainAspect") == 0) {
                // Reservado para futuro
            }
        }
    }
    
    fclose(fp);
    
    sprintf(msg, "[HB-CONFIG] Resultado final: %dx%d, Renderer=%d, Fullscreen=%d, VSync=%d\\n",
            m_videoSettings.width, m_videoSettings.height, 
            m_videoSettings.renderer, m_videoSettings.fullscreen, m_videoSettings.vsync);
    OutputDebugStringA(msg);
    
    return true;
}

bool CRendererConfig::SaveToFile(const char* filename)
{
    FILE* fp = fopen(filename, "w");
    if (!fp) return false;
    
    fprintf(fp, "# Video Configuration\n");
    fprintf(fp, "# Renderer: 0=Auto, 1=DirectDraw, 2=Direct3D11\n");
    fprintf(fp, "# ScaleMode: 0=Point, 1=Bilinear, 2=Integer\n");
    fprintf(fp, "Renderer=%d\n", (int)m_videoSettings.renderer);
    fprintf(fp, "Width=%d\n", m_videoSettings.width);
    fprintf(fp, "Height=%d\n", m_videoSettings.height);
    fprintf(fp, "Fullscreen=%d\n", m_videoSettings.fullscreen ? 1 : 0);
    fprintf(fp, "VSync=%d\n", m_videoSettings.vsync ? 1 : 0);
    fprintf(fp, "ScaleMode=%d\n", (int)m_videoSettings.scaleMode);
    fprintf(fp, "Quality=%d\n", m_videoSettings.quality);
    
    fclose(fp);
    return true;
}

// ===== REGISTRO DE WINDOWS =====

bool CRendererConfig::LoadFromRegistry()
{
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    DWORD type, size, value;
    
    size = sizeof(value);
    if (RegQueryValueExA(hKey, REG_RENDERER, NULL, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
        m_videoSettings.renderer = (RendererType)value;
    }
    
    size = sizeof(value);
    if (RegQueryValueExA(hKey, REG_WIDTH, NULL, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
        m_videoSettings.width = value;
    }
    
    size = sizeof(value);
    if (RegQueryValueExA(hKey, REG_HEIGHT, NULL, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
        m_videoSettings.height = value;
    }
    
    size = sizeof(value);
    if (RegQueryValueExA(hKey, REG_FULLSCREEN, NULL, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
        m_videoSettings.fullscreen = (value != 0);
    }
    
    size = sizeof(value);
    if (RegQueryValueExA(hKey, REG_VSYNC, NULL, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
        m_videoSettings.vsync = (value != 0);
    }
    
    RegCloseKey(hKey);
    return true;
}

bool CRendererConfig::SaveToRegistry()
{
    HKEY hKey;
    DWORD disposition;
    
    if (RegCreateKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, 
                        &hKey, &disposition) != ERROR_SUCCESS) {
        return false;
    }
    
    DWORD value;
    
    value = (DWORD)m_videoSettings.renderer;
    RegSetValueExA(hKey, REG_RENDERER, 0, REG_DWORD, (LPBYTE)&value, sizeof(value));
    
    value = m_videoSettings.width;
    RegSetValueExA(hKey, REG_WIDTH, 0, REG_DWORD, (LPBYTE)&value, sizeof(value));
    
    value = m_videoSettings.height;
    RegSetValueExA(hKey, REG_HEIGHT, 0, REG_DWORD, (LPBYTE)&value, sizeof(value));
    
    value = m_videoSettings.fullscreen ? 1 : 0;
    RegSetValueExA(hKey, REG_FULLSCREEN, 0, REG_DWORD, (LPBYTE)&value, sizeof(value));
    
    value = m_videoSettings.vsync ? 1 : 0;
    RegSetValueExA(hKey, REG_VSYNC, 0, REG_DWORD, (LPBYTE)&value, sizeof(value));
    
    RegCloseKey(hKey);
    return true;
}

// ===== DETECCIÓN DE CAPACIDADES =====

bool CRendererConfig::IsD3D11Available()
{
    // Intentar crear un device D3D11 para verificar soporte
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL featureLevel;
    
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    
    HRESULT hr = D3D11CreateDevice(
        nullptr,                    // Adaptador por defecto
        D3D_DRIVER_TYPE_HARDWARE,   // Hardware acceleration
        nullptr,                    // Sin software rasterizer
        0,                          // Sin flags especiales
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &device,
        &featureLevel,
        &context
    );
    
    if (SUCCEEDED(hr)) {
        if (context) context->Release();
        if (device) device->Release();
        return true;
    }
    
    return false;
}

bool CRendererConfig::IsDirectDrawAvailable()
{
    // DirectDraw siempre está disponible en Windows (emulado si es necesario)
    // Pero verificamos de todas formas
    
    LPDIRECTDRAW lpDD = nullptr;
    HRESULT hr = DirectDrawCreate(NULL, &lpDD, NULL);
    
    if (SUCCEEDED(hr)) {
        lpDD->Release();
        return true;
    }
    
    return false;
}

RendererType CRendererConfig::DetectBestRenderer()
{
    // Windows 10/11 con hardware moderno: preferir D3D11
    // Windows XP/Vista o hardware antiguo: usar DirectDraw
    
    // Verificar versión de Windows
    OSVERSIONINFOA osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    
    #pragma warning(disable: 4996) // GetVersionExA deprecated
    GetVersionExA(&osvi);
    #pragma warning(default: 4996)
    
    // Windows 7 = 6.1, Windows 8 = 6.2, Windows 10/11 = 10.0
    bool isWindows7OrNewer = (osvi.dwMajorVersion > 6) || 
                             (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion >= 1);
    
    if (isWindows7OrNewer && IsD3D11Available()) {
        return RENDERER_DIRECT3D11;
    }
    
    if (IsDirectDrawAvailable()) {
        return RENDERER_DIRECTDRAW;
    }
    
    // Fallback a D3D11 si DirectDraw no está disponible
    return RENDERER_DIRECT3D11;
}

// ===== RESOLUCIONES SOPORTADAS =====

int CRendererConfig::GetSupportedResolutions(Resolution* outResolutions, int maxCount)
{
    int count = 0;
    
    // Resoluciones estándar comunes
    static const Resolution standardResolutions[] = {
        {800, 600},     // Original del juego
        {1024, 768},    // XGA
        {1280, 720},    // 720p HD
        {1280, 800},    // WXGA
        {1366, 768},    // HD
        {1440, 900},    // WXGA+
        {1600, 900},    // HD+
        {1680, 1050},   // WSXGA+
        {1920, 1080},   // 1080p Full HD
        {2560, 1440},   // 1440p QHD
        {3840, 2160},   // 4K UHD
    };
    
    // Obtener resolución del escritorio
    int desktopWidth = GetSystemMetrics(SM_CXSCREEN);
    int desktopHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Solo incluir resoluciones que caben en la pantalla
    for (int i = 0; i < sizeof(standardResolutions) / sizeof(Resolution); i++) {
        if (count >= maxCount) break;
        
        if (standardResolutions[i].width <= desktopWidth &&
            standardResolutions[i].height <= desktopHeight) {
            outResolutions[count++] = standardResolutions[i];
        }
    }
    
    return count;
}
