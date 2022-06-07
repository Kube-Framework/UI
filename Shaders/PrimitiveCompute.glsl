// Constants - Common
const float PI = radians(180);
const float PI2 = 2 * PI;
const float Epsilon = 0.00001;

// Constants - Sprites
layout(constant_id = 0) const uint MaxSpriteCount = 1;
const uint NullSpriteIndex = ~0;

// Constants - FillMode
const uint FillModeCrop = 0;
const uint FillModeFit = 1;
const uint FillModeStretch = 2;

// Vertex
struct Vertex
{
    vec2 pos;
    vec2 center;
    vec2 halfSize;
    vec4 radius;
    vec2 uv;
    uint spriteIndex;
    uint color;
    float edgeSoftness;
    uint _padding;
    vec2 rotationCosSin;
};

// Offset
struct Offset
{
    uint vertexOffset;
    uint indexOffset;
};

// Context section
layout(std430, binding = 0) buffer ContextSection {
    vec2 windowSize;
    vec2 halfWindowSize;
} context;

// Instances section
layout(set = 0, binding = 1) buffer Instances { Instance data[]; } instances;

// Offsets section
layout(set = 0, binding = 2) buffer Offsets { Offset data[]; } offsets;

// Vertices section
layout(std430, set = 0, binding = 3) buffer Vertices { Vertex data[]; } vertices;

// Indices section
layout(std430, set = 0, binding = 4) buffer Indices { uint data[]; } indices;

// Samplers
layout(set = 1, binding = 0) uniform sampler2D sprites[MaxSpriteCount];


vec2 applyRotation(const mat2 matrix, const vec2 origin, const vec2 point)
{
    return (matrix * (point - origin)) + origin;
}