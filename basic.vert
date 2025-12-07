#version 330 core

layout(location = 0) in vec2 aPos;

uniform mat4 uProjection;
uniform vec4 uRect; // x, y, w, h

void main()
{
    float x = uRect.x + aPos.x * uRect.z;
    float y = uRect.y + aPos.y * uRect.w;
    gl_Position = uProjection * vec4(x, y, 0.0, 1.0);
}
