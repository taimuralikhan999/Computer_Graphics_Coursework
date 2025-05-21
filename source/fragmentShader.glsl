#version 330 core

uniform vec3 baseColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(baseColor, 1.0);
}
