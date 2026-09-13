
struct CameraData {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

struct MaterialParams {
    float4 albedo;
    float roughness;
    float metallic;
    float2 padding;
};

struct PushData {
    float4x4 transform;
};

//
//
//

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

//
//
//

// SET 0: Global Frame Data (Binding 0)
[[vk::binding(0, 0)]] 
ConstantBuffer<CameraData> ubo;

// SET 1: Pass Data (e.g., Shadow Map or G-Buffer Input)
[[vk::binding(0, 1)]] 
Sampler2D g_ShadowMap;

// SET 2: Material Resources (Binding 0: Params UBO, Binding 1: Albedo Texture)
[[vk::binding(0, 2)]] 
ConstantBuffer<MaterialParams> g_Material;

[[vk::binding(1, 2)]] 
Sampler2D g_AlbedoMap;

[[vk::push_constant]]
ConstantBuffer<PushData> g_Push;

//
//
//

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    
    // Combine Model (Push Constant) * View * Proj matrices
    float4 worldPos = mul(g_Push.transform, float4(input.position, 1.0));
    float4 viewPos  = mul(ubo.view, worldPos);
    output.position = mul(ubo.proj, viewPos);

    output.normal = mul((float3x3)g_Push.transform, input.normal);
    output.uv = input.uv;

    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    // 1. Sample base color texture from Set 2 (Binding 1)
    float4 albedoTex = g_AlbedoMap.Sample(input.uv);
    if (albedoTex.a < 0.5) {
        discard;
    }

    // 2. Fetch per-material factors from Set 2 (Binding 0 UBO)
    float4 baseColorFactor = g_Material.albedo;
    float roughness = g_Material.roughness;
    float metallic = g_Material.metallic;

    // 3. Combine base color texture with material factor multiplier
    float4 finalColor = albedoTex * baseColorFactor;

    // 4. Basic directional light test (Sun light vector)
    float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
    float3 N = normalize(input.normal);
    float NdotL = max(dot(N, lightDir), 0.1); // Add 0.1 ambient floor

    // Apply lighting factor to final RGB output
    finalColor.rgb *= NdotL;

    return finalColor;
}