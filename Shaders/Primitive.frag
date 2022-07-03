#version 450

// #extension GL_EXT_debug_printf : enable
// #extension GL_EXT_nonuniform_qualifier : enable

// Constants
layout(constant_id = 0) const uint MaxSpriteCount = 1;
const uint NullSpriteIndex = ~0;

// Inputs
layout(location = 0) in vec4 fragColor;
layout(location = 1) in flat vec2 fragCenter;
layout(location = 2) in flat vec2 fragHalfSize;
layout(location = 3) in flat vec4 fragRadius;
layout(location = 4) in vec2 fragUV;
layout(location = 5) in flat uint fragSpriteIndex;
layout(location = 6) in flat float fragEdgeSoftness;
layout(location = 7) in flat vec2 fragRotationCosSin;

// Outputs
layout(location = 0) out vec4 outColor;

// Bindings
layout(set = 0, binding = 0) uniform sampler2D sprites[MaxSpriteCount];

// Box signed distance field supporting 4 independent radius
float roundedBoxSDF(const vec2 point, const vec2 center, const vec2 halfSize, const vec4 radius4)
{
    // Select nearest radius
    const float isTop = float(point.y < center.y);
    const float isLeft = float(point.x < center.x);
    const vec2 radius2 = (isTop * radius4.xy) + ((1.0 - isTop) * radius4.zw);
    const float radius = (isLeft * radius2.x) + ((1.0 - isLeft) * radius2.y);

    // Compute SDF
    return length(max(abs(point - center) - halfSize + radius, 0.0)) - radius;
}

mat2 getInversedRotationMatrix(const vec2 rotationCosSin)
{
    return mat2(
        rotationCosSin.x, rotationCosSin.y,
        -rotationCosSin.y, rotationCosSin.x
    );
}

vec2 applyRotation(const mat2 matrix, const vec2 origin, const vec2 point)
{
    return (matrix * (point - origin)) + origin;
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
            float alpha = textureColor.a * fragColor.a;
            outColor = vec4(textureColor.rgb * (1.0 - alpha) + fragColor.rgb * alpha, alpha);
        }
    }

    // Compute SDF alpha
    const vec2 inversedPoint = applyRotation(getInversedRotationMatrix(fragRotationCosSin), fragCenter, gl_FragCoord.xy);
    const float dist = roundedBoxSDF(inversedPoint, fragCenter, fragHalfSize - fragEdgeSoftness, fragRadius);
    // Smooth the result by antialiasing
    const float smoothedAlpha =  1.0 - smoothstep(0.0, fragEdgeSoftness * 2.0, dist);
    // Apply final alpha
    outColor.a *= smoothedAlpha;
}
