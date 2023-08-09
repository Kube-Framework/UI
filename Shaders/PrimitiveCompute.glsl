// PLEASE INCLUDE THIS FILE AFTER DECLARING AN 'Instance' STRUCTURE

// Offset
struct Offset
{
    uint vertexOffset;
    uint indexOffset;
};

// Instances section
layout(std140, set = 0, binding = 1) buffer Instances { Instance data[]; } instances;

// Offsets section
layout(std430, set = 0, binding = 2) buffer Offsets { Offset data[]; } offsets;

// Vertices section
layout(std140, set = 0, binding = 3) buffer Vertices { Vertex data[]; } vertices;

// Indices section
layout(std430, set = 0, binding = 4) buffer Indices { uint data[]; } indices;
