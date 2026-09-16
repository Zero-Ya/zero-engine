
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

// Generate a unit cube directly in the vertex shader via 36 vertex indices
static const float3 CUBE_VERTICES[8] = {
    float3(-1.0, -1.0, -1.0),
    float3( 1.0, -1.0, -1.0),
    float3( 1.0,  1.0, -1.0),
    float3(-1.0,  1.0, -1.0),
    float3(-1.0, -1.0,  1.0),
    float3( 1.0, -1.0,  1.0),
    float3( 1.0,  1.0,  1.0),
    float3(-1.0,  1.0,  1.0)
};

static const uint CUBE_INDICES[36] = {
    // Back
    0, 2, 1, 0, 3, 2,
    // Front
    4, 5, 6, 4, 6, 7,
    // Left
    4, 7, 3, 4, 3, 0,
    // Right
    1, 2, 6, 1, 6, 5,
    // Bottom
    0, 1, 5, 0, 5, 4,
    // Top
    3, 7, 6, 3, 6, 2
};

struct VSOutput {
    float4 position : SV_Position;
    float3 uv       : TEXCOORD0;
};

// SET 0: Global Frame Data (Binding 0)
[[vk::binding(0, 0)]] 
ConstantBuffer<CameraData> camera;

// SET 1: Skybox Data
[[vk::binding(0, 1)]]
SamplerCube skyboxTexture;

// SET 2: Pass Data (e.g., Shadow Map or G-Buffer Input)
[[vk::binding(0, 2)]] 
Sampler2D g_ShadowMap;

// SET 3: Material Resources (Binding 0: Params UBO, Binding 1: Albedo Texture)
[[vk::binding(0, 3)]] 
ConstantBuffer<MaterialParams> g_Material;

[[vk::binding(1, 3)]] 
Sampler2D g_AlbedoMap;

[[vk::push_constant]]
ConstantBuffer<PushData> g_Push;

//
//
//

[shader("vertex")]
VSOutput vertMain(uint vertexID : SV_VertexID) {
    VSOutput output;

    // Get position from index lookup
    uint index = CUBE_INDICES[vertexID];
    float3 pos = CUBE_VERTICES[index];

    // Local position acts as 3D Direction Vector for Cubemap sampling
    output.uv = pos;

    // Strip translation from view matrix so skybox stays centered on camera
    float3x3 staticView = (float3x3)camera.view;
    float4 clipPos = mul(camera.proj, float4(mul(staticView, pos), 1.0));

    // Force z = w so that z/w = 1.0 (Far Plane) after perspective division
    output.position = clipPos.xyww;

    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    return skyboxTexture.Sample(input.uv);
}