#version 450

// Inputs
layout(location = 0) in vec2 vertPos;
layout(location = 1) in vec2 vertP0;
layout(location = 2) in vec2 vertP1;
layout(location = 3) in vec2 vertP2;
layout(location = 4) in vec2 vertP3;
layout(location = 5) in uint vertColor;
layout(location = 6) in float vertThickness;
layout(location = 7) in float vertEdgeSoftness;

// Outputs
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragP0;
layout(location = 2) out vec2 fragP1;
layout(location = 3) out vec2 fragP2;
layout(location = 4) out vec2 fragP3;
layout(location = 5) out float fragThickness;
layout(location = 6) out float fragEdgeSoftness;

void main(void)
{
    gl_Position = vec4(vertPos, 0.0, 1.0);
    fragColor = unpackUnorm4x8(vertColor);
    fragP0 = vertP0;
    fragP1 = vertP1;
    fragP2 = vertP2;
    fragP3 = vertP3;
    fragThickness = vertThickness;
    fragEdgeSoftness = vertEdgeSoftness;
}