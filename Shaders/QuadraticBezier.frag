#version 450

// #extension GL_EXT_debug_printf : enable
// #extension GL_EXT_nonuniform_qualifier : enable
// #extension GL_OES_standard_derivatives : enable
#extension GL_GOOGLE_include_directive : enable

#include "PrimitiveFrag.glsl"

// Inputs
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec4 fragInnerColor;
layout(location = 2) in flat vec2 fragLeft;
layout(location = 3) in flat vec2 fragControl;
layout(location = 4) in flat vec2 fragRight;
layout(location = 5) in flat float fragThickness;
layout(location = 6) in flat float fragEdgeSoftness;

// Outputs
layout(location = 0) out vec4 outColor;

float sdfSegment(const vec2 p, const vec2 a, const vec2 b)
{
    const vec2 pa = p - a;
    const vec2 ba = b - a;
    const float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

vec2 quadraticBezier(const vec2 a, const vec2 b, const vec2 c, const float t)
{
    const float oneMinusT = (1.0 - t);
    return oneMinusT * oneMinusT * a + 2.0 * t * oneMinusT * b + t * t * c;
}

float sdfBezier(const vec2 p, const vec2 a, const vec2 b, const vec2 c)
{
    // Define iteration count
    const float iterations = 20.0;
    const float iterationStep = 1.0 / float(iterations - 1.0);

    // Prepare cache
    vec2 res = vec2(1e10, 0.0);
    vec2 lastCurvePos = quadraticBezier(a, b, c, 0.0);

    // Render the curve
    for (float index = 1.0; index != iterations; ++index) {
        const float t = index * iterationStep;
        const vec2 curvePos = quadraticBezier(a, b, c, t);
        const float segmentDist = sdfSegment(p, lastCurvePos, curvePos);
        const float closer = float(segmentDist < res.x);
        // y = a * x + b -> compute sign relative from p to curve segment
        const float a = (curvePos.y - lastCurvePos.y) / (curvePos.x - lastCurvePos.x);
        const float b = lastCurvePos.y - a * lastCurvePos.x;
        const float s = float(a * p.x + b - p.y > 0.0) * 2.0 - 1.0;
        res = vec2(
            segmentDist * closer + (1.0 - closer) * res.x,
            s * closer + (1.0 - closer) * res.y
        );
        lastCurvePos = curvePos;
    }
    return res.x * res.y;
}

void main(void)
{
    const float d = sdfBezier(
        gl_FragCoord.xy,
        fragLeft,
        fragControl,
        fragRight
    );
    const float totalThickness = fragThickness + fragEdgeSoftness;
    const float minIntensity = 0.1 * float(d < -totalThickness) * float(fragInnerColor != vec4(0.0));
    const float strokeIntensity = minIntensity + (1.0 - minIntensity) * smoothstep(totalThickness, fragThickness, abs(d));
    outColor = vec4(fragColor.rgb, fragColor.a * strokeIntensity);
}
