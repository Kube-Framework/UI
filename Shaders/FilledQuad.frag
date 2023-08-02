#version 450

// #extension GL_EXT_debug_printf : enable
// #extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "PrimitiveFrag.glsl"

// Inputs
layout(location = 0) in vec4 fragColor;
layout(location = 1) in flat vec2 fragCenter;
layout(location = 2) in flat vec2 fragHalfSize;
layout(location = 3) in flat vec4 fragRadius;
layout(location = 4) in vec2 fragUV;
layout(location = 5) in vec4 fragBorderColor;
layout(location = 6) in flat float fragBorderWidth;
layout(location = 7) in flat uint fragSpriteIndex;
layout(location = 8) in flat float fragEdgeSoftness;
layout(location = 9) in flat vec2 fragRotationCosSin;

// Outputs
layout(location = 0) out vec4 outColor;

// Box signed distance field supporting 4 independent radius
float roundedBoxSDF(const vec2 point, const vec2 center, const vec2 halfSize, const vec4 radius4)
{
    // Select nearest radius
    const float isTop = float(point.y < center.y);
    const float isLeft = float(point.x < center.x);
    const vec2 radius2 = (isTop * radius4.xy) + ((1.0 - isTop) * radius4.zw);
    const float radius = (isLeft * radius2.x) + ((1.0 - isLeft) * radius2.y);

    // Compute SDF
    const vec2 componentWiseEdgeDistance = abs(point - center) - halfSize + radius;
    const float outsideDistance = length(max(componentWiseEdgeDistance, 0.0));
    const float insideDistance = min(max(componentWiseEdgeDistance.x, componentWiseEdgeDistance.y), 0.0);
    return (outsideDistance + insideDistance) + 0.25 - radius;
}

mat2 getInversedRotationMatrix(const vec2 rotationCosSin)
{
    return mat2(
        rotationCosSin.x, rotationCosSin.y,
        -rotationCosSin.y, rotationCosSin.x
    );
}

void main(void)
{
    // Fill by color
    if (fragSpriteIndex == NullSpriteIndex) {
        outColor = fragColor;
    // Fill by texture
    } else {
        vec4 textureColor = texture(sprites[fragSpriteIndex], fragUV);
        // Raw texture
        if (fragColor.a == 0.0) {
            outColor = textureColor;
        // Recolored texture
        } else {
            outColor = textureColor * fragColor;
        }
    }

    // Compute SDF alpha
    const vec2 inversedPoint = applyRotation(getInversedRotationMatrix(fragRotationCosSin), fragCenter, gl_FragCoord.xy);
    const float dist = roundedBoxSDF(inversedPoint, fragCenter, fragHalfSize, fragRadius);

    // Smooth the border by antialiasing
    const float smoothedBorderAlpha = float(fragBorderWidth != 0.0) * smoothstep(-(fragBorderWidth + fragEdgeSoftness), -fragBorderWidth, dist);
    outColor = fragBorderColor * smoothedBorderAlpha + outColor * (1.0 - smoothedBorderAlpha);

    // Smooth the outer bound by antialiasing
    const float smoothedAlpha =  smoothstep(fragEdgeSoftness, 0.0 , dist);
    outColor.a *= smoothedAlpha;
}
