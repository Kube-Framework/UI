#version 450

// #extension GL_EXT_nonuniform_qualifier : enable
// #extension GL_OES_standard_derivatives : enable
#extension GL_GOOGLE_include_directive : enable

#include "Arc.glsl"

// Inputs
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec4 fragBorderColor;
layout(location = 2) in flat vec2 fragCenter;
layout(location = 3) in flat float fragRadius;
layout(location = 4) in flat float fragThickness;
layout(location = 5) in flat vec2 fragApertureCosSin;
layout(location = 6) in flat float fragBorderWidth;
layout(location = 7) in flat float fragEdgeSoftness;
layout(location = 8) in flat vec2 fragRotationCosSin;

// Outputs
layout(location = 0) out vec4 outColor;

float arcSdf(const vec2 pos, const vec2 center, const float radius, const float thickness, const vec2 cosSinAperture)
{
    vec2 target = vec2(abs(pos.x - center.x), pos.y - center.y);
    float dist = cosSinAperture.x * target.x > cosSinAperture.y * target.y
        ? length(target - cosSinAperture.yx * radius)
        : abs(length(target) - radius);
    return dist - thickness;
}

void main(void)
{
    const vec2 inversedPoint = applyRotation(getInversedRotationMatrix(fragRotationCosSin), fragCenter, gl_FragCoord.xy);
    const float dist = arcSdf(inversedPoint, fragCenter, fragRadius, fragThickness, fragApertureCosSin);

    // Smooth the border by antialiasing
    const float smoothedBorderAlpha = float(fragBorderWidth != 0.0) * smoothstep(-(fragBorderWidth + fragEdgeSoftness), -fragBorderWidth, dist);
    outColor = fragBorderColor * smoothedBorderAlpha + fragColor * (1.0 - smoothedBorderAlpha);

    // Smooth the outer bound by antialiasing
    const float smoothedAlpha = smoothstep(max(fragEdgeSoftness, Epsilon), 0.0 , dist);
    outColor.a *= smoothedAlpha;
}
