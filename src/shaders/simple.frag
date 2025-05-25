#version 330 core

out vec4 FragColor;

in vec3 outColorToFrag; // Input color from vertex shader (matches 'out' name)

void main()
{
    FragColor = vec4(outColorToFrag, 1.0f);
} 