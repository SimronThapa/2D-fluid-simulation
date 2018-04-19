#version 330 core
in vec2 TexCoords;

uniform sampler2D pressure1;

layout(location = 5) out vec4 pressure0;

void main() {
	pressure0 = texture(pressure1, TexCoords);
}