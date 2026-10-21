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

struct PushConstants {
    uint   textureIndex;
    uint   samplerIndex;
};



// ----------------------------------------------------------------------------
// Resource Bindings
// ----------------------------------------------------------------------------

// Set 0: Global Bindless Textures, Samplers, and Material SSBO
[[vk::binding(0, 0)]] TextureCube      g_CubeTextures[];
[[vk::binding(1, 0)]] SamplerState     g_Samplers[];

// Set 1: Per-Frame / Scene Data
[[vk::binding(0, 1)]] ConstantBuffer<SceneUniformData> scene;

// Push Constants
[[vk::push_constant]]
PushConstants g_Push;



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



// ----------------------------------------------------------------------------
// Vertex Input / Output
// ----------------------------------------------------------------------------

struct VSOutput {
    float4 position : SV_Position;
    float3 uv       : TEXCOORD0;
};



// ----------------------------------------------------------------------------
// Vertex Shader
// ----------------------------------------------------------------------------

[shader("vertex")]
VSOutput vertMain(uint vertexID : SV_VertexID) {
    VSOutput output;

    // Get position from index lookup
    uint index = CUBE_INDICES[vertexID];
    float3 pos = CUBE_VERTICES[index];

    // Local position acts as 3D Direction Vector for Cubemap sampling
    output.uv = pos;

    // Strip translation from view matrix so skybox stays centered on camera
    float3x3 staticView = (float3x3)scene.camera.view;

    float4 clipPos = mul(scene.camera.proj, float4(mul(staticView, pos), 1.0));

    // Force z = w so that z/w = 1.0 (Far Plane) after perspective division
    output.position = clipPos.xyww;

    return output;
}



// ----------------------------------------------------------------------------
// Fragment Shader
// ----------------------------------------------------------------------------

[shader("fragment")]
float4 fragMain(VSOutput output) : SV_Target {
    float3 skyboxTexture = g_CubeTextures[g_Push.textureIndex].Sample(g_Samplers[g_Push.samplerIndex], output.uv).rgb;
    return float4(skyboxTexture, 1.0);
}