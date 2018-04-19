#version 330 core
in vec2 TexCoords;

uniform sampler2D color1;

layout(location = 0) out vec4 color0;

void main() {
	color0 = texture(color1, TexCoords);
}