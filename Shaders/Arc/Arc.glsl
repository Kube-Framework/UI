#include "../Primitive.glsl"

// Vertex
struct Vertex
{
    vec2 pos;
    vec2 center;
    float radius;
    float thickness;
    float aperture;
    uint color;
    uint borderColor;
    float borderWidth;
    float edgeSoftness;
    float rotationAngle;
};