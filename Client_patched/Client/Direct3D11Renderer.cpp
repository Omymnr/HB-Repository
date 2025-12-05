// Direct3D11Renderer.cpp - Implementación completa del renderer Direct3D 11
// Renderer moderno para Windows 10/11
//
// Este archivo implementa un sistema de renderizado completo usando Direct3D 11,
// reemplazando el antiguo DirectDraw 7 con tecnología moderna.

#include "Direct3D11Renderer.h"
#include "Sprite.h"
#include <d3dcompiler.h>

// Shaders embebidos (compilados en runtime para simplicidad)
// En producción podrían pre-compilarse

static const char* g_szVertexShader = R"(
cbuffer ShaderConstants : register(b0)
{
    matrix Projection;
    float4 GlobalTint;
    float GlobalAlpha;
    float3 Padding;
};

struct VS_INPUT
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.Position = mul(float4(input.Position, 1.0f), Projection);
    output.TexCoord = input.TexCoord;
    output.Color = input.Color * GlobalTint;
    output.Color.a *= GlobalAlpha;
    return output;
}
)";

static const char* g_szPixelShader = R"(
Texture2D SpriteTexture : register(t0);
SamplerState SpriteSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 texColor = SpriteTexture.Sample(SpriteSampler, input.TexCoord);
    if (texColor.a < 0.01f)
    {
        discard;
    }
    return texColor * input.Color;
}
)";

// ==================== CONSTRUCTOR / DESTRUCTOR ====================

Direct3D11Renderer::Direct3D11Renderer()
    : m_pDevice(NULL)
    , m_pContext(NULL)
    , m_pSwapChain(NULL)
    , m_pRenderTargetView(NULL)
    , m_pDepthStencilView(NULL)
    , m_pDepthStencil(NULL)
    , m_pVertexBuffer(NULL)
    , m_pIndexBuffer(NULL)
    , m_pConstantBuffer(NULL)
    , m_pInputLayout(NULL)
    , m_pVertexShader(NULL)
    , m_pPixelShader(NULL)
    , m_pSamplerPoint(NULL)
    , m_pSamplerLinear(NULL)
    , m_pBlendStateAlpha(NULL)
    , m_pBlendStateAdditive(NULL)
    , m_pBlendStateMultiply(NULL)
    , m_pBlendStateOpaque(NULL)
    , m_pRasterizerState(NULL)
    , m_pD2DFactory(NULL)
    , m_pD2DRenderTarget(NULL)
    , m_pDWriteFactory(NULL)
    , m_pTextFormat(NULL)
    , m_pTextBrush(NULL)
    , m_hWnd(NULL)
    , m_iWidth(800)
    , m_iHeight(600)
    , m_iGameWidth(800)
    , m_iGameHeight(600)
    , m_fScaleX(1.0f)
    , m_fScaleY(1.0f)
    , m_bFullscreen(FALSE)
    , m_bInitialized(FALSE)
    , m_bVSync(TRUE)
    , m_pCurrentTexture(NULL)
    , m_CurrentBlendMode(BlendMode::Alpha)
{
    SetRect(&m_rcClip, 0, 0, 800, 600);
    ZeroMemory(&m_Viewport, sizeof(m_Viewport));
}

Direct3D11Renderer::~Direct3D11Renderer()
{
    Shutdown();
}

// ==================== INICIALIZACIÓN ====================

BOOL Direct3D11Renderer::Initialize(HWND hWnd, int width, int height, BOOL fullscreen)
{
    if (m_bInitialized) {
        Shutdown();
    }
    
    m_hWnd = hWnd;
    m_iGameWidth = 800;  // Resolución base del juego
    m_iGameHeight = 600;
    m_bFullscreen = fullscreen;
    
    // Obtener tamaño real de la ventana/pantalla
    if (fullscreen) {
        m_iWidth = GetSystemMetrics(SM_CXSCREEN);
        m_iHeight = GetSystemMetrics(SM_CYSCREEN);
    } else {
        RECT rc;
        GetClientRect(hWnd, &rc);
        m_iWidth = rc.right - rc.left;
        m_iHeight = rc.bottom - rc.top;
        if (m_iWidth < 800) m_iWidth = 800;
        if (m_iHeight < 600) m_iHeight = 600;
    }
    
    // Calcular factores de escala
    m_fScaleX = (float)m_iWidth / (float)m_iGameWidth;
    m_fScaleY = (float)m_iHeight / (float)m_iGameHeight;
    
    // Crear dispositivo D3D11
    if (!CreateDevice()) {
        Shutdown();
        return FALSE;
    }
    
    // Crear swap chain
    if (!CreateSwapChain()) {
        Shutdown();
        return FALSE;
    }
    
    // Crear render target
    if (!CreateRenderTarget()) {
        Shutdown();
        return FALSE;
    }
    
    // Crear shaders
    if (!CreateShaders()) {
        Shutdown();
        return FALSE;
    }
    
    // Crear buffers
    if (!CreateBuffers()) {
        Shutdown();
        return FALSE;
    }
    
    // Crear blend states
    if (!CreateBlendStates()) {
        Shutdown();
        return FALSE;
    }
    
    // Crear samplers
    if (!CreateSamplers()) {
        Shutdown();
        return FALSE;
    }
    
    // Crear rasterizer state
    if (!CreateRasterizerState()) {
        Shutdown();
        return FALSE;
    }
    
    // Inicializar Direct2D para texto
    if (!InitDirect2D()) {
        // No es crítico, el texto puede fallar pero el juego funciona
    }
    
    // Configurar viewport
    m_Viewport.TopLeftX = 0;
    m_Viewport.TopLeftY = 0;
    m_Viewport.Width = (float)m_iWidth;
    m_Viewport.Height = (float)m_iHeight;
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;
    
    // Configurar clipping inicial
    SetRect(&m_rcClip, 0, 0, m_iGameWidth, m_iGameHeight);
    
    // Actualizar matriz de proyección
    UpdateProjectionMatrix();
    
    m_bInitialized = TRUE;
    return TRUE;
}

BOOL Direct3D11Renderer::CreateDevice()
{
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    
    D3D_FEATURE_LEVEL featureLevel;
    
    HRESULT hr = D3D11CreateDevice(
        NULL,                       // Adaptador por defecto
        D3D_DRIVER_TYPE_HARDWARE,   // Hardware acceleration
        NULL,                       // Sin software rasterizer
        createDeviceFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &m_pDevice,
        &featureLevel,
        &m_pContext
    );
    
    if (FAILED(hr)) {
        // Intentar con WARP (software) como fallback
        hr = D3D11CreateDevice(
            NULL,
            D3D_DRIVER_TYPE_WARP,
            NULL,
            createDeviceFlags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &m_pDevice,
            &featureLevel,
            &m_pContext
        );
        
        if (FAILED(hr)) {
            return FALSE;
        }
    }
    
    return TRUE;
}

BOOL Direct3D11Renderer::CreateSwapChain()
{
    // Obtener factory DXGI
    IDXGIDevice* dxgiDevice = NULL;
    m_pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    
    IDXGIAdapter* dxgiAdapter = NULL;
    dxgiDevice->GetAdapter(&dxgiAdapter);
    
    IDXGIFactory* dxgiFactory = NULL;
    dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
    
    // Describir swap chain
    DXGI_SWAP_CHAIN_DESC scd;
    ZeroMemory(&scd, sizeof(scd));
    scd.BufferCount = 2;  // Double buffering
    scd.BufferDesc.Width = m_iWidth;
    scd.BufferDesc.Height = m_iHeight;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = m_hWnd;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = !m_bFullscreen;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    
    HRESULT hr = dxgiFactory->CreateSwapChain(m_pDevice, &scd, &m_pSwapChain);
    
    // Deshabilitar Alt+Enter automático
    dxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
    
    dxgiFactory->Release();
    dxgiAdapter->Release();
    dxgiDevice->Release();
    
    return SUCCEEDED(hr);
}

BOOL Direct3D11Renderer::CreateRenderTarget()
{
    // Obtener back buffer
    ID3D11Texture2D* pBackBuffer = NULL;
    HRESULT hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (FAILED(hr)) return FALSE;
    
    // Crear render target view
    hr = m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pRenderTargetView);
    pBackBuffer->Release();
    
    if (FAILED(hr)) return FALSE;
    
    // Establecer render target
    m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);
    
    return TRUE;
}

BOOL Direct3D11Renderer::CreateDepthStencil()
{
    // Para un juego 2D, el depth stencil es opcional
    // Pero lo creamos por si acaso necesitamos ordenar sprites por Z
    
    D3D11_TEXTURE2D_DESC depthDesc;
    ZeroMemory(&depthDesc, sizeof(depthDesc));
    depthDesc.Width = m_iWidth;
    depthDesc.Height = m_iHeight;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    
    HRESULT hr = m_pDevice->CreateTexture2D(&depthDesc, NULL, &m_pDepthStencil);
    if (FAILED(hr)) {
        // No es crítico, podemos funcionar sin depth buffer
        OutputDebugStringA("Direct3D11Renderer: Warning - Could not create depth stencil\n");
        return TRUE;
    }
    
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    ZeroMemory(&dsvDesc, sizeof(dsvDesc));
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    
    hr = m_pDevice->CreateDepthStencilView(m_pDepthStencil, &dsvDesc, &m_pDepthStencilView);
    if (FAILED(hr)) {
        m_pDepthStencil->Release();
        m_pDepthStencil = NULL;
        return TRUE;
    }
    
    return TRUE;
}

BOOL Direct3D11Renderer::CreateShaders()
{
    HRESULT hr;
    ID3DBlob* pVSBlob = NULL;
    ID3DBlob* pPSBlob = NULL;
    ID3DBlob* pErrorBlob = NULL;
    
    // Compilar vertex shader
    hr = D3DCompile(
        g_szVertexShader,
        strlen(g_szVertexShader),
        "VS",
        NULL,
        NULL,
        "main",
        "vs_4_0",
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        &pVSBlob,
        &pErrorBlob
    );
    
    if (FAILED(hr)) {
        if (pErrorBlob) {
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
            pErrorBlob->Release();
        }
        return FALSE;
    }
    
    // Crear vertex shader
    hr = m_pDevice->CreateVertexShader(
        pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(),
        NULL,
        &m_pVertexShader
    );
    
    if (FAILED(hr)) {
        pVSBlob->Release();
        return FALSE;
    }
    
    // Crear input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    
    hr = m_pDevice->CreateInputLayout(
        layout,
        ARRAYSIZE(layout),
        pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(),
        &m_pInputLayout
    );
    
    pVSBlob->Release();
    
    if (FAILED(hr)) return FALSE;
    
    // Compilar pixel shader
    hr = D3DCompile(
        g_szPixelShader,
        strlen(g_szPixelShader),
        "PS",
        NULL,
        NULL,
        "main",
        "ps_4_0",
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        &pPSBlob,
        &pErrorBlob
    );
    
    if (FAILED(hr)) {
        if (pErrorBlob) {
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
            pErrorBlob->Release();
        }
        return FALSE;
    }
    
    // Crear pixel shader
    hr = m_pDevice->CreatePixelShader(
        pPSBlob->GetBufferPointer(),
        pPSBlob->GetBufferSize(),
        NULL,
        &m_pPixelShader
    );
    
    pPSBlob->Release();
    
    return SUCCEEDED(hr);
}

BOOL Direct3D11Renderer::CreateBuffers()
{
    HRESULT hr;
    
    // Crear vertex buffer para batch de sprites
    D3D11_BUFFER_DESC vbd;
    ZeroMemory(&vbd, sizeof(vbd));
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.ByteWidth = sizeof(SpriteVertex) * MAX_BATCH_SIZE * 4;  // 4 vértices por sprite
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    hr = m_pDevice->CreateBuffer(&vbd, NULL, &m_pVertexBuffer);
    if (FAILED(hr)) return FALSE;
    
    // Crear index buffer
    // Cada sprite usa 6 índices (2 triángulos)
    std::vector<WORD> indices(MAX_BATCH_SIZE * 6);
    for (int i = 0; i < MAX_BATCH_SIZE; i++) {
        indices[i * 6 + 0] = i * 4 + 0;
        indices[i * 6 + 1] = i * 4 + 1;
        indices[i * 6 + 2] = i * 4 + 2;
        indices[i * 6 + 3] = i * 4 + 0;
        indices[i * 6 + 4] = i * 4 + 2;
        indices[i * 6 + 5] = i * 4 + 3;
    }
    
    D3D11_BUFFER_DESC ibd;
    ZeroMemory(&ibd, sizeof(ibd));
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(WORD) * MAX_BATCH_SIZE * 6;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA iData;
    ZeroMemory(&iData, sizeof(iData));
    iData.pSysMem = indices.data();
    
    hr = m_pDevice->CreateBuffer(&ibd, &iData, &m_pIndexBuffer);
    if (FAILED(hr)) return FALSE;
    
    // Crear constant buffer
    D3D11_BUFFER_DESC cbd;
    ZeroMemory(&cbd, sizeof(cbd));
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(ShaderConstants);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    hr = m_pDevice->CreateBuffer(&cbd, NULL, &m_pConstantBuffer);
    
    return SUCCEEDED(hr);
}

BOOL Direct3D11Renderer::CreateBlendStates()
{
    HRESULT hr;
    D3D11_BLEND_DESC bd;
    
    // Alpha blending normal
    ZeroMemory(&bd, sizeof(bd));
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    
    hr = m_pDevice->CreateBlendState(&bd, &m_pBlendStateAlpha);
    if (FAILED(hr)) return FALSE;
    
    // Blending aditivo (para luces/efectos)
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    
    hr = m_pDevice->CreateBlendState(&bd, &m_pBlendStateAdditive);
    if (FAILED(hr)) return FALSE;
    
    // Blending multiplicativo (para sombras)
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    
    hr = m_pDevice->CreateBlendState(&bd, &m_pBlendStateMultiply);
    if (FAILED(hr)) return FALSE;
    
    // Sin blending (opaco)
    bd.RenderTarget[0].BlendEnable = FALSE;
    
    hr = m_pDevice->CreateBlendState(&bd, &m_pBlendStateOpaque);
    
    return SUCCEEDED(hr);
}

BOOL Direct3D11Renderer::CreateSamplers()
{
    HRESULT hr;
    D3D11_SAMPLER_DESC sd;
    
    // Sampler point (para sprites pixel-art, sin filtrado)
    ZeroMemory(&sd, sizeof(sd));
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    
    hr = m_pDevice->CreateSamplerState(&sd, &m_pSamplerPoint);
    if (FAILED(hr)) return FALSE;
    
    // Sampler linear (para UI escalada, con filtrado)
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    
    hr = m_pDevice->CreateSamplerState(&sd, &m_pSamplerLinear);
    
    return SUCCEEDED(hr);
}

BOOL Direct3D11Renderer::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC rd;
    ZeroMemory(&rd, sizeof(rd));
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;  // Sin culling para sprites 2D
    rd.FrontCounterClockwise = FALSE;
    rd.DepthClipEnable = TRUE;
    rd.ScissorEnable = TRUE;  // Habilitar scissor para clipping
    
    HRESULT hr = m_pDevice->CreateRasterizerState(&rd, &m_pRasterizerState);
    return SUCCEEDED(hr);
}

BOOL Direct3D11Renderer::InitDirect2D()
{
    HRESULT hr;
    
    // Crear D2D factory
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
    if (FAILED(hr)) return FALSE;
    
    // Crear DirectWrite factory
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&m_pDWriteFactory)
    );
    if (FAILED(hr)) return FALSE;
    
    // Crear formato de texto por defecto
    hr = m_pDWriteFactory->CreateTextFormat(
        L"Tahoma",
        NULL,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        14.0f,
        L"en-us",
        &m_pTextFormat
    );
    
    return SUCCEEDED(hr);
}

void Direct3D11Renderer::UpdateProjectionMatrix()
{
    if (m_pContext == NULL || m_pConstantBuffer == NULL) return;
    
    // Crear matriz de proyección ortográfica
    // Mapea coordenadas del juego (0-800, 0-600) a coordenadas normalizadas (-1 a 1)
    XMMATRIX proj = XMMatrixOrthographicOffCenterLH(
        0.0f,                       // Left
        (float)m_iGameWidth,        // Right
        (float)m_iGameHeight,       // Bottom (invertido para que Y crezca hacia abajo)
        0.0f,                       // Top
        0.0f,                       // Near
        1.0f                        // Far
    );
    
    // Actualizar constant buffer
    D3D11_MAPPED_SUBRESOURCE ms;
    if (SUCCEEDED(m_pContext->Map(m_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
        ShaderConstants* constants = (ShaderConstants*)ms.pData;
        constants->projection = XMMatrixTranspose(proj);
        constants->globalTint = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        constants->alpha = 1.0f;
        m_pContext->Unmap(m_pConstantBuffer, 0);
    }
}

void Direct3D11Renderer::Shutdown()
{
    // Limpiar cache de texturas
    for (auto& pair : m_TextureCache) {
        if (pair.second.pSRV) pair.second.pSRV->Release();
        if (pair.second.pTexture) pair.second.pTexture->Release();
    }
    m_TextureCache.clear();
    
    // Limpiar Direct2D
    if (m_pTextBrush) { m_pTextBrush->Release(); m_pTextBrush = NULL; }
    if (m_pTextFormat) { m_pTextFormat->Release(); m_pTextFormat = NULL; }
    if (m_pD2DRenderTarget) { m_pD2DRenderTarget->Release(); m_pD2DRenderTarget = NULL; }
    if (m_pDWriteFactory) { m_pDWriteFactory->Release(); m_pDWriteFactory = NULL; }
    if (m_pD2DFactory) { m_pD2DFactory->Release(); m_pD2DFactory = NULL; }
    
    // Limpiar D3D11
    if (m_pRasterizerState) { m_pRasterizerState->Release(); m_pRasterizerState = NULL; }
    if (m_pBlendStateOpaque) { m_pBlendStateOpaque->Release(); m_pBlendStateOpaque = NULL; }
    if (m_pBlendStateMultiply) { m_pBlendStateMultiply->Release(); m_pBlendStateMultiply = NULL; }
    if (m_pBlendStateAdditive) { m_pBlendStateAdditive->Release(); m_pBlendStateAdditive = NULL; }
    if (m_pBlendStateAlpha) { m_pBlendStateAlpha->Release(); m_pBlendStateAlpha = NULL; }
    if (m_pSamplerLinear) { m_pSamplerLinear->Release(); m_pSamplerLinear = NULL; }
    if (m_pSamplerPoint) { m_pSamplerPoint->Release(); m_pSamplerPoint = NULL; }
    if (m_pConstantBuffer) { m_pConstantBuffer->Release(); m_pConstantBuffer = NULL; }
    if (m_pIndexBuffer) { m_pIndexBuffer->Release(); m_pIndexBuffer = NULL; }
    if (m_pVertexBuffer) { m_pVertexBuffer->Release(); m_pVertexBuffer = NULL; }
    if (m_pPixelShader) { m_pPixelShader->Release(); m_pPixelShader = NULL; }
    if (m_pVertexShader) { m_pVertexShader->Release(); m_pVertexShader = NULL; }
    if (m_pInputLayout) { m_pInputLayout->Release(); m_pInputLayout = NULL; }
    if (m_pDepthStencil) { m_pDepthStencil->Release(); m_pDepthStencil = NULL; }
    if (m_pDepthStencilView) { m_pDepthStencilView->Release(); m_pDepthStencilView = NULL; }
    if (m_pRenderTargetView) { m_pRenderTargetView->Release(); m_pRenderTargetView = NULL; }
    if (m_pSwapChain) { m_pSwapChain->Release(); m_pSwapChain = NULL; }
    if (m_pContext) { m_pContext->Release(); m_pContext = NULL; }
    if (m_pDevice) { m_pDevice->Release(); m_pDevice = NULL; }
    
    m_bInitialized = FALSE;
}

BOOL Direct3D11Renderer::SetDisplayMode(int width, int height, BOOL fullscreen)
{
    // TODO: Implementar cambio de modo sin reinicializar
    m_iWidth = width;
    m_iHeight = height;
    m_bFullscreen = fullscreen;
    m_fScaleX = (float)width / (float)m_iGameWidth;
    m_fScaleY = (float)height / (float)m_iGameHeight;
    return TRUE;
}

// ==================== FRAME MANAGEMENT ====================

void Direct3D11Renderer::BeginFrame()
{
    if (!m_bInitialized) return;
    
    // Limpiar sprite batch
    m_SpriteBatch.clear();
    m_pCurrentTexture = NULL;
    
    // Establecer render target
    m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, NULL);
    
    // Establecer viewport
    m_pContext->RSSetViewports(1, &m_Viewport);
    
    // Establecer rasterizer state
    m_pContext->RSSetState(m_pRasterizerState);
    
    // Establecer shaders
    m_pContext->VSSetShader(m_pVertexShader, NULL, 0);
    m_pContext->PSSetShader(m_pPixelShader, NULL, 0);
    m_pContext->IASetInputLayout(m_pInputLayout);
    
    // Establecer buffers
    UINT stride = sizeof(SpriteVertex);
    UINT offset = 0;
    m_pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
    m_pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // Establecer constant buffer
    m_pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
    
    // Establecer sampler
    m_pContext->PSSetSamplers(0, 1, &m_pSamplerPoint);
    
    // Establecer blend state por defecto
    float blendFactor[4] = { 0, 0, 0, 0 };
    m_pContext->OMSetBlendState(m_pBlendStateAlpha, blendFactor, 0xFFFFFFFF);
}

HRESULT Direct3D11Renderer::EndFrame()
{
    if (!m_bInitialized) return E_FAIL;
    
    // Flush cualquier sprite pendiente
    FlushBatch();
    
    // Presentar
    HRESULT hr = m_pSwapChain->Present(m_bVSync ? 1 : 0, 0);
    
    return hr;
}

void Direct3D11Renderer::FlushBatch()
{
    if (m_SpriteBatch.empty()) return;
    
    // Actualizar vertex buffer
    D3D11_MAPPED_SUBRESOURCE ms;
    if (SUCCEEDED(m_pContext->Map(m_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
        memcpy(ms.pData, m_SpriteBatch.data(), m_SpriteBatch.size() * sizeof(SpriteVertex));
        m_pContext->Unmap(m_pVertexBuffer, 0);
    }
    
    // Dibujar
    int numSprites = (int)m_SpriteBatch.size() / 4;
    m_pContext->DrawIndexed(numSprites * 6, 0, 0);
    
    // Limpiar batch
    m_SpriteBatch.clear();
}

// ==================== DRAWING PRIMITIVES ====================

void Direct3D11Renderer::Clear(COLORREF color)
{
    if (!m_bInitialized) return;
    
    float clearColor[4] = {
        GetRValue(color) / 255.0f,
        GetGValue(color) / 255.0f,
        GetBValue(color) / 255.0f,
        1.0f
    };
    
    m_pContext->ClearRenderTargetView(m_pRenderTargetView, clearColor);
}

void Direct3D11Renderer::FillRect(int x1, int y1, int x2, int y2, COLORREF color)
{
    if (!m_bInitialized) return;
    
    // Flush sprites pendientes
    FlushBatch();
    
    // Crear un quad de color sólido
    float r = GetRValue(color) / 255.0f;
    float g = GetGValue(color) / 255.0f;
    float b = GetBValue(color) / 255.0f;
    
    SpriteVertex vertices[4] = {
        { XMFLOAT3((float)x1, (float)y1, 0.0f), XMFLOAT2(0, 0), XMFLOAT4(r, g, b, 1.0f) },
        { XMFLOAT3((float)x2, (float)y1, 0.0f), XMFLOAT2(1, 0), XMFLOAT4(r, g, b, 1.0f) },
        { XMFLOAT3((float)x2, (float)y2, 0.0f), XMFLOAT2(1, 1), XMFLOAT4(r, g, b, 1.0f) },
        { XMFLOAT3((float)x1, (float)y2, 0.0f), XMFLOAT2(0, 1), XMFLOAT4(r, g, b, 1.0f) },
    };
    
    // Añadir al batch
    for (int i = 0; i < 4; i++) {
        m_SpriteBatch.push_back(vertices[i]);
    }
    
    // Dibujar sin textura
    m_pContext->PSSetShaderResources(0, 0, NULL);
    FlushBatch();
}

void Direct3D11Renderer::DrawRect(int x1, int y1, int x2, int y2, COLORREF color)
{
    DrawLine(x1, y1, x2, y1, color);
    DrawLine(x2, y1, x2, y2, color);
    DrawLine(x2, y2, x1, y2, color);
    DrawLine(x1, y2, x1, y1, color);
}

void Direct3D11Renderer::DrawLine(int x1, int y1, int x2, int y2, COLORREF color)
{
    // Implementación simple usando rectángulos de 1 pixel de ancho
    if (x1 == x2) {
        FillRect(x1, min(y1, y2), x1 + 1, max(y1, y2), color);
    } else if (y1 == y2) {
        FillRect(min(x1, x2), y1, max(x1, x2), y1 + 1, color);
    } else {
        // Línea diagonal - implementación Bresenham simplificada
        // Por ahora usar GDI fallback o implementar después
    }
}

void Direct3D11Renderer::PutPixel(int x, int y, COLORREF color)
{
    FillRect(x, y, x + 1, y + 1, color);
}

// ==================== TEXT RENDERING ====================

void Direct3D11Renderer::DrawText(int x, int y, const char* text, COLORREF color)
{
    // TODO: Implementar con DirectWrite
    // Por ahora placeholder
}

void Direct3D11Renderer::DrawTextRect(RECT* rect, const char* text, COLORREF color, UINT format)
{
    // TODO: Implementar con DirectWrite
}

void Direct3D11Renderer::SetFont(HFONT hFont)
{
    // TODO: Convertir HFONT a IDWriteTextFormat
}

// ==================== SPRITE RENDERING ====================

void Direct3D11Renderer::DrawSprite(int x, int y, CSprite* sprite, int frame, BOOL mirror)
{
    DrawSpriteAlpha(x, y, sprite, frame, 1.0f, mirror);
}

void Direct3D11Renderer::DrawSpriteAlpha(int x, int y, CSprite* sprite, int frame, float alpha, BOOL mirror)
{
    if (!m_bInitialized || sprite == NULL) return;
    
    // Obtener textura del sprite
    ID3D11ShaderResourceView* pSRV = GetSpriteTexture(sprite, frame);
    if (pSRV == NULL) return;
    
    // Si cambia la textura, flush el batch
    if (pSRV != m_pCurrentTexture) {
        FlushBatch();
        m_pCurrentTexture = pSRV;
        m_pContext->PSSetShaderResources(0, 1, &pSRV);
    }
    
    // Obtener dimensiones del sprite
    int width = sprite->m_stBrush[frame].szx;
    int height = sprite->m_stBrush[frame].szy;
    
    // Calcular posición (ajustar por hotspot del sprite)
    float fx = (float)x;
    float fy = (float)y;
    float fw = (float)width;
    float fh = (float)height;
    
    // Coordenadas de textura
    float u1 = mirror ? 1.0f : 0.0f;
    float u2 = mirror ? 0.0f : 1.0f;
    
    // Crear vértices
    SpriteVertex vertices[4] = {
        { XMFLOAT3(fx, fy, 0.0f), XMFLOAT2(u1, 0.0f), XMFLOAT4(1, 1, 1, alpha) },
        { XMFLOAT3(fx + fw, fy, 0.0f), XMFLOAT2(u2, 0.0f), XMFLOAT4(1, 1, 1, alpha) },
        { XMFLOAT3(fx + fw, fy + fh, 0.0f), XMFLOAT2(u2, 1.0f), XMFLOAT4(1, 1, 1, alpha) },
        { XMFLOAT3(fx, fy + fh, 0.0f), XMFLOAT2(u1, 1.0f), XMFLOAT4(1, 1, 1, alpha) },
    };
    
    // Añadir al batch
    for (int i = 0; i < 4; i++) {
        m_SpriteBatch.push_back(vertices[i]);
    }
    
    // Si el batch está lleno, flush
    if (m_SpriteBatch.size() >= MAX_BATCH_SIZE * 4) {
        FlushBatch();
    }
}

void Direct3D11Renderer::DrawSpriteBlend(int x, int y, CSprite* sprite, int frame, BlendMode mode, BOOL mirror)
{
    if (!m_bInitialized) return;
    
    // Cambiar blend state si es necesario
    if (mode != m_CurrentBlendMode) {
        FlushBatch();
        
        float blendFactor[4] = { 0, 0, 0, 0 };
        switch (mode) {
            case BlendMode::None:
                m_pContext->OMSetBlendState(m_pBlendStateOpaque, blendFactor, 0xFFFFFFFF);
                break;
            case BlendMode::Alpha:
                m_pContext->OMSetBlendState(m_pBlendStateAlpha, blendFactor, 0xFFFFFFFF);
                break;
            case BlendMode::Additive:
                m_pContext->OMSetBlendState(m_pBlendStateAdditive, blendFactor, 0xFFFFFFFF);
                break;
            case BlendMode::Multiply:
                m_pContext->OMSetBlendState(m_pBlendStateMultiply, blendFactor, 0xFFFFFFFF);
                break;
        }
        m_CurrentBlendMode = mode;
    }
    
    DrawSprite(x, y, sprite, frame, mirror);
}

void Direct3D11Renderer::DrawSpriteScaled(int x, int y, CSprite* sprite, int frame, float scaleX, float scaleY, BOOL mirror)
{
    // TODO: Implementar scaling
    DrawSprite(x, y, sprite, frame, mirror);
}

void Direct3D11Renderer::DrawSpriteTinted(int x, int y, CSprite* sprite, int frame, COLORREF tint, BOOL mirror)
{
    // TODO: Implementar tinting
    DrawSprite(x, y, sprite, frame, mirror);
}

// ==================== TEXTURE MANAGEMENT ====================

ID3D11ShaderResourceView* Direct3D11Renderer::GetSpriteTexture(CSprite* sprite, int frame)
{
    if (sprite == NULL) return NULL;
    
    // Generar ID único para este sprite+frame
    int textureId = (int)(intptr_t)sprite * 1000 + frame;
    
    // Buscar en cache
    auto it = m_TextureCache.find(textureId);
    if (it != m_TextureCache.end()) {
        return it->second.pSRV;
    }
    
    // No está en cache, convertir sprite a textura
    CachedTexture tex = ConvertSpriteToTexture(sprite, frame);
    if (tex.pSRV == NULL) return NULL;
    
    // Guardar en cache
    m_TextureCache[textureId] = tex;
    
    return tex.pSRV;
}

CachedTexture Direct3D11Renderer::ConvertSpriteToTexture(CSprite* sprite, int frame)
{
    CachedTexture result = { NULL, NULL, 0, 0 };
    
    if (sprite == NULL || frame < 0 || frame >= sprite->m_iTotalFrame) {
        return result;
    }
    
    int width = sprite->m_stBrush[frame].szx;
    int height = sprite->m_stBrush[frame].szy;
    
    if (width <= 0 || height <= 0) return result;
    
    // Los datos del sprite están en la superficie DirectDraw
    // Necesitamos bloquear la superficie para leer los pixels
    if (sprite->m_lpSurface == NULL) {
        return result;
    }
    
    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    
    HRESULT hr = sprite->m_lpSurface->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_READONLY, NULL);
    if (FAILED(hr)) {
        return result;
    }
    
    // Obtener datos del sprite (16-bit 565 RGB)
    WORD* surfaceData = (WORD*)ddsd.lpSurface;
    int pitch = ddsd.lPitch / 2;  // Pitch en WORDs
    
    // Offset del frame dentro de la superficie
    int frameX = sprite->m_stBrush[frame].sx;
    int frameY = sprite->m_stBrush[frame].sy;
    
    // Convertir a 32-bit BGRA
    DWORD* dstData = new DWORD[width * height];
    WORD colorKey = sprite->m_wColorKey;  // Color key del sprite
    
    for (int y = 0; y < height; y++) {
        WORD* srcRow = surfaceData + (frameY + y) * pitch + frameX;
        DWORD* dstRow = dstData + y * width;
        
        for (int x = 0; x < width; x++) {
            WORD pixel = srcRow[x];
            
            if (pixel == colorKey) {
                // Pixel transparente
                dstRow[x] = 0x00000000;
            } else {
                // Convertir 565 RGB a 8888 BGRA
                BYTE r = ((pixel >> 11) & 0x1F) << 3;
                BYTE g = ((pixel >> 5) & 0x3F) << 2;
                BYTE b = (pixel & 0x1F) << 3;
                
                // Expandir los bits bajos para mejor calidad
                r |= r >> 5;
                g |= g >> 6;
                b |= b >> 5;
                
                dstRow[x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    }
    
    sprite->m_lpSurface->Unlock(NULL);
    
    // Crear textura D3D11
    D3D11_TEXTURE2D_DESC td;
    ZeroMemory(&td, sizeof(td));
    td.Width = width;
    td.Height = height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    
    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(initData));
    initData.pSysMem = dstData;
    initData.SysMemPitch = width * sizeof(DWORD);
    
    ID3D11Texture2D* pTexture = NULL;
    hr = m_pDevice->CreateTexture2D(&td, &initData, &pTexture);
    
    delete[] dstData;
    
    if (FAILED(hr)) return result;
    
    // Crear shader resource view
    ID3D11ShaderResourceView* pSRV = NULL;
    hr = m_pDevice->CreateShaderResourceView(pTexture, NULL, &pSRV);
    
    if (FAILED(hr)) {
        pTexture->Release();
        return result;
    }
    
    result.pTexture = pTexture;
    result.pSRV = pSRV;
    result.width = width;
    result.height = height;
    
    return result;
}

// ==================== SURFACE OPERATIONS ====================

void* Direct3D11Renderer::CreateOffscreenSurface(int width, int height)
{
    // TODO: Implementar
    return NULL;
}

void Direct3D11Renderer::DestroyOffscreenSurface(void* surface)
{
    // TODO: Implementar
}

void Direct3D11Renderer::SetRenderTarget(void* surface)
{
    // TODO: Implementar
}

void Direct3D11Renderer::Blit(void* srcSurface, RECT* srcRect, int dstX, int dstY)
{
    // TODO: Implementar
}

// ==================== CLIPPING ====================

void Direct3D11Renderer::SetClipRect(RECT* rect)
{
    if (!m_bInitialized || rect == NULL) return;
    
    CopyRect(&m_rcClip, rect);
    
    // Convertir a coordenadas de pantalla
    D3D11_RECT scissor;
    scissor.left = (LONG)(rect->left * m_fScaleX);
    scissor.top = (LONG)(rect->top * m_fScaleY);
    scissor.right = (LONG)(rect->right * m_fScaleX);
    scissor.bottom = (LONG)(rect->bottom * m_fScaleY);
    
    m_pContext->RSSetScissorRects(1, &scissor);
}

void Direct3D11Renderer::GetClipRect(RECT* rect)
{
    if (rect != NULL) {
        CopyRect(rect, &m_rcClip);
    }
}

void Direct3D11Renderer::ResetClipRect()
{
    SetRect(&m_rcClip, 0, 0, m_iGameWidth, m_iGameHeight);
    
    D3D11_RECT scissor;
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = m_iWidth;
    scissor.bottom = m_iHeight;
    
    m_pContext->RSSetScissorRects(1, &scissor);
}

// ==================== SPECIAL EFFECTS ====================

void Direct3D11Renderer::DrawShadowBox(int x1, int y1, int x2, int y2, int intensity)
{
    // Dibujar un rectángulo semi-transparente negro
    FlushBatch();
    
    float alpha = 0.5f - intensity * 0.1f;
    if (alpha < 0.1f) alpha = 0.1f;
    
    SpriteVertex vertices[4] = {
        { XMFLOAT3((float)x1, (float)y1, 0.0f), XMFLOAT2(0, 0), XMFLOAT4(0, 0, 0, alpha) },
        { XMFLOAT3((float)x2, (float)y1, 0.0f), XMFLOAT2(1, 0), XMFLOAT4(0, 0, 0, alpha) },
        { XMFLOAT3((float)x2, (float)y2, 0.0f), XMFLOAT2(1, 1), XMFLOAT4(0, 0, 0, alpha) },
        { XMFLOAT3((float)x1, (float)y2, 0.0f), XMFLOAT2(0, 1), XMFLOAT4(0, 0, 0, alpha) },
    };
    
    for (int i = 0; i < 4; i++) {
        m_SpriteBatch.push_back(vertices[i]);
    }
    
    m_pContext->PSSetShaderResources(0, 0, NULL);
    FlushBatch();
}

void Direct3D11Renderer::DrawItemShadowBox(int x1, int y1, int x2, int y2, int type)
{
    DrawShadowBox(x1, y1, x2, y2, type);
}

// ==================== SCREENSHOTS ====================

BOOL Direct3D11Renderer::SaveScreenshot(const char* filename)
{
    // TODO: Implementar usando WIC
    return FALSE;
}

// ==================== COORDINATE CONVERSION ====================

void Direct3D11Renderer::GameToScreen(int& x, int& y)
{
    x = (int)(x * m_fScaleX);
    y = (int)(y * m_fScaleY);
}

void Direct3D11Renderer::ScreenToGame(int& x, int& y)
{
    x = (int)(x / m_fScaleX);
    y = (int)(y / m_fScaleY);
}

// ==================== WINDOW RESIZE ====================

void Direct3D11Renderer::OnResize(int width, int height)
{
    if (!m_bInitialized || !m_pDevice || !m_pSwapChain) {
        return;
    }
    
    // Liberar recursos del render target actual
    if (m_pRenderTargetView) {
        m_pRenderTargetView->Release();
        m_pRenderTargetView = NULL;
    }
    
    if (m_pDepthStencilView) {
        m_pDepthStencilView->Release();
        m_pDepthStencilView = NULL;
    }
    
    if (m_pDepthStencil) {
        m_pDepthStencil->Release();
        m_pDepthStencil = NULL;
    }
    
    // Recrear Direct2D render target si existe
    if (m_pD2DRenderTarget) {
        m_pD2DRenderTarget->Release();
        m_pD2DRenderTarget = NULL;
    }
    
    // Resize swap chain
    HRESULT hr = m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        char msg[256];
        sprintf(msg, "Direct3D11Renderer::OnResize - ResizeBuffers failed: 0x%08X\n", hr);
        OutputDebugStringA(msg);
        return;
    }
    
    // Actualizar dimensiones
    m_iWidth = width;
    m_iHeight = height;
    
    // Recalcular escala
    m_fScaleX = (float)m_iWidth / (float)m_iGameWidth;
    m_fScaleY = (float)m_iHeight / (float)m_iGameHeight;
    
    // Recrear render target
    CreateRenderTarget();
    CreateDepthStencil();
    
    // Actualizar viewport
    m_Viewport.Width = (float)m_iWidth;
    m_Viewport.Height = (float)m_iHeight;
    m_pContext->RSSetViewports(1, &m_Viewport);
    
    // Actualizar matriz de proyección
    UpdateProjectionMatrix();
    
    // Reinicializar Direct2D
    InitDirect2D();
    
    char msg[256];
    sprintf(msg, "Direct3D11Renderer::OnResize - Resized to %dx%d (scale: %.2fx%.2f)\n", 
            width, height, m_fScaleX, m_fScaleY);
    OutputDebugStringA(msg);
}
