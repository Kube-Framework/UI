#version 450
// #extension GL_EXT_debug_printf : enable

// Inputs
layout(location = 0) in vec2 vertPos;
layout(location = 1) in vec2 vertCenter;
layout(location = 2) in vec2 vertHalfSize;
layout(location = 3) in vec4 vertRadius;
layout(location = 4) in vec2 vertUV;
layout(location = 5) in uint vertSpriteIndex;
layout(location = 6) in uint vertColor;
layout(location = 7) in float vertEdgeSoftness;
layout(location = 8) in vec2 vertRotationCosSin;

// Outputs
layout(location = 0) out vec4 fragColor;
layout(location = 1) out flat vec2 fragCenter;
layout(location = 2) out flat vec2 fragHalfSize;
layout(location = 3) out flat vec4 fragRadius;
layout(location = 4) out vec2 fragUV;
layout(location = 5) out flat uint fragSpriteIndex;
layout(location = 6) out flat float fragEdgeSoftness;
layout(location = 7) out flat vec2 fragRotationCosSin;

void main(void)
{
    // debugPrintfEXT("> Vert spriteIndex: %u", vertSpriteIndex);
    gl_Position = vec4(vertPos, 0.0, 1.0);
    fragColor = unpackUnorm4x8(vertColor);
    fragCenter = vertCenter;
    fragHalfSize = vertHalfSize;
    fragRadius = vertRadius;
    fragUV = vertUV;
    fragSpriteIndex = vertSpriteIndex;
    fragEdgeSoftness = vertEdgeSoftness;
    fragRotationCosSin = vertRotationCosSin;
}