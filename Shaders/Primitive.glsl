// Enable the use of nonuniformEXT(index) to access bindless textures
#extension GL_EXT_nonuniform_qualifier : enable

// Constants - Common
const float PI = radians(180);
const float PI2 = 2 * PI;
const float Epsilon = 0.00001;

// Constants - Sprites
layout(constant_id = 0) const uint MaxSpriteCount = 1024;
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

// Samplers, to be accessed with nonuniformEXT(index)
layout(set = 1, binding = 0) uniform sampler2D sprites[MaxSpriteCount];

// Push constants
layout(push_constant) uniform ComputeConstants
{
    uint instanceCount;
} computeConstants;

// Area
struct Area
{
    vec2 pos;
    vec2 size;
};

Area getClampedArea(const Area area)
{
    Area clamped;
    clamped.pos = round(area.pos);
    clamped.size = round(area.pos + area.size) - clamped.pos;
    return clamped;
}

vec2 getCosSin(const float angle)
{
    return vec2(cos(angle), sin(angle));
}

mat2 getRotationMatrix(const vec2 rotationCosSin)
{
    return mat2(
        rotationCosSin.x, -rotationCosSin.y,
        rotationCosSin.y, rotationCosSin.x
    );
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

vec2 toRelative(const vec2 point)
{
    return (point / context.halfWindowSize) - 1.0;
}

vec2 toAbsolute(const vec2 point)
{
    return (1.0 + point) * context.halfWindowSize;
}


vec2 branchlessIf(const bool test, const vec2 onTestPassed, const vec2 onTestFailed)
{
    return float(test) * onTestPassed + float(!test) * onTestFailed;
}