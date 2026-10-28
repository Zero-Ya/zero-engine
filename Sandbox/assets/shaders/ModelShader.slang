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
    float4x4         lightViewProj;
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

struct MeshPushConstants {
    float4x4 modelMatrix;
    uint   albedoTextureIndex;
    uint   normalTextureIndex;
    uint   roughnessTextureIndex;
    uint   samplerIndex;
};

struct ShadowMapData {
    uint   textureIndex;
    uint   samplerIndex;
};

struct PushConstants {
    MeshPushConstants meshConstants;
    ShadowMapData shadowMapData;
};



// ----------------------------------------------------------------------------
// Resource Bindings
// ----------------------------------------------------------------------------

// Set 0: Global bindless textures, samplers, material SSBO and comparison samplers
[[vk::binding(0, 0)]] Texture2D        g_Textures[];
[[vk::binding(1, 0)]] SamplerState     g_Samplers[];
[[vk::binding(2, 0)]] StructuredBuffer<MaterialData> g_Materials;
[[vk::binding(3, 0)]] SamplerComparisonState g_ShadowSamplers[];

// Set 1: Per-frame / Scene data
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
    float4 lightSpacePos : TEXCOORD1;
};



// ----------------------------------------------------------------------------
// Vertex Shader
// ----------------------------------------------------------------------------

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    
    float4 worldPos = mul(g_Push.meshConstants.modelMatrix, float4(input.position, 1.0));
    float4 viewPos  = mul(scene.camera.view, worldPos);
    output.position = mul(scene.camera.proj, viewPos);
    output.normal = mul((float3x3)g_Push.meshConstants.modelMatrix, input.normal);
    output.uv = input.uv;

    output.fragPos = worldPos.xyz;

    // Transform world position into light clip space using the light's view-projection matrix
    output.lightSpacePos = mul(scene.lightViewProj, worldPos);

    return output;
}



// Utility function

float CalculateShadow(float4 lightSpacePos, float3 normal, float3 lightDir) {
    // Perspective division
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    
    // Transform from [-1, 1] NDC to [0, 1] texture space
    projCoords.xy = projCoords.xy * 0.5f + 0.5f;
    projCoords.y = 1.0f - projCoords.y; // Vulkan Y-inversion

    // Out-of-bounds check (no shadow outside light frustum)
    if (projCoords.x < 0.0f || projCoords.x > 1.0f || 
        projCoords.y < 0.0f || projCoords.y > 1.0f || 
        projCoords.z > 1.0f || projCoords.z < 0.0f) {
        return 1.0f; 
    }

    // Dynamic bias based on surface angle to prevent shadow acne
    float bias = 0.0005f; // Adjust based on scene scale
    float currentDepth = projCoords.z - bias; 

    // PCF 3x3 soft shadows using SampleCmp directly
    float shadow = 0.0f;
    float2 texelSize = 1.0f / 2048.0f; // Assuming 2048x2048 shadow map resolution

    uint shadowTextureIndex = g_Push.shadowMapData.textureIndex;
    uint shadowSamplerIndex = g_Push.shadowMapData.samplerIndex;
    [unroll]
    for (int x = -1; x <= 1; ++x) {
        [unroll]
        for (int y = -1; y <= 1; ++y) {
            shadow += g_Textures[shadowTextureIndex].SampleCmp(g_ShadowSamplers[shadowSamplerIndex], projCoords.xy + float2(x, y) * texelSize, currentDepth);
        }
    }
    shadow /= 9.0f;
    
    return shadow;
}

// ----------------------------------------------------------------------------
// Fragment Shader
// ----------------------------------------------------------------------------

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    uint albedoTextureIndex = g_Push.meshConstants.albedoTextureIndex;
    uint samplerIndex       = g_Push.meshConstants.samplerIndex;

    float4 albedoTexture = g_Textures[albedoTextureIndex].Sample(g_Samplers[samplerIndex], input.uv);
    if (albedoTexture.a < 0.5) discard;

    // Lighting calculations
    float3 lightDir = normalize(-scene.sunLight.direction);
    float3 N = normalize(input.normal);
    float NdotL = max(dot(N, lightDir), 0.0f);

    float shadow = CalculateShadow(input.lightSpacePos, N, lightDir);

    float lightingFactor = max((shadow), 0.2f); // Ambient floor so it's not pitch black

    float3 finalColor = albedoTexture.rgb * scene.sunLight.color * scene.sunLight.intensity * lightingFactor;
    return float4(finalColor, albedoTexture.a);
}






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