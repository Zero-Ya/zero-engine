
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
    float2 inPosition;
};

struct VSOutput {
    float4 pos : SV_Position;
};

// SET 0: Global Frame Data (Binding 0)
[[vk::binding(0, 0)]] 
uniform ConstantBuffer<CameraData> ubo;

// SET 1: Pass Data (e.g., Shadow Map or G-Buffer Input)
[[vk::binding(0, 1)]] 
Sampler2D g_ShadowMap;

// SET 2: Material Resources (Binding 0: Params UBO, Binding 1: Albedo Texture)
[[vk::binding(0, 2)]] 
uniform ConstantBuffer<MaterialParams> g_Material;

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

    float4 position = float4(input.inPosition, 0.0, 1.0);
    float4 objectPos = mul(g_Push.transform, position);
    float4 mvpPos = mul(ubo.proj, mul(ubo.view, mul(ubo.model, objectPos)));

    output.pos = mvpPos;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
   return float4(1.0, 0.0, 0.0, 1.0);
}