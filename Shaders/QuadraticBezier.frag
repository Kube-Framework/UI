#version 450

// #extension GL_EXT_debug_printf : enable
// #extension GL_EXT_nonuniform_qualifier : enable

// Constants
layout(constant_id = 0) const uint MaxSpriteCount = 1;
const uint NullSpriteIndex = ~0;

// Inputs
layout(location = 0) in vec4 fragColor;
layout(location = 1) in flat vec2 fragLeft;
layout(location = 2) in flat vec2 fragRight;
layout(location = 3) in flat vec2 fragControl;
layout(location = 4) in flat float fragThickness;
layout(location = 5) in flat float fragEdgeSoftness;

// Outputs
layout(location = 0) out vec4 outColor;

// Bindings
layout(set = 0, binding = 0) uniform sampler2D sprites[MaxSpriteCount];


float det(const vec2 a, const vec2 b) { return a.x * b.y - b.x * a.y; }

float sdfBezier(const vec2 pos, const vec2 left, const vec2 right, const vec2 control)
{
    const vec2 b0 = left - pos;
    const vec2 b1 = control - pos;
    const vec2 b2 = right - pos;
    const float a = det(b0, b2);
    const float b = 2.0 * det(b1, b0);
    const float d = 2.0 * det(b2, b1);
    const float f = b * d - a * a;
    const vec2 d21 = b2 - b1;
    const vec2 d10 = b1 - b0;
    const vec2 d20 = b2 - b0;
    vec2 gf = 2.0 * (b * d21 + d * d10 + a * d20);
    gf = vec2(gf.y, -gf.x);
    const vec2 pp = -f * gf / dot(gf, gf);
    const vec2 d0p = b0 - pp;
    const float ap = det(d0p, d20);
    const float bp = 2.0 * det(d10, d0p);
    const float t = clamp((ap + bp) / (2.0 * a + b + d), 0.0, 1.0);
    const vec2 dist = mix(mix(b0, b1, t), mix(b1, b2, t), t);
    return length(dist);
}

void main(void)
{
	float d = sdfBezier(gl_FragCoord.xy - 0.5, fragLeft, fragRight, fragControl);
    outColor = vec4(
        fragColor.rgb,
        fragColor.a * smoothstep(fragThickness, 0.0, d)
    );
}
