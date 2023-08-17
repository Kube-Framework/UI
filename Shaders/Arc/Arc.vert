#version 450

#extension GL_GOOGLE_include_directive : enable

#include "../Primitive.glsl"

// Inputs
layout(location = 0) in vec2 vertPos;
layout(location = 1) in vec2 vertCenter;
layout(location = 2) in float vertRadius;
layout(location = 3) in float vertThickness;
layout(location = 4) in float vertAperture;
layout(location = 5) in uint vertColor;
layout(location = 6) in uint vertBorderColor;
layout(location = 7) in float vertBorderWidth;
layout(location = 8) in float vertEdgeSoftness;
layout(location = 9) in float vertRotationAngle;

// Outputs
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragBorderColor;
layout(location = 2) out flat vec2 fragCenter;
layout(location = 3) out flat float fragRadius;
layout(location = 4) out flat float fragThickness;
layout(location = 5) out flat vec2 fragApertureCosSin;
layout(location = 6) out flat float fragBorderWidth;
layout(location = 7) out flat float fragEdgeSoftness;
layout(location = 8) out flat vec2 fragRotationCosSin;

void main(void)
{
    gl_Position = vec4(vertPos, 0.0, 1.0);
    fragColor = unpackUnorm4x8(vertColor);
    fragBorderColor = unpackUnorm4x8(vertBorderColor);
    fragCenter = vertCenter;
    fragRadius = vertRadius;
    fragThickness = vertThickness;
    fragApertureCosSin = getCosSin(vertAperture);
    fragBorderWidth = vertBorderWidth;
    fragEdgeSoftness = vertEdgeSoftness;
    fragRotationCosSin = getCosSin(vertRotationAngle);
}