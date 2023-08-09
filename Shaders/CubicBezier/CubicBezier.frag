#version 450

// #extension GL_EXT_nonuniform_qualifier : enable
// #extension GL_OES_standard_derivatives : enable
#extension GL_GOOGLE_include_directive : enable

#include "CubicBezier.glsl"

// Inputs
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragP0;
layout(location = 2) in vec2 fragP1;
layout(location = 3) in vec2 fragP2;
layout(location = 4) in vec2 fragP3;
layout(location = 5) in float fragThickness;
layout(location = 6) in float fragEdgeSoftness;

// Outputs
layout(location = 0) out vec4 outColor;

// Return point on cubic from parameter
vec2 cubic(float t)
{
    float tt = t * t;
    float ttt = tt * t;
    return fragP0 + fragP1 * t + fragP2 * tt + fragP3 * ttt;
}

void main(void)
{
    const int IterationCount = 2;
    const int DivisionCount = 10;

    float from = 0.0;
    float to = 1.0;
    float minPos1 = 0.0;
    float minPos2 = 0.0;
    float minDist1 = 1000000000.0;
    float minDist2 = 1000000000.0;

    for (int iteration = 0; iteration != IterationCount; ++iteration) {
        const float delta = (to - from) / float(DivisionCount);
        for (int division = 0; division != DivisionCount; ++division) {
            const float cubicPos = from + delta * division;
            const float cubicDist = distance(cubic(cubicPos), gl_FragCoord.xy);
            const float closer = float(cubicDist <= minDist1);
            const float further = 1.0 - closer;
            minPos2 = minPos2 * further + minPos1 * closer;
            minDist2 = minDist2 * further + minDist1 * closer;
            minPos1 = minPos1 * further + cubicPos * closer;
            minDist1 = minDist1 * further + cubicDist * closer;
        }
        from = min(minPos1, minPos2);
        to = max(minPos1, minPos2);
    }
    if (minDist1 > 10.0)
        discard;
    // const float intensity = smoothstep(fragThickness + fragEdgeSoftness, fragThickness, minDist1);
    outColor = fragColor; //vec4(fragColor.rgb, fragColor.a * intensity);
}