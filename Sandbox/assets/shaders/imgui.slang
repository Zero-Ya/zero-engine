struct VSInput {
    [[vk::location(0)]] float2 inPosition;
    [[vk::location(1)]] float2 inTexCoord;
    [[vk::location(2)]] float4 inColor;
};

struct VSOutput {
    float4 pos : SV_Position;
    [[vk::location(0)]] float4 fragColor;
    [[vk::location(1)]] float2 fragTexCoord;
};

struct PushConstants {
    float2 Scale;
    float2 Translate;
};

[[vk::push_constant]] PushConstants pushConstants;
[[vk::binding(0, 0)]] Sampler2D fontTexture;

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;

    // Transform position
    output.pos = float4(input.inPosition * pushConstants.Scale + pushConstants.Translate, 0.0, 1.0);

    // Pass texture coordinates and color to fragment shader
    output.fragTexCoord = input.inTexCoord;
    output.fragColor = input.inColor;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_TARGET
{
    // Sample font texture and multiply by color
    float4 color = input.fragColor * fontTexture.Sample(input.fragTexCoord);
    return color;
}
