struct ShadowPushConstants {
    float4x4 lightViewProj;
    float4x4 modelMatrix;
};

[[vk::push_constant]] ShadowPushConstants g_ShadowPush;

struct VSInput {
    [[vk::location(0)]] float3 position : POSITION;
};

[shader("vertex")]
float4 vertMain(VSInput input) : SV_Position {
    float4 worldPos = mul(g_ShadowPush.modelMatrix, float4(input.position, 1.0f));
    return mul(g_ShadowPush.lightViewProj, worldPos);
}

// Optional empty fragment shader for strict pipelines or leave fragment stage unbound if allowed
[shader("fragment")]
void fragMain() {
    // No color output needed, depth is written automatically
}