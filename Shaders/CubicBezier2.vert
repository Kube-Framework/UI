#version 450
// #extension GL_EXT_debug_printf : enable

// Inputs
layout(location = 0) in vec2 vertPos;
layout(location = 1) in uint vertColor;
layout(location = 2) in float vertThickness;
layout(location = 3) in float vertEdgeSoftness;

// Outputs
layout(location = 0) out vec2 geometryPos;
layout(location = 1) out uint geometryColor;
layout(location = 2) out float geometryThickness;
layout(location = 3) out float geometryEdgeSoftness;

void main(void)
{
    gl_Position = vec4(vertPos, 0.0, 1.0);
    geometryColor = vertColor;
    geometryThickness = vertThickness;
    geometryEdgeSoftness = vertEdgeSoftness;
}