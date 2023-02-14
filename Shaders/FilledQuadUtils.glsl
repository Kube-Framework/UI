// Vertex
struct Vertex
{
    vec2 pos;
    vec2 center;
    vec2 halfSize;
    vec2 uv;
    vec4 radius;
    uint spriteIndex;
    uint color;
    uint borderColor;
    float borderWidth;
    float edgeSoftness;
    uint _padding;
    vec2 rotationCosSin;
};