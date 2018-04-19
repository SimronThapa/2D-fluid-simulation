#version 330 core
in vec2 TexCoords;

uniform sampler2D velocity1;

layout(location = 2) out vec4 velocity0;

void main() {
	velocity0 = texture(velocity1, TexCoords);
}