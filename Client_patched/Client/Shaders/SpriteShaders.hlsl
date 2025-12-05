// SpriteShaders.hlsl - Shaders para renderizado de sprites en Helbreath
// Compatible con Direct3D 11 (Shader Model 5.0)

// ==================== CONSTANTES ====================

cbuffer ShaderConstants : register(b0)
{
    matrix Projection;      // Matriz de proyección ortográfica
    float4 GlobalTint;      // Tinte global (r, g, b, a)
    float GlobalAlpha;      // Alpha global
    float3 Padding;         // Padding para alineación
};

// ==================== ESTRUCTURAS ====================

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

// ==================== TEXTURAS Y SAMPLERS ====================

Texture2D SpriteTexture : register(t0);
SamplerState SpriteSampler : register(s0);

// ==================== VERTEX SHADER ====================

PS_INPUT VS_Sprite(VS_INPUT input)
{
    PS_INPUT output;
    
    // Transformar posición con la matriz de proyección ortográfica
    output.Position = mul(float4(input.Position, 1.0f), Projection);
    
    // Pasar coordenadas de textura
    output.TexCoord = input.TexCoord;
    
    // Combinar color del vértice con tinte global
    output.Color = input.Color * GlobalTint;
    output.Color.a *= GlobalAlpha;
    
    return output;
}

// ==================== PIXEL SHADER - NORMAL ====================

float4 PS_Sprite(PS_INPUT input) : SV_TARGET
{
    // Muestrear textura
    float4 texColor = SpriteTexture.Sample(SpriteSampler, input.TexCoord);
    
    // Si el pixel es transparente (alpha muy bajo), descartarlo
    // Esto es importante para los sprites con color key
    if (texColor.a < 0.01f)
    {
        discard;
    }
    
    // Aplicar tinte del vértice
    float4 finalColor = texColor * input.Color;
    
    return finalColor;
}

// ==================== PIXEL SHADER - SHADOW ====================
// Para el efecto de sombra (oscurecimiento)

float4 PS_Shadow(PS_INPUT input) : SV_TARGET
{
    // Muestrear textura
    float4 texColor = SpriteTexture.Sample(SpriteSampler, input.TexCoord);
    
    if (texColor.a < 0.01f)
    {
        discard;
    }
    
    // Oscurecer el color
    float shadowIntensity = input.Color.a; // Usamos alpha como intensidad de sombra
    float3 shadowColor = texColor.rgb * (1.0f - shadowIntensity * 0.5f);
    
    return float4(shadowColor, texColor.a);
}

// ==================== PIXEL SHADER - ADDITIVE ====================
// Para efectos de luz/fuego/magia

float4 PS_Additive(PS_INPUT input) : SV_TARGET
{
    float4 texColor = SpriteTexture.Sample(SpriteSampler, input.TexCoord);
    
    if (texColor.a < 0.01f)
    {
        discard;
    }
    
    // El blending aditivo se maneja en el blend state,
    // pero podemos pre-multiplicar por alpha para mejor resultado
    float4 finalColor = texColor * input.Color;
    finalColor.rgb *= finalColor.a;
    
    return finalColor;
}

// ==================== PIXEL SHADER - GRAYSCALE ====================
// Para efectos de "piedra" o "congelado"

float4 PS_Grayscale(PS_INPUT input) : SV_TARGET
{
    float4 texColor = SpriteTexture.Sample(SpriteSampler, input.TexCoord);
    
    if (texColor.a < 0.01f)
    {
        discard;
    }
    
    // Convertir a escala de grises
    float gray = dot(texColor.rgb, float3(0.299f, 0.587f, 0.114f));
    
    // Mezclar con el color original según el tinte
    float3 finalRGB = lerp(texColor.rgb, float3(gray, gray, gray), input.Color.a);
    
    return float4(finalRGB, texColor.a);
}

// ==================== PIXEL SHADER - COLOR KEY ====================
// Para sprites con transparencia basada en color específico (magenta, etc.)

float4 PS_ColorKey(PS_INPUT input) : SV_TARGET
{
    float4 texColor = SpriteTexture.Sample(SpriteSampler, input.TexCoord);
    
    // Color key típico: magenta (255, 0, 255)
    // Si el color está muy cerca del color key, hacerlo transparente
    float3 colorKey = float3(1.0f, 0.0f, 1.0f);
    float distance = length(texColor.rgb - colorKey);
    
    if (distance < 0.1f)
    {
        discard;
    }
    
    return texColor * input.Color;
}

// ==================== PIXEL SHADER - OUTLINE ====================
// Para resaltar personajes/objetos seleccionados

float4 PS_Outline(PS_INPUT input) : SV_TARGET
{
    float4 texColor = SpriteTexture.Sample(SpriteSampler, input.TexCoord);
    
    if (texColor.a < 0.01f)
    {
        discard;
    }
    
    // Detectar borde muestreando pixels vecinos
    float2 texelSize = float2(1.0f / 256.0f, 1.0f / 256.0f); // Ajustar según tamaño de sprite
    
    float alphaUp = SpriteTexture.Sample(SpriteSampler, input.TexCoord + float2(0, -texelSize.y)).a;
    float alphaDown = SpriteTexture.Sample(SpriteSampler, input.TexCoord + float2(0, texelSize.y)).a;
    float alphaLeft = SpriteTexture.Sample(SpriteSampler, input.TexCoord + float2(-texelSize.x, 0)).a;
    float alphaRight = SpriteTexture.Sample(SpriteSampler, input.TexCoord + float2(texelSize.x, 0)).a;
    
    // Si algún vecino es transparente, estamos en un borde
    float edgeFactor = (alphaUp < 0.5f || alphaDown < 0.5f || alphaLeft < 0.5f || alphaRight < 0.5f) ? 1.0f : 0.0f;
    
    // Color del outline (del tinte del vértice)
    float3 outlineColor = input.Color.rgb;
    
    // Mezclar: borde = color outline, interior = color original
    float3 finalColor = lerp(texColor.rgb, outlineColor, edgeFactor);
    
    return float4(finalColor, texColor.a);
}
