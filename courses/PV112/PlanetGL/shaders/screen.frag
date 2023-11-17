#version 450

uniform sampler2D renderTexture;

in vec2 UV;

out vec4 color;

void main() {
    color = texture(renderTexture, UV);
}

