# version 330 core
in vec2 TexCoords;

uniform sampler2D screenTexture;

layout(location = 0) out vec4 FragColor;
//in vec2 pos;
//uniform vec2 mousePos;

void main() {
	FragColor = abs(texture(screenTexture, TexCoords));
}