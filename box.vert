#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTex; // ne koristi se ali mora postojati zbog stride
out vec2 dummyTex;

void main()
{
    gl_Position = vec4(inPos, 0.0, 1.0);
    dummyTex = inTex;
}
