// --------------------------------------------------------------
//                  Helbreath External Launcher
//                  Con Sistema de Auto-Update via GitHub
//
//          Mismo estilo visual que el launcher del Game.exe
//          Ejecuta Game.exe con -nolauncher para saltar su launcher
// --------------------------------------------------------------

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma warning(disable: 4018)

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <wininet.h>
#include <wincrypt.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "crypt32.lib")

// Recurso del icono (definir en resource.h)
#define IDI_ICON1 101

// ===== CONFIGURACIÓN - CAMBIAR ESTOS VALORES =====

// URL de tu repositorio de GitHub (raw content)
// Formato: https://raw.githubusercontent.com/USUARIO/REPOSITORIO/RAMA
#define GITHUB_RAW_URL "https://raw.githubusercontent.com/Omymnr/HB-Repository/main/updates"

// Versión actual del cliente instalado (se lee de version.txt)
#define DEFAULT_VERSION "1.0.0"

// ===== FIN CONFIGURACIÓN =====

// ===== LAUNCHER CONFIGURATION =====
#define LAUNCHER_WIDTH 800
#define LAUNCHER_HEIGHT 600
#define LEFT_PANEL_WIDTH 400
#define ID_CHECKBOX_BORDERLESS 1001
#define ID_RADIO_ONLINE 1004
#define ID_RADIO_TEST 1005
#define ID_COMBO_RENDERER 1006
#define ID_COMBO_RESOLUTION 1007
#define ID_CHECKBOX_VSYNC 1008
#define ID_COMBO_SCALEMODE 1009
#define ID_CHECKBOX_FULLSCREEN 1010

// Colores del tema medieval (idénticos al Game.exe)
#define COLOR_BG RGB(25, 20, 15)
#define COLOR_BG_LIGHT RGB(45, 35, 25)
#define COLOR_TEXT RGB(210, 190, 150)
#define COLOR_ACCENT RGB(180, 140, 60)
#define COLOR_BUTTON RGB(60, 45, 30)
#define COLOR_BUTTON_HOVER RGB(180, 140, 60)
#define COLOR_BORDER RGB(100, 75, 45)
#define COLOR_SUCCESS RGB(255, 220, 0)  // Amarillo para verificar update
#define COLOR_ERROR RGB(200, 80, 80)
#define COLOR_PROGRESS_BG RGB(40, 30, 20)

// ===== CONFIGURACIÓN DE SERVIDORES =====
#define ONLINE_SERVER_IP "89.7.69.125"
#define ONLINE_SERVER_PORT 2500
#define TEST_SERVER_IP "192.168.0.15"
#define TEST_SERVER_PORT 2500

// Clave del registro
#define LAUNCHER_REG_KEY "SOFTWARE\\HelbreathApocalypse\\Launcher"

// ===== VARIABLES GLOBALES =====
HINSTANCE g_hInstance = NULL;
HWND g_hMainWnd = NULL;
BOOL g_bTestServer = FALSE;
HBRUSH g_hBrushBG = NULL;
HBRUSH g_hBrushBGLight = NULL;
HFONT g_hFontTitle = NULL;
HFONT g_hFontNormal = NULL;
HFONT g_hFontButton = NULL;
HFONT g_hFontSmall = NULL;

// ===== CONFIGURACIÓN DE VIDEO D3D11 =====
int g_iRenderer = 2;        // 0=Auto, 1=DDraw, 2=D3D11 Híbrido
int g_iScreenWidth = 1920;
int g_iScreenHeight = 1080;
BOOL g_bVSync = TRUE;
BOOL g_bFullscreen = TRUE;
int g_iScaleMode = 1;       // 0=Point, 1=Bilinear, 2=Integer

// Resoluciones disponibles
struct Resolution {
    int width;
    int height;
    const char* name;
};

Resolution g_Resolutions[] = {
    {800, 600, "800x600 (Nativo)"},
    {1024, 768, "1024x768"},
    {1280, 720, "1280x720 (HD)"},
    {1366, 768, "1366x768"},
    {1600, 900, "1600x900"},
    {1920, 1080, "1920x1080 (Full HD)"},
    {2560, 1440, "2560x1440 (2K)"},
    {3840, 2160, "3840x2160 (4K)"}
};
int g_iNumResolutions = sizeof(g_Resolutions) / sizeof(Resolution);

// Variables del Auto-Update
enum UpdateState {
    STATE_IDLE,
    STATE_CHECKING,
    STATE_DOWNLOADING,
    STATE_INSTALLING,
    STATE_READY,
    STATE_ERROR,
    STATE_OFFLINE
};

UpdateState g_updateState = STATE_IDLE;
char g_szStatusText[256] = "Iniciando...";
char g_szCurrentFile[128] = "";
int g_iProgressCurrent = 0;
int g_iProgressTotal = 0;
BOOL g_bCanPlay = FALSE;
BOOL g_bUpdateInProgress = FALSE;
HANDLE g_hUpdateThread = NULL;

// Progreso de instalación
int g_iInstallCurrent = 0;
int g_iInstallTotal = 0;

// Progreso de descarga de archivo actual (en bytes)
DWORD g_dwDownloadedBytes = 0;
DWORD g_dwTotalFileSize = 0;

// Estado de ambos servidores
BOOL g_bOnlineServerStatus = FALSE;    // Servidor Principal
BOOL g_bTestServerStatus = FALSE;       // Servidor de Pruebas
HANDLE g_hServerCheckThread = NULL;
BOOL g_bServerCheckRunning = FALSE;

// Estructura para archivos a actualizar
struct FileInfo {
    char path[260];
    char hash[33];
    DWORD size;
};
std::vector<FileInfo> g_filesToUpdate;

// Self-update del launcher
BOOL g_bLauncherNeedsUpdate = FALSE;
char g_szNewLauncherHash[33] = "";

// Logo del servidor
Gdiplus::Image* g_pLogoImage = NULL;

// Noticias del servidor
#define MAX_NEWS_LINES 50
#define MAX_NEWS_LINE_LENGTH 256
char g_szNewsLines[MAX_NEWS_LINES][MAX_NEWS_LINE_LENGTH];
int g_iNewsLineCount = 0;
int g_iNewsScrollPos = 0;
BOOL g_bNewsLoaded = FALSE;

// ===== FUNCIONES DE CONFIGURACIÓN =====

// Declaración adelantada
void SaveVideoConfig();

void LoadVideoConfig()
{
    // Verificar si video.cfg existe
    FILE* fp = fopen("video.cfg", "r");
    if (!fp) {
        // No existe, crear uno con valores por defecto
        SaveVideoConfig();
        return;
    }
    
    // Cargar video.cfg existente
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        
        char key[64], value[64];
        if (sscanf(line, "%63[^=]=%63s", key, value) == 2) {
            // Limpiar espacios
            char* k = key;
            while (*k == ' ') k++;
            char* end = k + strlen(k) - 1;
            while (end > k && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
                *end = '\0';
                end--;
            }
            
            if (strcmp(k, "Renderer") == 0) {
                g_iRenderer = atoi(value);
                if (g_iRenderer < 0 || g_iRenderer > 2) g_iRenderer = 2;
            }
            else if (strcmp(k, "ScreenWidth") == 0) {
                g_iScreenWidth = atoi(value);
            }
            else if (strcmp(k, "ScreenHeight") == 0) {
                g_iScreenHeight = atoi(value);
            }
            else if (strcmp(k, "Fullscreen") == 0) {
                g_bFullscreen = (atoi(value) != 0);
            }
            else if (strcmp(k, "VSync") == 0) {
                g_bVSync = (atoi(value) != 0);
            }
            else if (strcmp(k, "ScaleMode") == 0) {
                g_iScaleMode = atoi(value);
                if (g_iScaleMode < 0 || g_iScaleMode > 2) g_iScaleMode = 1;
            }
        }
    }
    fclose(fp);
}

void SaveVideoConfig()
{
    FILE* fp = fopen("video.cfg", "w");
    if (fp) {
        fprintf(fp, "# Helbreath Xtreme - Video Configuration\n");
        fprintf(fp, "# ========================================\n");
        fprintf(fp, "#\n");
        fprintf(fp, "# Renderer Types:\n");
        fprintf(fp, "#   0 = Auto-detect (usa D3D11 si disponible)\n");
        fprintf(fp, "#   1 = DirectDraw (legacy) - Máxima compatibilidad, 800x600 nativo\n");
        fprintf(fp, "#   2 = Direct3D11 HÍBRIDO - DDraw dibuja, D3D11 escala y presenta\n");
        fprintf(fp, "#\n");
        fprintf(fp, "# ========================================\n\n");
        
        fprintf(fp, "# Renderer (0=Auto, 1=DirectDraw, 2=D3D11 Híbrido)\n");
        fprintf(fp, "Renderer=%d\n\n", g_iRenderer);
        
        fprintf(fp, "# Resolución de salida\n");
        fprintf(fp, "ScreenWidth=%d\n", g_iScreenWidth);
        fprintf(fp, "ScreenHeight=%d\n\n", g_iScreenHeight);
        
        fprintf(fp, "# Modo de escalado (0=Point, 1=Bilinear, 2=Integer)\n");
        fprintf(fp, "ScaleMode=%d\n\n", g_iScaleMode);
        
        fprintf(fp, "# Mantener proporción 4:3\n");
        fprintf(fp, "MaintainAspect=1\n\n");
        
        fprintf(fp, "# Pantalla completa (0=Ventana, 1=Pantalla completa)\n");
        fprintf(fp, "Fullscreen=%d\n\n", g_bFullscreen ? 1 : 0);
        
        fprintf(fp, "# VSync (0=Off, 1=On)\n");
        fprintf(fp, "VSync=%d\n\n", g_bVSync ? 1 : 0);
        
        fprintf(fp, "# Calidad gráfica (reservado)\n");
        fprintf(fp, "Quality=2\n");
        
        fclose(fp);
    }
}

void LoadSettings()
{
    HKEY hKey;
    DWORD dwValue, dwSize = sizeof(DWORD);
    
    if (RegOpenKeyEx(HKEY_CURRENT_USER, LAUNCHER_REG_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, "TestServer", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
            g_bTestServer = (dwValue != 0);
        RegCloseKey(hKey);
    }
    
    // Cargar configuración de video desde archivo
    LoadVideoConfig();
}

void SaveSettings()
{
    HKEY hKey;
    DWORD dwDisposition;
    
    if (RegCreateKeyEx(HKEY_CURRENT_USER, LAUNCHER_REG_KEY, 0, NULL, 
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS) {
        DWORD dwValue = g_bTestServer ? 1 : 0;
        RegSetValueEx(hKey, "TestServer", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
        RegCloseKey(hKey);
    }
    
    // Guardar configuración de video a archivo
    SaveVideoConfig();
}

void WriteLoginConfig()
{
    char cPath[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, cPath);
    strcat(cPath, "\\CONTENTS\\LOGIN.CFG");
    
    FILE* fp = fopen(cPath, "w");
    if (fp != NULL) {
        fprintf(fp, "[CONFIG]\n\n");
        if (g_bTestServer) {
            fprintf(fp, "log-server-address  = %s\n", TEST_SERVER_IP);
            fprintf(fp, "log-server-port     = %d\n", TEST_SERVER_PORT);
        } else {
            fprintf(fp, "log-server-address  = %s\n", ONLINE_SERVER_IP);
            fprintf(fp, "log-server-port     = %d\n", ONLINE_SERVER_PORT);
        }
        fprintf(fp, "game-server-port    = 9907\n");
        fprintf(fp, "game-server-mode    = LAN\n");
        fclose(fp);
    }
}

// ===== FUNCIONES DE NOTICIAS =====

void LoadNewsFromFile()
{
    g_iNewsLineCount = 0;
    g_bNewsLoaded = FALSE;
    
    FILE* fp = fopen("news.txt", "r");
    if (!fp) {
        // Crear archivo de noticias por defecto si no existe
        fp = fopen("news.txt", "w");
        if (fp) {
            fprintf(fp, "=== HELBREATH APOCALYPSE ===\n");
            fprintf(fp, "\n");
            fprintf(fp, "Bienvenido al servidor!\n");
            fprintf(fp, "\n");
            fprintf(fp, "--- ULTIMAS ACTUALIZACIONES ---\n");
            fprintf(fp, "\n");
            fprintf(fp, "[v1.0.6] Launcher con logo\n");
            fprintf(fp, "- Logo del servidor en launcher\n");
            fprintf(fp, "- Deteccion inmediata servidor\n");
            fprintf(fp, "\n");
            fprintf(fp, "[v1.0.5] Recursos actualizables\n");
            fprintf(fp, "- Contents, Mapdata, Music\n");
            fprintf(fp, "- Sounds, Sprites\n");
            fprintf(fp, "\n");
            fprintf(fp, "[v1.0.4] Fix Alt+F4\n");
            fprintf(fp, "- Alt+F4 vuelve al menu\n");
            fprintf(fp, "\n");
            fprintf(fp, "[v1.0.3] Version personalizada\n");
            fprintf(fp, "\n");
            fprintf(fp, "[v1.0.2] Volumen guardado\n");
            fprintf(fp, "- Sonido y musica se guardan\n");
            fprintf(fp, "\n");
            fprintf(fp, "[v1.0.1] Barra de progreso\n");
            fprintf(fp, "- Muestra KB/MB descargados\n");
            fprintf(fp, "\n");
            fprintf(fp, "[v1.0.0] Sistema Auto-Update\n");
            fprintf(fp, "- Actualizaciones automaticas\n");
            fprintf(fp, "- Descarga desde GitHub\n");
            fclose(fp);
            fp = fopen("news.txt", "r");
        }
    }
    
    if (fp) {
        char line[MAX_NEWS_LINE_LENGTH];
        while (fgets(line, sizeof(line), fp) && g_iNewsLineCount < MAX_NEWS_LINES) {
            // Eliminar salto de línea
            char* newline = strchr(line, '\n');
            if (newline) *newline = '\0';
            newline = strchr(line, '\r');
            if (newline) *newline = '\0';
            
            strncpy(g_szNewsLines[g_iNewsLineCount], line, MAX_NEWS_LINE_LENGTH - 1);
            g_szNewsLines[g_iNewsLineCount][MAX_NEWS_LINE_LENGTH - 1] = '\0';
            g_iNewsLineCount++;
        }
        fclose(fp);
        g_bNewsLoaded = TRUE;
    }
}

// ===== FUNCIONES DE AUTO-UPDATE =====

char* GetLocalVersion()
{
    static char version[32] = DEFAULT_VERSION;
    FILE* fp = fopen("version.txt", "r");
    if (fp) {
        if (fgets(version, sizeof(version), fp)) {
            // Eliminar salto de línea
            char* newline = strchr(version, '\n');
            if (newline) *newline = '\0';
            newline = strchr(version, '\r');
            if (newline) *newline = '\0';
        }
        fclose(fp);
    }
    return version;
}

void SaveLocalVersion(const char* version)
{
    FILE* fp = fopen("version.txt", "w");
    if (fp) {
        fprintf(fp, "%s", version);
        fclose(fp);
    }
}

BOOL DownloadString(const char* url, char* buffer, int bufferSize)
{
    HINTERNET hInternet = InternetOpen("HelbreathLauncher/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return FALSE;
    
    HINTERNET hUrl = InternetOpenUrl(hInternet, url, NULL, 0, 
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return FALSE;
    }
    
    DWORD bytesRead;
    int totalRead = 0;
    char tempBuf[1024];
    buffer[0] = '\0';
    
    while (InternetReadFile(hUrl, tempBuf, sizeof(tempBuf) - 1, &bytesRead) && bytesRead > 0) {
        tempBuf[bytesRead] = '\0';
        if (totalRead + bytesRead < bufferSize - 1) {
            strcat(buffer, tempBuf);
            totalRead += bytesRead;
        }
    }
    
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    return TRUE;
}

BOOL DownloadFile(const char* url, const char* localPath)
{
    HINTERNET hInternet = InternetOpen("HelbreathLauncher/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return FALSE;
    
    HINTERNET hUrl = InternetOpenUrl(hInternet, url, NULL, 0, 
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return FALSE;
    }
    
    // Obtener tamaño del archivo
    DWORD fileSize = 0;
    DWORD bufferSize = sizeof(fileSize);
    HttpQueryInfo(hUrl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &fileSize, &bufferSize, NULL);
    g_dwTotalFileSize = fileSize;
    g_dwDownloadedBytes = 0;
    
    // Crear directorios si no existen
    char localPathFixed[MAX_PATH];
    strcpy(localPathFixed, localPath);
    // Convertir / a \ para Windows
    for (char* p = localPathFixed; *p; p++) {
        if (*p == '/') *p = '\\';
    }
    
    char dirPath[MAX_PATH];
    strcpy(dirPath, localPathFixed);
    char* lastSlash = strrchr(dirPath, '\\');
    if (lastSlash) {
        *lastSlash = '\0';
        // Crear directorios recursivamente
        char* p = dirPath;
        while (*p) {
            if (*p == '\\') {
                *p = '\0';
                CreateDirectory(dirPath, NULL);
                *p = '\\';
            }
            p++;
        }
        CreateDirectory(dirPath, NULL);
    }
    
    FILE* fp = fopen(localPathFixed, "wb");
    if (!fp) {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return FALSE;
    }
    
    char buffer[8192];
    DWORD bytesRead;
    DWORD lastUIUpdate = GetTickCount();
    
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        fwrite(buffer, 1, bytesRead, fp);
        g_dwDownloadedBytes += bytesRead;
        
        // Actualizar UI cada 100ms para no sobrecargar
        DWORD now = GetTickCount();
        if (now - lastUIUpdate >= 100) {
            lastUIUpdate = now;
            if (g_hMainWnd) {
                // Solo invalidar la zona de la barra de progreso
                RECT rcProgress = {0, 370, LAUNCHER_WIDTH, 440};
                InvalidateRect(g_hMainWnd, &rcProgress, FALSE);
            }
        }
    }
    
    // Actualización final
    if (g_hMainWnd) {
        RECT rcProgress = {0, 370, LAUNCHER_WIDTH, 440};
        InvalidateRect(g_hMainWnd, &rcProgress, FALSE);
    }
    
    fclose(fp);
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    return TRUE;
}

BOOL CalculateMD5(const char* filepath, char* outHash)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        return FALSE;
    
    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return FALSE;
    }
    
    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        outHash[0] = '\0';
        return TRUE; // Archivo no existe, hash vacío
    }
    
    BYTE buffer[8192];
    DWORD bytesRead;
    
    while ((bytesRead = (DWORD)fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        CryptHashData(hHash, buffer, bytesRead, 0);
    }
    fclose(fp);
    
    BYTE hash[16];
    DWORD hashLen = 16;
    
    if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
        for (int i = 0; i < 16; i++) {
            sprintf(outHash + i * 2, "%02x", hash[i]);
        }
        outHash[32] = '\0';
    }
    
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return TRUE;
}

// Parser simple de JSON para patchlist
BOOL ParsePatchList(const char* json, char* outVersion, std::vector<FileInfo>& files)
{
    files.clear();
    
    // Buscar versión
    const char* versionStart = strstr(json, "\"version\"");
    if (versionStart) {
        versionStart = strchr(versionStart, ':');
        if (versionStart) {
            versionStart = strchr(versionStart, '\"');
            if (versionStart) {
                versionStart++;
                const char* versionEnd = strchr(versionStart, '\"');
                if (versionEnd) {
                    int len = versionEnd - versionStart;
                    if (len < 32) {
                        strncpy(outVersion, versionStart, len);
                        outVersion[len] = '\0';
                    }
                }
            }
        }
    }
    
    // Buscar archivos
    const char* filesStart = strstr(json, "\"files\"");
    if (!filesStart) return TRUE;
    
    filesStart = strchr(filesStart, '[');
    if (!filesStart) return TRUE;
    
    const char* pos = filesStart;
    while ((pos = strchr(pos, '{')) != NULL) {
        FileInfo info = {0};
        
        // Buscar path
        const char* pathStart = strstr(pos, "\"path\"");
        if (pathStart && pathStart < strchr(pos, '}')) {
            pathStart = strchr(pathStart + 6, '\"');
            if (pathStart) {
                pathStart++;
                const char* pathEnd = strchr(pathStart, '\"');
                if (pathEnd) {
                    int len = pathEnd - pathStart;
                    if (len < 260) {
                        strncpy(info.path, pathStart, len);
                        info.path[len] = '\0';
                        // Reemplazar / por \ 
                        for (char* p = info.path; *p; p++) {
                            if (*p == '/') *p = '\\';
                        }
                    }
                }
            }
        }
        
        // Buscar hash
        const char* hashStart = strstr(pos, "\"hash\"");
        if (hashStart && hashStart < strchr(pos, '}')) {
            hashStart = strchr(hashStart + 6, '\"');
            if (hashStart) {
                hashStart++;
                const char* hashEnd = strchr(hashStart, '\"');
                if (hashEnd) {
                    int len = hashEnd - hashStart;
                    if (len == 32) {
                        strncpy(info.hash, hashStart, 32);
                        info.hash[32] = '\0';
                    }
                }
            }
        }
        
        // Buscar size
        const char* sizeStart = strstr(pos, "\"size\"");
        if (sizeStart && sizeStart < strchr(pos, '}')) {
            sizeStart = strchr(sizeStart, ':');
            if (sizeStart) {
                info.size = (DWORD)atoi(sizeStart + 1);
            }
        }
        
        if (info.path[0] != '\0') {
            files.push_back(info);
        }
        
        pos = strchr(pos, '}');
        if (!pos) break;
        pos++;
    }
    
    return TRUE;
}

// Función para verificar si el servidor está online usando socket TCP real
BOOL CheckServerOnline(const char* ip, int port)
{
    // Crear socket TCP
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return FALSE;
    }
    
    // Configurar modo no bloqueante
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
    
    // Configurar dirección del servidor
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((u_short)port);
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    
    // Intentar conectar (no bloqueante)
    connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    
    // Esperar con select() - timeout de 2 segundos
    fd_set writeSet, errorSet;
    FD_ZERO(&writeSet);
    FD_ZERO(&errorSet);
    FD_SET(sock, &writeSet);
    FD_SET(sock, &errorSet);
    
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    
    BOOL online = FALSE;
    int result = select(0, NULL, &writeSet, &errorSet, &tv);
    
    if (result > 0 && FD_ISSET(sock, &writeSet)) {
        // Verificar si realmente se conectó
        int error = 0;
        int len = sizeof(error);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&error, &len);
        if (error == 0) {
            online = TRUE;
        }
    }
    
    closesocket(sock);
    return online;
}

// Thread para verificar estado de servidores cada 5 segundos
DWORD WINAPI ServerCheckThread(LPVOID lpParam)
{
    // Verificar inmediatamente al iniciar
    g_bOnlineServerStatus = CheckServerOnline(ONLINE_SERVER_IP, ONLINE_SERVER_PORT);
    g_bTestServerStatus = CheckServerOnline(TEST_SERVER_IP, TEST_SERVER_PORT);
    
    // Redibujar ventana inmediatamente
    if (g_hMainWnd) {
        RECT rcServers = {50, 115, 470, 200};
        InvalidateRect(g_hMainWnd, &rcServers, FALSE);
    }
    
    while (g_bServerCheckRunning) {
        // Esperar 5 segundos antes de la siguiente verificación
        for (int i = 0; i < 50 && g_bServerCheckRunning; i++) {
            Sleep(100);
        }
        
        // Verificar servidor principal
        g_bOnlineServerStatus = CheckServerOnline(ONLINE_SERVER_IP, ONLINE_SERVER_PORT);
        
        // Verificar servidor de pruebas
        g_bTestServerStatus = CheckServerOnline(TEST_SERVER_IP, TEST_SERVER_PORT);
        
        // Redibujar ventana (sin invalidar todo para evitar parpadeo)
        if (g_hMainWnd) {
            // Solo invalidar la zona de los radio buttons
            RECT rcServers = {50, 115, 470, 200};
            InvalidateRect(g_hMainWnd, &rcServers, FALSE);
        }
    }
    return 0;
}

void StartServerCheck()
{
    if (!g_bServerCheckRunning) {
        g_bServerCheckRunning = TRUE;
        g_hServerCheckThread = CreateThread(NULL, 0, ServerCheckThread, NULL, 0, NULL);
    }
}

void StopServerCheck()
{
    g_bServerCheckRunning = FALSE;
    if (g_hServerCheckThread) {
        WaitForSingleObject(g_hServerCheckThread, 3000);
        CloseHandle(g_hServerCheckThread);
        g_hServerCheckThread = NULL;
    }
}

// ===== SELF-UPDATE DEL LAUNCHER =====
// Esta función crea un script batch que:
// 1. Espera a que el launcher se cierre
// 2. Reemplaza el exe viejo con el nuevo
// 3. Reinicia el launcher
BOOL PerformLauncherSelfUpdate()
{
    // Obtener la ruta del launcher actual
    char currentPath[MAX_PATH];
    GetModuleFileName(NULL, currentPath, MAX_PATH);
    
    // Ruta del archivo descargado temporalmente
    char tempPath[MAX_PATH];
    sprintf(tempPath, "%s.new", currentPath);
    
    // Verificar que el archivo nuevo existe
    if (GetFileAttributes(tempPath) == INVALID_FILE_ATTRIBUTES) {
        return FALSE;
    }
    
    // Crear script batch para hacer el reemplazo
    char batchPath[MAX_PATH];
    sprintf(batchPath, "%s.update.bat", currentPath);
    
    FILE* fp = fopen(batchPath, "w");
    if (!fp) return FALSE;
    
    // Script batch con reintentos
    fprintf(fp, "@echo off\n");
    fprintf(fp, "echo Actualizando Launcher...\n");
    fprintf(fp, "echo Por favor espere...\n");
    fprintf(fp, "\n");
    fprintf(fp, ":wait_loop\n");
    fprintf(fp, "timeout /t 1 /nobreak >nul\n");
    fprintf(fp, "del \"%s\" >nul 2>&1\n", currentPath);
    fprintf(fp, "if exist \"%s\" goto wait_loop\n", currentPath);
    fprintf(fp, "\n");
    fprintf(fp, "move /y \"%s\" \"%s\"\n", tempPath, currentPath);
    fprintf(fp, "if errorlevel 1 (\n");
    fprintf(fp, "    echo Error al actualizar el launcher.\n");
    fprintf(fp, "    pause\n");
    fprintf(fp, "    del \"%%~f0\"\n");
    fprintf(fp, "    exit /b 1\n");
    fprintf(fp, ")\n");
    fprintf(fp, "\n");
    fprintf(fp, "echo Actualizacion completada!\n");
    fprintf(fp, "start \"\" \"%s\"\n", currentPath);
    fprintf(fp, "del \"%%~f0\"\n");
    
    fclose(fp);
    
    // Ejecutar el batch de forma oculta
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    // Ejecutar con cmd /c para que funcione el batch
    char cmdLine[MAX_PATH * 2];
    sprintf(cmdLine, "cmd.exe /c \"%s\"", batchPath);
    
    if (CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, 
        CREATE_NO_WINDOW | DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return TRUE;
    }
    
    return FALSE;
}

// Obtener el nombre del exe actual
void GetCurrentExeName(char* outName, int maxLen)
{
    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    const char* fileName = strrchr(path, '\\');
    if (fileName) fileName++;
    else fileName = path;
    strncpy(outName, fileName, maxLen - 1);
    outName[maxLen - 1] = '\0';
}

// Thread de actualización
DWORD WINAPI UpdateThread(LPVOID lpParam)
{
    g_bUpdateInProgress = TRUE;
    g_updateState = STATE_CHECKING;
    strcpy(g_szStatusText, "Verificando actualizaciones...");
    g_bCanPlay = FALSE;
    InvalidateRect(g_hMainWnd, NULL, FALSE);
    
    // Descargar version.txt
    char versionUrl[512];
    sprintf(versionUrl, "%s/version.txt", GITHUB_RAW_URL);
    
    char remoteVersionBuf[64];
    if (!DownloadString(versionUrl, remoteVersionBuf, sizeof(remoteVersionBuf))) {
        g_updateState = STATE_OFFLINE;
        strcpy(g_szStatusText, "Sin conexion - Modo offline");
        g_bCanPlay = TRUE;
        g_bUpdateInProgress = FALSE;
        InvalidateRect(g_hMainWnd, NULL, FALSE);
        return 0;
    }
    
    // Limpiar versión remota
    char* newline = strchr(remoteVersionBuf, '\n');
    if (newline) *newline = '\0';
    newline = strchr(remoteVersionBuf, '\r');
    if (newline) *newline = '\0';
    
    char* localVersion = GetLocalVersion();
    
    // Comparar versiones
    if (strcmp(remoteVersionBuf, localVersion) == 0) {
        g_updateState = STATE_READY;
        strcpy(g_szStatusText, "Cliente actualizado");
        g_bCanPlay = TRUE;
        g_bUpdateInProgress = FALSE;
        InvalidateRect(g_hMainWnd, NULL, FALSE);
        return 0;
    }
    
    sprintf(g_szStatusText, "Nueva version: %s", remoteVersionBuf);
    InvalidateRect(g_hMainWnd, NULL, FALSE);
    
    // Descargar patchlist.json
    char patchlistUrl[512];
    sprintf(patchlistUrl, "%s/patchlist.json", GITHUB_RAW_URL);
    
    char* patchlistBuf = new char[1024 * 1024]; // 1MB buffer
    if (!DownloadString(patchlistUrl, patchlistBuf, 1024 * 1024)) {
        delete[] patchlistBuf;
        g_updateState = STATE_ERROR;
        strcpy(g_szStatusText, "Error al obtener lista de archivos");
        g_bCanPlay = TRUE;
        g_bUpdateInProgress = FALSE;
        InvalidateRect(g_hMainWnd, NULL, FALSE);
        return 0;
    }
    
    // Parsear patchlist
    char patchVersion[32];
    std::vector<FileInfo> allFiles;
    ParsePatchList(patchlistBuf, patchVersion, allFiles);
    delete[] patchlistBuf;
    
    // Obtener el nombre del exe actual (el launcher)
    char currentExeName[MAX_PATH];
    GetCurrentExeName(currentExeName, MAX_PATH);
    
    // Verificar qué archivos necesitan actualización
    g_filesToUpdate.clear();
    g_bLauncherNeedsUpdate = FALSE;
    g_szNewLauncherHash[0] = '\0';
    
    for (size_t i = 0; i < allFiles.size(); i++) {
        char localHash[33];
        CalculateMD5(allFiles[i].path, localHash);
        
        // Verificar si este archivo necesita actualización
        if (_stricmp(localHash, allFiles[i].hash) != 0) {
            // Detectar si es el launcher (HelbreathLauncher.exe)
            const char* fileName = strrchr(allFiles[i].path, '\\');
            if (!fileName) fileName = allFiles[i].path;
            else fileName++;
            
            if (_stricmp(fileName, currentExeName) == 0 || 
                _stricmp(fileName, "HelbreathLauncher.exe") == 0) {
                // El launcher necesita actualizarse - marcarlo para self-update
                g_bLauncherNeedsUpdate = TRUE;
                strcpy(g_szNewLauncherHash, allFiles[i].hash);
            }
            
            // Agregar a la lista de archivos a actualizar
            g_filesToUpdate.push_back(allFiles[i]);
        }
    }
    
    if (g_filesToUpdate.empty()) {
        SaveLocalVersion(remoteVersionBuf);
        g_updateState = STATE_READY;
        strcpy(g_szStatusText, "Cliente actualizado");
        g_bCanPlay = TRUE;
        g_bUpdateInProgress = FALSE;
        InvalidateRect(g_hMainWnd, NULL, FALSE);
        return 0;
    }
    
    // Crear carpeta temporal para descargas
    char tempDir[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, tempDir);
    strcat(tempDir, "\\update_temp");
    CreateDirectory(tempDir, NULL);
    
    // Descargar archivos a carpeta temporal
    g_updateState = STATE_DOWNLOADING;
    g_iProgressTotal = (int)g_filesToUpdate.size();
    g_iInstallTotal = g_iProgressTotal;
    
    // Obtener el nombre del exe actual (el launcher)
    char launcherExeName[MAX_PATH];
    GetCurrentExeName(launcherExeName, MAX_PATH);
    char currentLauncherPath[MAX_PATH];
    GetModuleFileName(NULL, currentLauncherPath, MAX_PATH);
    
    // Vector para guardar rutas temporales
    std::vector<std::string> tempPaths;
    
    for (int i = 0; i < g_iProgressTotal; i++) {
        g_iProgressCurrent = i + 1;
        
        // Obtener solo el nombre del archivo
        const char* fileName = strrchr(g_filesToUpdate[i].path, '\\');
        if (!fileName) fileName = strrchr(g_filesToUpdate[i].path, '/');
        if (!fileName) fileName = g_filesToUpdate[i].path;
        else fileName++;
        
        sprintf(g_szStatusText, "Descargando %d/%d", g_iProgressCurrent, g_iProgressTotal);
        strncpy(g_szCurrentFile, fileName, sizeof(g_szCurrentFile) - 1);
        InvalidateRect(g_hMainWnd, NULL, FALSE);
        
        // Construir URL (reemplazar \ por / para URL)
        char urlPath[260];
        strcpy(urlPath, g_filesToUpdate[i].path);
        for (char* p = urlPath; *p; p++) {
            if (*p == '\\') *p = '/';
        }
        
        char fileUrl[768];
        sprintf(fileUrl, "%s/files/%s", GITHUB_RAW_URL, urlPath);
        
        // Descargar a carpeta temporal
        char tempPath[MAX_PATH];
        sprintf(tempPath, "%s\\%d_%s", tempDir, i, fileName);
        tempPaths.push_back(tempPath);
        
        if (!DownloadFile(fileUrl, tempPath)) {
            g_updateState = STATE_ERROR;
            sprintf(g_szStatusText, "Error descargando: %s", fileName);
            g_bCanPlay = TRUE;
            g_bUpdateInProgress = FALSE;
            InvalidateRect(g_hMainWnd, NULL, FALSE);
            return 0;
        }
    }
    
    // Fase de instalación - copiar de temp a destino
    g_updateState = STATE_INSTALLING;
    g_iInstallCurrent = 0;
    strcpy(g_szStatusText, "Instalando archivos...");
    InvalidateRect(g_hMainWnd, NULL, FALSE);
    
    for (int i = 0; i < g_iProgressTotal; i++) {
        g_iInstallCurrent = i + 1;
        
        // Obtener solo el nombre del archivo para mostrar
        const char* fileName = strrchr(g_filesToUpdate[i].path, '\\');
        if (!fileName) fileName = strrchr(g_filesToUpdate[i].path, '/');
        if (!fileName) fileName = g_filesToUpdate[i].path;
        else fileName++;
        
        sprintf(g_szStatusText, "Instalando %d/%d", g_iInstallCurrent, g_iInstallTotal);
        strncpy(g_szCurrentFile, fileName, sizeof(g_szCurrentFile) - 1);
        InvalidateRect(g_hMainWnd, NULL, FALSE);
        
        // Determinar ruta de destino
        char destPath[MAX_PATH];
        BOOL isLauncherFile = (_stricmp(fileName, launcherExeName) == 0 || 
                               _stricmp(fileName, "HelbreathLauncher.exe") == 0);
        
        if (isLauncherFile) {
            sprintf(destPath, "%s.new", currentLauncherPath);
        } else {
            strcpy(destPath, g_filesToUpdate[i].path);
            // Convertir / a \ para Windows
            for (char* p = destPath; *p; p++) {
                if (*p == '/') *p = '\\';
            }
            
            // Crear directorios si no existen
            char dirPath[MAX_PATH];
            strcpy(dirPath, destPath);
            char* lastSlash = strrchr(dirPath, '\\');
            if (lastSlash) {
                *lastSlash = '\0';
                // Crear directorios recursivamente
                char* p = dirPath;
                while (*p) {
                    if (*p == '\\') {
                        *p = '\0';
                        CreateDirectory(dirPath, NULL);
                        *p = '\\';
                    }
                    p++;
                }
                CreateDirectory(dirPath, NULL);
            }
        }
        
        // Copiar archivo de temp a destino
        if (!CopyFile(tempPaths[i].c_str(), destPath, FALSE)) {
            // Si falla, intentar mover
            if (!MoveFileEx(tempPaths[i].c_str(), destPath, MOVEFILE_REPLACE_EXISTING)) {
                g_updateState = STATE_ERROR;
                sprintf(g_szStatusText, "Error instalando: %s", fileName);
                g_bCanPlay = TRUE;
                g_bUpdateInProgress = FALSE;
                InvalidateRect(g_hMainWnd, NULL, FALSE);
                return 0;
            }
        } else {
            // Eliminar archivo temporal
            DeleteFile(tempPaths[i].c_str());
        }
        
        // Pequeña pausa para que se vea el progreso
        Sleep(50);
    }
    
    // Limpiar carpeta temporal
    RemoveDirectory(tempDir);
    
    // Guardar versión
    SaveLocalVersion(remoteVersionBuf);
    
    // Si el launcher necesita actualizarse, mostrar mensaje especial
    if (g_bLauncherNeedsUpdate) {
        g_updateState = STATE_READY;
        g_szCurrentFile[0] = '\0';
        strcpy(g_szStatusText, "Reiniciar para actualizar Launcher");
        g_bCanPlay = TRUE;  // Permitir jugar o reiniciar
        g_bUpdateInProgress = FALSE;
        InvalidateRect(g_hMainWnd, NULL, FALSE);
        return 0;
    }
    
    // Éxito completo
    g_updateState = STATE_READY;
    g_szCurrentFile[0] = '\0';
    strcpy(g_szStatusText, "Actualizacion completada!");
    g_bCanPlay = TRUE;
    g_bUpdateInProgress = FALSE;
    InvalidateRect(g_hMainWnd, NULL, FALSE);
    
    return 0;
}

void StartUpdateCheck()
{
    if (g_bUpdateInProgress) return;
    g_hUpdateThread = CreateThread(NULL, 0, UpdateThread, NULL, 0, NULL);
}

// ===== FUNCIONES DE UI =====

void DrawCustomButton(HDC hdc, RECT* rc, const char* text, BOOL hover, BOOL isPlayButton, BOOL enabled)
{
    HBRUSH hBrush;
    if (!enabled) {
        hBrush = CreateSolidBrush(RGB(50, 40, 30));
    } else if (isPlayButton) {
        hBrush = CreateSolidBrush(hover ? RGB(200, 160, 70) : COLOR_ACCENT);
    } else {
        hBrush = CreateSolidBrush(hover ? COLOR_BG_LIGHT : COLOR_BUTTON);
    }
    
    SelectObject(hdc, hBrush);
    HPEN hPen = CreatePen(PS_SOLID, 2, enabled ? COLOR_BORDER : RGB(60, 50, 40));
    SelectObject(hdc, hPen);
    RoundRect(hdc, rc->left, rc->top, rc->right, rc->bottom, 6, 6);
    DeleteObject(hBrush);
    DeleteObject(hPen);
    
    if (isPlayButton && enabled) {
        HPEN hPenGold = CreatePen(PS_SOLID, 1, RGB(220, 180, 80));
        SelectObject(hdc, hPenGold);
        RECT rcInner = {rc->left + 3, rc->top + 3, rc->right - 3, rc->bottom - 3};
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        RoundRect(hdc, rcInner.left, rcInner.top, rcInner.right, rcInner.bottom, 4, 4);
        DeleteObject(hPenGold);
    }
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(20, 15, 10));
    RECT rcShadow = {rc->left + 1, rc->top + 1, rc->right + 1, rc->bottom + 1};
    SelectObject(hdc, g_hFontButton);
    DrawText(hdc, text, -1, &rcShadow, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SetTextColor(hdc, enabled ? COLOR_TEXT : RGB(100, 80, 60));
    DrawText(hdc, text, -1, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hRadioOnline = NULL;
    static HWND hRadioTest = NULL;
    static HWND hComboRenderer = NULL;
    static HWND hComboResolution = NULL;
    static HWND hComboScaleMode = NULL;
    static BOOL bPlayHover = FALSE;
    static BOOL bExitHover = FALSE;
    
    switch (message) {
    case WM_CREATE:
    {
        // Crear fuentes
        g_hFontTitle = CreateFont(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DECORATIVE, "Times New Roman");
        g_hFontNormal = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        g_hFontButton = CreateFont(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        g_hFontSmall = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        
        g_hBrushBG = CreateSolidBrush(COLOR_BG);
        g_hBrushBGLight = CreateSolidBrush(COLOR_BG_LIGHT);
        
        // Cargar noticias
        LoadNewsFromFile();
        
        int centerX = LEFT_PANEL_WIDTH / 2;
        int contentY = 120;
        
        // Radio buttons (panel izquierdo)
        hRadioOnline = CreateWindow("BUTTON", "  Servidor Principal",
            WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,
            centerX - 100, contentY, 190, 25, hWnd, (HMENU)ID_RADIO_ONLINE, NULL, NULL);
        SendMessage(hRadioOnline, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        
        hRadioTest = CreateWindow("BUTTON", "  Servidor de Pruebas",
            WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
            centerX - 100, contentY + 28, 190, 25, hWnd, (HMENU)ID_RADIO_TEST, NULL, NULL);
        SendMessage(hRadioTest, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        
        SendMessage(g_bTestServer ? hRadioTest : hRadioOnline, BM_SETCHECK, BST_CHECKED, 0);
        
        // ===== SECCIÓN DE VIDEO D3D11 (panel izquierdo) =====
        int videoY = 225;
        int labelWidth = 100;
        int comboWidth = 160;
        int comboX = centerX - 30;
        
        // Combo Renderer (alineado a la derecha de la etiqueta)
        hComboRenderer = CreateWindow("COMBOBOX", NULL,
            WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
            comboX, videoY, comboWidth, 200, hWnd, (HMENU)ID_COMBO_RENDERER, NULL, NULL);
        SendMessage(hComboRenderer, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);
        SendMessage(hComboRenderer, CB_ADDSTRING, 0, (LPARAM)"Auto-detectar");
        SendMessage(hComboRenderer, CB_ADDSTRING, 0, (LPARAM)"DirectDraw (Legacy)");
        SendMessage(hComboRenderer, CB_ADDSTRING, 0, (LPARAM)"D3D11 Hibrido");
        SendMessage(hComboRenderer, CB_SETCURSEL, g_iRenderer, 0);
        
        // Combo Resolución
        hComboResolution = CreateWindow("COMBOBOX", NULL,
            WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
            comboX, videoY + 30, comboWidth, 200, hWnd, (HMENU)ID_COMBO_RESOLUTION, NULL, NULL);
        SendMessage(hComboResolution, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);
        
        int selectedRes = 0;
        for (int i = 0; i < g_iNumResolutions; i++) {
            SendMessage(hComboResolution, CB_ADDSTRING, 0, (LPARAM)g_Resolutions[i].name);
            if (g_Resolutions[i].width == g_iScreenWidth && g_Resolutions[i].height == g_iScreenHeight) {
                selectedRes = i;
            }
        }
        SendMessage(hComboResolution, CB_SETCURSEL, selectedRes, 0);
        
        // Combo Scale Mode
        hComboScaleMode = CreateWindow("COMBOBOX", NULL,
            WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
            comboX, videoY + 60, comboWidth, 200, hWnd, (HMENU)ID_COMBO_SCALEMODE, NULL, NULL);
        SendMessage(hComboScaleMode, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);
        SendMessage(hComboScaleMode, CB_ADDSTRING, 0, (LPARAM)"Point (Pixelado)");
        SendMessage(hComboScaleMode, CB_ADDSTRING, 0, (LPARAM)"Bilinear (Suave)");
        SendMessage(hComboScaleMode, CB_ADDSTRING, 0, (LPARAM)"Integer (Pixel Perfect)");
        SendMessage(hComboScaleMode, CB_SETCURSEL, g_iScaleMode, 0);
        
        // Timer para actualizar UI (menos frecuente para reducir parpadeo)
        SetTimer(hWnd, 1, 250, NULL);
        
        // Iniciar verificación de estado de servidores
        StartServerCheck();
        
        // Iniciar verificación de actualizaciones
        StartUpdateCheck();
        
        return 0;
    }
    
    case WM_TIMER:
    {
        // Solo invalidar si hay cambios en el estado de actualización
        static UpdateState lastState = STATE_IDLE;
        static int lastProgress = 0;
        
        if (g_updateState != lastState || g_iProgressCurrent != lastProgress) {
            lastState = g_updateState;
            lastProgress = g_iProgressCurrent;
            // Invalidar solo la zona de estado
            RECT rcStatus = {20, 350, LAUNCHER_WIDTH - 20, 450};
            InvalidateRect(hWnd, &rcStatus, FALSE);
        }
        return 0;
    }
    
    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, COLOR_TEXT);
        SetBkColor(hdcStatic, COLOR_BG);
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)g_hBrushBG;
    }
    
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        
        int width = LAUNCHER_WIDTH;
        int height = LAUNCHER_HEIGHT;
        
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, width, height);
        SelectObject(hdcMem, hbmMem);
        
        // Fondo
        RECT rcClient = {0, 0, width, height};
        FillRect(hdcMem, &rcClient, g_hBrushBG);
        
        // Borde decorativo
        HPEN hPenBorder = CreatePen(PS_SOLID, 3, COLOR_BORDER);
        HPEN hPenGold = CreatePen(PS_SOLID, 1, COLOR_ACCENT);
        SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
        SelectObject(hdcMem, hPenBorder);
        Rectangle(hdcMem, 2, 2, width - 2, height - 2);
        SelectObject(hdcMem, hPenGold);
        Rectangle(hdcMem, 6, 6, width - 6, height - 6);
        
        SetBkMode(hdcMem, TRANSPARENT);
        
        // Título del servidor (texto grande)
        SelectObject(hdcMem, g_hFontTitle);
        SetTextColor(hdcMem, COLOR_ACCENT);
        RECT rcTitle = {0, 25, LEFT_PANEL_WIDTH, 75};
        DrawText(hdcMem, "HELBREATH", -1, &rcTitle, DT_CENTER | DT_SINGLELINE);
        RECT rcTitle2 = {0, 55, LEFT_PANEL_WIDTH, 95};
        DrawText(hdcMem, "APOCALYPSE", -1, &rcTitle2, DT_CENTER | DT_SINGLELINE);
        
        // Línea decorativa (panel izquierdo)
        SelectObject(hdcMem, hPenGold);
        int lineY = 100;
        MoveToEx(hdcMem, 20, lineY, NULL);
        LineTo(hdcMem, LEFT_PANEL_WIDTH - 20, lineY);
        
        // Etiqueta servidor (panel izquierdo)
        SetTextColor(hdcMem, COLOR_ACCENT);
        RECT rcServerLabel = {0, 105, LEFT_PANEL_WIDTH, 125};
        DrawText(hdcMem, "- Seleccionar Servidor -", -1, &rcServerLabel, DT_CENTER | DT_SINGLELINE);
        
        // ===== INDICADORES DE ESTADO DE SERVIDORES =====
        int contentY = 120;
        int statusX = LEFT_PANEL_WIDTH / 2 + 95;  // A la derecha de los radio buttons
        SelectObject(hdcMem, g_hFontSmall);
        
        // Estado Servidor Principal
        if (g_bOnlineServerStatus) {
            SetTextColor(hdcMem, COLOR_SUCCESS);
            RECT rcOnlineStatus = {statusX, contentY + 3, statusX + 80, contentY + 20};
            DrawText(hdcMem, "[ONLINE]", -1, &rcOnlineStatus, DT_LEFT | DT_SINGLELINE);
        } else {
            SetTextColor(hdcMem, COLOR_ERROR);
            RECT rcOnlineStatus = {statusX, contentY + 3, statusX + 80, contentY + 20};
            DrawText(hdcMem, "[OFFLINE]", -1, &rcOnlineStatus, DT_LEFT | DT_SINGLELINE);
        }
        
        // Estado Servidor de Pruebas
        if (g_bTestServerStatus) {
            SetTextColor(hdcMem, COLOR_SUCCESS);
            RECT rcTestStatus = {statusX, contentY + 31, statusX + 80, contentY + 48};
            DrawText(hdcMem, "[ONLINE]", -1, &rcTestStatus, DT_LEFT | DT_SINGLELINE);
        } else {
            SetTextColor(hdcMem, COLOR_ERROR);
            RECT rcTestStatus = {statusX, contentY + 31, statusX + 80, contentY + 48};
            DrawText(hdcMem, "[OFFLINE]", -1, &rcTestStatus, DT_LEFT | DT_SINGLELINE);
        }
        
        // Línea opciones (panel izquierdo)
        int optionsLineY = 200;
        MoveToEx(hdcMem, 20, optionsLineY, NULL);
        LineTo(hdcMem, LEFT_PANEL_WIDTH - 20, optionsLineY);
        
        // Etiqueta opciones (panel izquierdo)
        SetTextColor(hdcMem, COLOR_ACCENT);  // Asegurar color dorado
        RECT rcOptionsLabel = {0, 205, LEFT_PANEL_WIDTH, 225};
        DrawText(hdcMem, "- Opciones de Video -", -1, &rcOptionsLabel, DT_CENTER | DT_SINGLELINE);
        
        // ===== ETIQUETAS DE VIDEO =====
        int videoY = 225;
        int labelX = 60;
        SelectObject(hdcMem, g_hFontSmall);
        SetTextColor(hdcMem, COLOR_TEXT);
        
        RECT rcLblRenderer = {labelX, videoY + 3, labelX + 100, videoY + 20};
        DrawText(hdcMem, "Renderer:", -1, &rcLblRenderer, DT_LEFT | DT_SINGLELINE);
        
        RECT rcLblResolution = {labelX, videoY + 33, labelX + 100, videoY + 50};
        DrawText(hdcMem, "Resolucion:", -1, &rcLblResolution, DT_LEFT | DT_SINGLELINE);
        
        RECT rcLblScale = {labelX, videoY + 63, labelX + 100, videoY + 80};
        DrawText(hdcMem, "Escalado:", -1, &rcLblScale, DT_LEFT | DT_SINGLELINE);
        
        // Línea separadora antes del status de update (panel izquierdo)
        SelectObject(hdcMem, hPenGold);
        int updateLineY = 345;
        MoveToEx(hdcMem, 20, updateLineY, NULL);
        LineTo(hdcMem, LEFT_PANEL_WIDTH - 20, updateLineY);
        
        // Etiqueta actualizaciones (panel izquierdo)
        SetTextColor(hdcMem, COLOR_ACCENT);
        RECT rcUpdateLabel = {0, 350, LEFT_PANEL_WIDTH, 370};
        DrawText(hdcMem, "- Estado -", -1, &rcUpdateLabel, DT_CENTER | DT_SINGLELINE);
        
        // ===== PANEL DE NOTICIAS (lado derecho) =====
        int newsX = LEFT_PANEL_WIDTH + 10;
        int newsY = 15;
        int newsWidth = width - LEFT_PANEL_WIDTH - 25;
        int newsHeight = height - 30;
        
        // Borde del panel de noticias
        RECT rcNewsPanel = {newsX, newsY, newsX + newsWidth, newsY + newsHeight};
        HBRUSH hBrushNews = CreateSolidBrush(RGB(20, 16, 12));
        FillRect(hdcMem, &rcNewsPanel, hBrushNews);
        DeleteObject(hBrushNews);
        
        SelectObject(hdcMem, hPenGold);
        Rectangle(hdcMem, newsX, newsY, newsX + newsWidth, newsY + newsHeight);
        
        // Título de noticias
        SetTextColor(hdcMem, COLOR_ACCENT);
        SelectObject(hdcMem, g_hFontNormal);
        RECT rcNewsTitle = {newsX, newsY + 5, newsX + newsWidth, newsY + 25};
        DrawText(hdcMem, "- NOTICIAS -", -1, &rcNewsTitle, DT_CENTER | DT_SINGLELINE);
        
        // Línea bajo el título
        MoveToEx(hdcMem, newsX + 10, newsY + 28, NULL);
        LineTo(hdcMem, newsX + newsWidth - 10, newsY + 28);
        
        // Contenido de noticias
        SelectObject(hdcMem, g_hFontSmall);
        int newsLineY = newsY + 35;
        int newsLineHeight = 14;
        int maxVisibleLines = (newsHeight - 45) / newsLineHeight;
        
        for (int i = 0; i < g_iNewsLineCount && i < maxVisibleLines; i++) {
            const char* line = g_szNewsLines[i];
            
            // Colorear según el tipo de línea
            if (line[0] == '=' || line[0] == '-') {
                SetTextColor(hdcMem, COLOR_ACCENT);  // Títulos en dorado
            } else if (line[0] == '[') {
                SetTextColor(hdcMem, COLOR_SUCCESS);  // Versiones en verde
            } else {
                SetTextColor(hdcMem, COLOR_TEXT);  // Texto normal
            }
            
            RECT rcNewsLine = {newsX + 10, newsLineY, newsX + newsWidth - 10, newsLineY + newsLineHeight};
            DrawText(hdcMem, line, -1, &rcNewsLine, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
            newsLineY += newsLineHeight;
        }
        
        DeleteObject(hPenBorder);
        DeleteObject(hPenGold);
        
        // ===== SECCIÓN DE ACTUALIZACIONES (panel izquierdo) =====
        int updateY = 375;
        
        // Estado de actualización
        SelectObject(hdcMem, g_hFontNormal);
        COLORREF statusColor = COLOR_TEXT;
        if (g_updateState == STATE_READY) {
            statusColor = COLOR_SUCCESS;
        }
        else if (g_updateState == STATE_ERROR) statusColor = COLOR_ERROR;
        else if (g_updateState == STATE_OFFLINE) statusColor = RGB(180, 160, 100);
        
        SetTextColor(hdcMem, statusColor);
        RECT rcStatus = {10, updateY, LEFT_PANEL_WIDTH - 10, updateY + 20};
        DrawText(hdcMem, g_szStatusText, -1, &rcStatus, DT_CENTER | DT_SINGLELINE);
        
        // Barra de progreso (si está descargando)
        if (g_updateState == STATE_DOWNLOADING && g_iProgressTotal > 0) {
            int barY = updateY + 25;
            int barWidth = LEFT_PANEL_WIDTH - 80;
            int barHeight = 16;  // Más alta para mejor visibilidad
            int barX = (LEFT_PANEL_WIDTH - barWidth) / 2;
            
            // Fondo de la barra
            RECT rcBarBG = {barX, barY, barX + barWidth, barY + barHeight};
            HBRUSH hBrushBarBG = CreateSolidBrush(COLOR_PROGRESS_BG);
            FillRect(hdcMem, &rcBarBG, hBrushBarBG);
            DeleteObject(hBrushBarBG);
            
            // Calcular progreso basado en bytes del archivo actual
            int progressWidth = 0;
            if (g_dwTotalFileSize > 0) {
                progressWidth = (int)(((__int64)barWidth * g_dwDownloadedBytes) / g_dwTotalFileSize);
            }
            
            // Dibujar barra de progreso
            if (progressWidth > 0) {
                RECT rcBar = {barX, barY, barX + progressWidth, barY + barHeight};
                HBRUSH hBrushBar = CreateSolidBrush(COLOR_ACCENT);
                FillRect(hdcMem, &rcBar, hBrushBar);
                DeleteObject(hBrushBar);
            }
            
            // Borde de la barra
            HPEN hPenBar = CreatePen(PS_SOLID, 1, COLOR_BORDER);
            SelectObject(hdcMem, hPenBar);
            SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
            Rectangle(hdcMem, barX, barY, barX + barWidth, barY + barHeight);
            DeleteObject(hPenBar);
            
            // Texto de progreso (porcentaje y KB/MB)
            char progressText[128];
            if (g_dwTotalFileSize > 0) {
                int percent = (int)(((__int64)100 * g_dwDownloadedBytes) / g_dwTotalFileSize);
                if (g_dwTotalFileSize >= 1024 * 1024) {
                    // Mostrar en MB
                    sprintf(progressText, "%d%% - %.1f / %.1f MB", 
                        percent,
                        g_dwDownloadedBytes / (1024.0 * 1024.0),
                        g_dwTotalFileSize / (1024.0 * 1024.0));
                } else {
                    // Mostrar en KB
                    sprintf(progressText, "%d%% - %d / %d KB", 
                        percent,
                        g_dwDownloadedBytes / 1024,
                        g_dwTotalFileSize / 1024);
                }
            } else {
                sprintf(progressText, "Descargando...");
            }
            
            // Dibujar texto del progreso centrado en la barra
            SelectObject(hdcMem, g_hFontSmall);
            SetTextColor(hdcMem, RGB(255, 255, 255));
            SetBkMode(hdcMem, TRANSPARENT);
            RECT rcProgressText = {barX, barY, barX + barWidth, barY + barHeight};
            DrawText(hdcMem, progressText, -1, &rcProgressText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            // Nombre archivo debajo de la barra
            SetTextColor(hdcMem, RGB(150, 130, 100));
            RECT rcFile = {10, barY + barHeight + 5, LEFT_PANEL_WIDTH - 10, barY + barHeight + 20};
            char fileInfo[256];
            sprintf(fileInfo, "%s (%d/%d)", g_szCurrentFile, g_iProgressCurrent, g_iProgressTotal);
            DrawText(hdcMem, fileInfo, -1, &rcFile, DT_CENTER | DT_SINGLELINE);
        }
        
        // Barra de instalación (si está instalando)
        if (g_updateState == STATE_INSTALLING && g_iInstallTotal > 0) {
            int barY = updateY + 25;
            int barWidth = LEFT_PANEL_WIDTH - 80;
            int barHeight = 16;
            int barX = (LEFT_PANEL_WIDTH - barWidth) / 2;
            
            // Fondo de la barra
            RECT rcBarBG = {barX, barY, barX + barWidth, barY + barHeight};
            HBRUSH hBrushBarBG = CreateSolidBrush(COLOR_PROGRESS_BG);
            FillRect(hdcMem, &rcBarBG, hBrushBarBG);
            DeleteObject(hBrushBarBG);
            
            // Calcular progreso de instalación
            int progressWidth = 0;
            if (g_iInstallTotal > 0) {
                progressWidth = (barWidth * g_iInstallCurrent) / g_iInstallTotal;
            }
            
            // Dibujar barra de progreso
            if (progressWidth > 0) {
                RECT rcBar = {barX, barY, barX + progressWidth, barY + barHeight};
                HBRUSH hBrushBar = CreateSolidBrush(RGB(50, 180, 50)); // Verde para instalación
                FillRect(hdcMem, &rcBar, hBrushBar);
                DeleteObject(hBrushBar);
            }
            
            // Borde de la barra
            HPEN hPenBar = CreatePen(PS_SOLID, 1, COLOR_BORDER);
            SelectObject(hdcMem, hPenBar);
            SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
            Rectangle(hdcMem, barX, barY, barX + barWidth, barY + barHeight);
            DeleteObject(hPenBar);
            
            // Texto de progreso
            char progressText[128];
            int percent = 0;
            if (g_iInstallTotal > 0) {
                percent = (100 * g_iInstallCurrent) / g_iInstallTotal;
            }
            sprintf(progressText, "%d%% - %d / %d archivos", percent, g_iInstallCurrent, g_iInstallTotal);
            
            // Dibujar texto del progreso centrado en la barra
            SelectObject(hdcMem, g_hFontSmall);
            SetTextColor(hdcMem, RGB(255, 255, 255));
            SetBkMode(hdcMem, TRANSPARENT);
            RECT rcProgressText = {barX, barY, barX + barWidth, barY + barHeight};
            DrawText(hdcMem, progressText, -1, &rcProgressText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            // Nombre archivo debajo de la barra
            SetTextColor(hdcMem, RGB(150, 130, 100));
            RECT rcFile = {10, barY + barHeight + 5, LEFT_PANEL_WIDTH - 10, barY + barHeight + 20};
            char fileInfo[256];
            sprintf(fileInfo, "%s", g_szCurrentFile);
            DrawText(hdcMem, fileInfo, -1, &rcFile, DT_CENTER | DT_SINGLELINE);
        }
        
        // Versión (panel izquierdo)
        SelectObject(hdcMem, g_hFontSmall);
        SetTextColor(hdcMem, RGB(100, 80, 60));
        char versionText[64];
        sprintf(versionText, "v%s", GetLocalVersion());
        RECT rcVersion = {0, height - 25, LEFT_PANEL_WIDTH, height - 10};
        DrawText(hdcMem, versionText, -1, &rcVersion, DT_CENTER | DT_SINGLELINE);
        
        // Botones (panel izquierdo)
        int btnWidth = 180;
        int btnHeight = 45;
        int btnY = height - 90;
        int btnExitY = height - 35;
        
        RECT rcPlay = {(LEFT_PANEL_WIDTH - btnWidth) / 2, btnY, (LEFT_PANEL_WIDTH + btnWidth) / 2, btnY + btnHeight};
        // Cambiar texto del botón si el launcher necesita reiniciarse
        const char* playButtonText = g_bLauncherNeedsUpdate ? "REINICIAR" : "ENTRAR AL REINO";
        DrawCustomButton(hdcMem, &rcPlay, playButtonText, bPlayHover, TRUE, g_bCanPlay);
        
        int exitBtnWidth = 100;
        RECT rcExit = {(LEFT_PANEL_WIDTH - exitBtnWidth) / 2, btnExitY, (LEFT_PANEL_WIDTH + exitBtnWidth) / 2, btnExitY + 25};
        DrawCustomButton(hdcMem, &rcExit, "Salir", bExitHover, FALSE, TRUE);
        
        BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        
        EndPaint(hWnd, &ps);
        return 0;
    }
    
    case WM_MOUSEMOVE:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        
        int btnWidth = 180;
        int btnHeight = 45;
        int btnY = LAUNCHER_HEIGHT - 90;
        int btnExitY = LAUNCHER_HEIGHT - 35;
        int exitBtnWidth = 100;
        
        BOOL newPlayHover = (x >= (LEFT_PANEL_WIDTH - btnWidth) / 2 && x <= (LEFT_PANEL_WIDTH + btnWidth) / 2 && 
                            y >= btnY && y <= btnY + btnHeight);
        BOOL newExitHover = (x >= (LEFT_PANEL_WIDTH - exitBtnWidth) / 2 && x <= (LEFT_PANEL_WIDTH + exitBtnWidth) / 2 && 
                            y >= btnExitY && y <= btnExitY + 25);
        
        if (newPlayHover != bPlayHover || newExitHover != bExitHover) {
            bPlayHover = newPlayHover;
            bExitHover = newExitHover;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }
    
    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        
        int btnWidth = 180;
        int btnHeight = 45;
        int btnY = LAUNCHER_HEIGHT - 90;
        int btnExitY = LAUNCHER_HEIGHT - 35;
        int exitBtnWidth = 100;
        
        // Click en JUGAR (o REINICIAR si hay actualización del launcher pendiente)
        if (g_bCanPlay && x >= (LEFT_PANEL_WIDTH - btnWidth) / 2 && x <= (LEFT_PANEL_WIDTH + btnWidth) / 2 && 
            y >= btnY && y <= btnY + btnHeight) {
            
            // Si el launcher necesita actualizarse, hacer self-update y reiniciar
            if (g_bLauncherNeedsUpdate) {
                // Guardar configuración antes de reiniciar
                g_iRenderer = (int)SendMessage(hComboRenderer, CB_GETCURSEL, 0, 0);
                int resSel = (int)SendMessage(hComboResolution, CB_GETCURSEL, 0, 0);
                if (resSel >= 0 && resSel < g_iNumResolutions) {
                    g_iScreenWidth = g_Resolutions[resSel].width;
                    g_iScreenHeight = g_Resolutions[resSel].height;
                }
                g_iScaleMode = (int)SendMessage(hComboScaleMode, CB_GETCURSEL, 0, 0);
                g_bTestServer = (SendMessage(hRadioTest, BM_GETCHECK, 0, 0) == BST_CHECKED);
                SaveSettings();
                
                // Verificar que existe el archivo .new y ejecutar self-update
                char currentPath[MAX_PATH];
                GetModuleFileName(NULL, currentPath, MAX_PATH);
                char newPath[MAX_PATH];
                sprintf(newPath, "%s.new", currentPath);
                
                if (GetFileAttributes(newPath) != INVALID_FILE_ATTRIBUTES) {
                    if (PerformLauncherSelfUpdate()) {
                        DestroyWindow(hWnd);
                        return 0;
                    } else {
                        MessageBox(hWnd, "Error al preparar la actualizacion.\nInténtalo de nuevo.", "Error", MB_OK | MB_ICONERROR);
                    }
                } else {
                    MessageBox(hWnd, "Archivo de actualizacion no encontrado.\nReinicia el launcher.", "Error", MB_OK | MB_ICONERROR);
                }
                return 0;
            }
            
            // Leer estado de los controles de servidor
            g_bTestServer = (SendMessage(hRadioTest, BM_GETCHECK, 0, 0) == BST_CHECKED);
            
            // Leer configuración de video
            g_iRenderer = (int)SendMessage(hComboRenderer, CB_GETCURSEL, 0, 0);
            int resSel = (int)SendMessage(hComboResolution, CB_GETCURSEL, 0, 0);
            if (resSel >= 0 && resSel < g_iNumResolutions) {
                g_iScreenWidth = g_Resolutions[resSel].width;
                g_iScreenHeight = g_Resolutions[resSel].height;
            }
            g_iScaleMode = (int)SendMessage(hComboScaleMode, CB_GETCURSEL, 0, 0);
            
            SaveSettings();
            WriteLoginConfig();
            
            // Ejecutar Game.exe con -nolauncher
            STARTUPINFO si = {sizeof(si)};
            PROCESS_INFORMATION pi;
            
            char cmdLine[MAX_PATH];
            strcpy(cmdLine, "Game.exe -nolauncher");
            
            if (CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                DestroyWindow(hWnd);
            } else {
                MessageBox(hWnd, "No se pudo iniciar Game.exe\n\nAsegurate de que el launcher este en la misma carpeta que el juego.", "Error", MB_OK | MB_ICONERROR);
            }
        }
        // Click en Salir
        else if (x >= (LEFT_PANEL_WIDTH - exitBtnWidth) / 2 && x <= (LEFT_PANEL_WIDTH + exitBtnWidth) / 2 && 
                 y >= btnExitY && y <= btnExitY + 25) {
            // Guardar configuración de video antes de salir
            g_iRenderer = (int)SendMessage(hComboRenderer, CB_GETCURSEL, 0, 0);
            int resSel = (int)SendMessage(hComboResolution, CB_GETCURSEL, 0, 0);
            if (resSel >= 0 && resSel < g_iNumResolutions) {
                g_iScreenWidth = g_Resolutions[resSel].width;
                g_iScreenHeight = g_Resolutions[resSel].height;
            }
            g_iScaleMode = (int)SendMessage(hComboScaleMode, CB_GETCURSEL, 0, 0);
            g_bTestServer = (SendMessage(hRadioTest, BM_GETCHECK, 0, 0) == BST_CHECKED);
            
            SaveSettings();
            DestroyWindow(hWnd);
        }
        return 0;
    }
    
    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            ScreenToClient(hWnd, &pt);
            if (pt.y < 30) return HTCAPTION;
        }
        return hit;
    }
    
    case WM_CLOSE:
        // Si el launcher necesita actualizarse, ejecutar self-update
        if (g_bLauncherNeedsUpdate) {
            // Verificar que existe el archivo .new
            char currentPath[MAX_PATH];
            GetModuleFileName(NULL, currentPath, MAX_PATH);
            char newPath[MAX_PATH];
            sprintf(newPath, "%s.new", currentPath);
            
            if (GetFileAttributes(newPath) != INVALID_FILE_ATTRIBUTES) {
                if (PerformLauncherSelfUpdate()) {
                    // El batch se encargará de reiniciar
                    DestroyWindow(hWnd);
                    return 0;
                }
            }
        }
        DestroyWindow(hWnd);
        return 0;
    
    case WM_DESTROY:
        KillTimer(hWnd, 1);
        StopServerCheck();  // Detener thread de verificación de servidores
        if (g_hFontTitle) DeleteObject(g_hFontTitle);
        if (g_hFontNormal) DeleteObject(g_hFontNormal);
        if (g_hFontButton) DeleteObject(g_hFontButton);
        if (g_hFontSmall) DeleteObject(g_hFontSmall);
        if (g_hBrushBG) DeleteObject(g_hBrushBG);
        if (g_hBrushBGLight) DeleteObject(g_hBrushBGLight);
        PostQuitMessage(0);
        return 0;
    
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;
    
    // Inicializar GDI+ para cargar imágenes PNG
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    // Cargar imagen del logo
    g_pLogoImage = Gdiplus::Image::FromFile(L"Logo_Server.png");
    if (g_pLogoImage && g_pLogoImage->GetLastStatus() != Gdiplus::Ok) {
        delete g_pLogoImage;
        g_pLogoImage = NULL;
    }
    
    // Inicializar Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    // Cargar configuración
    LoadSettings();
    
    // Inicializar controles comunes
    INITCOMMONCONTROLSEX icex = {sizeof(icex), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icex);
    
    // Registrar clase
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "HelbreathLauncher";
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Error al registrar ventana", "Error", MB_OK);
        return 0;
    }
    
    // Calcular posición centrada
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - LAUNCHER_WIDTH) / 2;
    int posY = (screenH - LAUNCHER_HEIGHT) / 2;
    
    // Crear ventana
    g_hMainWnd = CreateWindowEx(
        WS_EX_LAYERED,
        "HelbreathLauncher",
        "Helbreath Launcher",
        WS_POPUP | WS_VISIBLE,
        posX, posY, LAUNCHER_WIDTH, LAUNCHER_HEIGHT,
        NULL, NULL, hInstance, NULL);
    
    if (!g_hMainWnd) {
        MessageBox(NULL, "Error al crear ventana", "Error", MB_OK);
        return 0;
    }
    
    SetLayeredWindowAttributes(g_hMainWnd, 0, 245, LWA_ALPHA);
    
    // Loop de mensajes
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Limpiar imagen del logo
    if (g_pLogoImage) {
        delete g_pLogoImage;
        g_pLogoImage = NULL;
    }
    
    // Cerrar GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);
    
    // Limpiar Winsock
    WSACleanup();
    
    return (int)msg.wParam;
}
