#version 450

// #extension GL_EXT_debug_printf : enable
// #extension GL_EXT_nonuniform_qualifier : enable
// #extension GL_OES_standard_derivatives : enable

// Constants
layout(constant_id = 0) const uint MaxSpriteCount = 1;
const uint NullSpriteIndex = ~0;

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

// Bindings
layout(set = 0, binding = 0) uniform sampler2D sprites[MaxSpriteCount];


// void main(void)
// {
//     outColor = vec4(1.0, 1.0, 1.0, 1.0);
// }


vec2 cubic(float t)     // return point on cubic from parameter
{
    float tt = t * t;
    float ttt = tt * t;
    return fragP0 + fragP1 * t + fragP2 * tt + fragP3 * ttt;
}

#define show_coverage

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


//     float d = 5.0; // half thickness
//     vec2 p;
//     int i, n;
//     float t, tt, t0, t1, dt, l, ll;
//     tt = -1.0;
//     ll = -1.0;
//     l = 0.0;
// #ifdef show_coverage
//     t0 = 0.0;
//     t1 = 1.0;
//     dt = 0.05;
//     n = 3;
// #else
//     t0 = 0.0;
//     n = 2;
//     t1 = 1.0;
//     dt = (t1 - t0) * 0.1;
// #endif
//     for (i = 0; i < n; i++) {
//         for (t = t0; t <= t1; t += dt) {
//             p = cubic(t) - gl_FragCoord.xy;
//             l = length(p);
//             if ((ll < 0.0) || (ll > l)) {
//                 ll = l;
//                 tt = t;
//             }
//         }
//         t0 = tt - dt;
//         if (t0 < 0.0)
//             t0 = 0.0;
//         t1 = tt + dt;
//         if (t1 > 1.0)
//             t1 = 1.0;
//         dt *= 0.2;
//     }
// #ifdef show_coverage
//     if (ll > d)
//         outColor = vec4(0.1, 0.1, 0.1, 1.0);
//     else
// #else
//     if (ll > d)
//         discard;
// #endif
    // outColor = fragColor;
}