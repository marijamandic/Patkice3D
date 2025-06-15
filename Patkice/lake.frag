#version 330 core
in vec2 TexCoord;

uniform sampler2D texture1;

out vec4 FragColor;

void main()
{
    FragColor = vec4(texture(texture1, TexCoord).rgb, 0.8);
}
