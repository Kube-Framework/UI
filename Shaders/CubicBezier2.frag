#version 450

// #extension GL_EXT_debug_printf : enable
// #extension GL_EXT_nonuniform_qualifier : enable
// #extension GL_OES_standard_derivatives : enable

// Constants
layout(constant_id = 0) const uint MaxSpriteCount = 1;
const uint NullSpriteIndex = ~0;

// #define show_coverage

// Inputs
layout(location = 0) in vec2 fragPos; // Fragment position
layout(location = 1) in vec3 fragColor; // Fragment color
layout(location = 2) in vec2 fragA0; // Begin
layout(location = 3) in vec2 fragA1; // Coefficient 1
layout(location = 4) in vec2 fragA2; // Coefficient 2
layout(location = 5) in vec2 fragA3; // End
layout(location = 6) in vec2 fragRange; // T range

// Outputs
layout(location = 0) out vec4 outColor;

// Bindings
layout(set = 0, binding = 0) uniform sampler2D sprites[MaxSpriteCount];

// Compute cubic bezier
vec2 cubicBezier(float t)
{
    float t2 = t * t;
    float t3 = t2 * t;
    return fragA0 + (fragA1 * t) + (fragA2 * t2) + (fragA3 * t3);
}

void main(void)
{
    vec2 p;
    int i, n;
    float t, tt, t0, t1, dt, l, ll;
    tt = -1.0;
    ll = -1.0;
    l = 0.0;
#ifdef show_coverage
    t0 = 0.0;
    t1 = 1.0;
    dt = 0.05;
    n = 3;
#else
    t0 = fragRange.x;
    n = 2;
    t1 = fragRange.y;
    dt = (t1 - t0) * 0.1;
#endif
    for (i = 0; i < n; i++) {
        for (t = t0; t <= t1; t += dt) {
            p = cubicBezier(t) - fragPos;
            l = length(p);
            if ((ll < 0.0) || (ll > l)) {
                ll = l;
                tt = t;
            }
        }
        t0 = tt - dt;
        if (t0 < 0.0)
            t0 = 0.0;
        t1 = tt + dt;
        if (t1 > 1.0)
            t1 = 1.0;
        dt *= 0.2;
    }
#ifdef show_coverage
    if (ll > fragThickness)
        outColor = vec4(0.1, 0.1, 0.1, 1.0);
    else
#else
    if (ll > fragThickness)
        discard;
#endif
        outColor = vec4(fragColor, 1.0);
}