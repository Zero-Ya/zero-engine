// ----------------------------------------------------------------------------
// Data Structures
// ----------------------------------------------------------------------------

struct DirectionalLight {
    float3 direction;
    float  intensity;
    float3 color;
    float  padding;
};

struct CameraData {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

struct SceneUniformData {
    CameraData       camera;
    DirectionalLight sunLight;
    float4           cameraPos;
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
[[vk::binding(0, 1)]] ConstantBuffer<SceneUniformData> scene;

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
    float3 fragPos;
};



// ----------------------------------------------------------------------------
// Vertex Shader
// ----------------------------------------------------------------------------

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    
    // Combine Model (Push Constant) * View * Proj matrices
    float4 worldPos = mul(g_Push.modelMatrix, float4(input.position, 1.0));
    float4 viewPos  = mul(scene.camera.view, worldPos);
    output.position = mul(scene.camera.proj, viewPos);

    // Transform normals correctly using upper 3x3 of model matrix
    output.normal = mul((float3x3)g_Push.modelMatrix, input.normal);
    output.uv = input.uv;

    output.fragPos = worldPos.xyz;

    return output;
}



// ----------------------------------------------------------------------------
// Fragment Shader
// ----------------------------------------------------------------------------

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    // Resolve texture and sampler indices from Push Constants (Bindless lookup)
    uint albedoTextureIndex = g_Push.albedoTextureIndex;
    uint samplerIndex       = g_Push.samplerIndex;

    // Sample base color texture from global bindless array (Set 0)
    float4 albedoTexture = g_Textures[albedoTextureIndex].Sample(g_Samplers[samplerIndex], input.uv);
    if (albedoTexture.a < 0.5) {
        discard;
    }

    // Fetch default or fallback material factors 
    float4 baseColorFactor = float4(1.0, 1.0, 1.0, 1.0);

    // Combine base color texture with material factor multiplier
    float4 finalColor = albedoTexture * baseColorFactor;

    // Use dynamic directional light from SceneUniformData (Set 1)
    float3 lightDir = normalize(-scene.sunLight.direction); // Direction light points toward scene
    float3 N = normalize(input.normal);
    float NdotL = max(dot(N, lightDir), 0.1); // Ambient floor

    // Apply color, intensity, and lighting factor
    finalColor.rgb *= scene.sunLight.color * scene.sunLight.intensity * NdotL;

    return finalColor;

    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////

    //float3 lightColor = float3(0.0, 0.0, 0.0);
    //
    //// Ambient
    //float3 ambient = 0.1 * lightColor;
    //
    //// Diffuse
    //float3 N = normalize(input.normal);
    //float3 lightDir = normalize(float3(0.5, 1.0, 0.3) - input.fragPos);
    //float diff = max(dot(N, lightDir), 0.0);
    //float3 diffuse = diff * lightColor;
    //
    //// Specular
    //float3 viewDir = normalize(scene.cameraPos.xyz - input.fragPos);
    //float3 reflectDir = reflect(-lightDir, N);
    //float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    //float3 specular = 0.5 * spec * lightColor;
    //
    //float3 result = (ambient + diffuse + specular) * albedoTexture.xyz;
    //return float4(result, 1.0);
}