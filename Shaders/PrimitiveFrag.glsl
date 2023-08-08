// Constants - Common
const float PI = radians(180);
const float PI2 = 2 * PI;
const float Epsilon = 0.00001;

// Constants - Sprites
layout(constant_id = 0) const uint MaxSpriteCount = 4096;
const uint NullSpriteIndex = ~0;

// Constants - FillMode
const uint FillModeCrop = 0;
const uint FillModeFit = 1;
const uint FillModeStretch = 2;

// Context section
layout(std430, binding = 0) buffer ContextSection {
    vec2 windowSize;
    vec2 halfWindowSize;
} context;

// Samplers
layout(set = 1, binding = 0) uniform sampler2D sprites[MaxSpriteCount]; // 640 FOR M1


vec2 applyRotation(const mat2 matrix, const vec2 origin, const vec2 point)
{
    return (matrix * (point - origin)) + origin;
}

vec2 toRelative(const vec2 point)
{
    return (point / context.halfWindowSize) - 1.0;
}

vec2 toAbsolute(const vec2 point)
{
    return (1.0 + point) * context.halfWindowSize;
}