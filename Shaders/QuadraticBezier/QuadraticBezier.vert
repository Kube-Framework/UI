#version 450

// Inputs
layout(location = 0) in vec2 vertPos;
layout(location = 1) in vec2 vertLeft;
layout(location = 2) in vec2 vertControl;
layout(location = 3) in vec2 vertRight;
layout(location = 4) in uint vertColor;
layout(location = 5) in uint vertInnerColor;
layout(location = 6) in float vertThickness;
layout(location = 7) in float vertEdgeSoftness;

// Outputs
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragInnerColor;
layout(location = 2) out flat vec2 fragLeft;
layout(location = 3) out flat vec2 fragControl;
layout(location = 4) out flat vec2 fragRight;
layout(location = 5) out flat float fragThickness;
layout(location = 6) out flat float fragEdgeSoftness;


void main(void)
{
    gl_Position = vec4(vertPos, 0.0, 1.0);
    fragColor = unpackUnorm4x8(vertColor);
    fragInnerColor = unpackUnorm4x8(vertInnerColor);
    fragLeft = vertLeft;
    fragControl = vertControl;
    fragRight = vertRight;
    fragThickness = vertThickness;
    fragEdgeSoftness = vertEdgeSoftness;
}