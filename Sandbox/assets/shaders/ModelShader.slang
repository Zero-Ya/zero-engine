// ----------------------------------------------------------------------------
// Data Structures
// ----------------------------------------------------------------------------

struct CameraData {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

struct MaterialData {
    float4 baseColorFactor;
    float3 emissiveFactor;
    float  metallicFactor;
    float  roughnessFactor;
    float  alphaCutoff;
    
    uint   albedoTextureIndex;
    uint   normalTextureIndex;
    uint   roughnessTextureIndex;
    uint   emissiveTextureIndex;
    uint   samplerIndex;
    uint   flags;
    
    uint2  padding;
};

struct PushConstants {
    float4x4 modelMatrix;
    uint   albedoTextureIndex;
    uint   normalTextureIndex;
    uint   roughnessTextureIndex;
    uint   samplerIndex;
};



// ----------------------------------------------------------------------------
// Resource Bindings
// ----------------------------------------------------------------------------

// Set 0: Global Bindless Textures, Samplers, and Material SSBO
[[vk::binding(0, 0)]] Texture2D        g_Textures[];
[[vk::binding(1, 0)]] SamplerState     g_Samplers[];
[[vk::binding(2, 0)]] StructuredBuffer<MaterialData> g_Materials;

// Set 1: Per-Frame / Scene Data
[[vk::binding(0, 1)]] ConstantBuffer<CameraData> camera;

// Push Constants
[[vk::push_constant]]
PushConstants g_Push;



// ----------------------------------------------------------------------------
// Vertex Input / Output
// ----------------------------------------------------------------------------

struct VSInput {
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 normal   : NORMAL;
    [[vk::location(2)]] float2 uv       : TEXCOORD0;
};

struct VSOutput {
    float4 position : SV_Position;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
};



// ----------------------------------------------------------------------------
// Vertex Shader
// ----------------------------------------------------------------------------

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    
    // Combine Model (Push Constant) * View * Proj matrices
    float4 worldPos = mul(g_Push.modelMatrix, float4(input.position, 1.0));
    float4 viewPos  = mul(camera.view, worldPos);
    output.position = mul(camera.proj, viewPos);

    // Transform normals correctly using upper 3x3 of model matrix
    output.normal = mul((float3x3)g_Push.modelMatrix, input.normal);
    output.uv = input.uv;

    return output;
}



// ----------------------------------------------------------------------------
// Fragment Shader
// ----------------------------------------------------------------------------

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    // Resolve texture and sampler indices from Push Constants (Bindless lookup)
    uint albedoTextureIndex = g_Push.albedoTextureIndex;
    uint samplerIndex   = g_Push.samplerIndex;

    // Sample base color texture from global bindless array (Set 0)
    float4 albedoTex = g_Textures[albedoTextureIndex].Sample(g_Samplers[samplerIndex], input.uv);
    if (albedoTex.a < 0.5) {
        discard;
    }

    // Fetch default or fallback material factors 
    float4 baseColorFactor = float4(1.0, 1.0, 1.0, 1.0);

    // Combine base color texture with material factor multiplier
    float4 finalColor = albedoTex * baseColorFactor;

    // Basic directional light test (Sun light vector)
    float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
    float3 N = normalize(input.normal);
    float NdotL = max(dot(N, lightDir), 0.1); // Add 0.1 ambient floor

    // Apply lighting factor to final RGB output
    finalColor.rgb *= NdotL;

    return finalColor;
}