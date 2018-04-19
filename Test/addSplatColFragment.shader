# version 330 core
in vec2 TexCoords;

uniform vec4 change;
uniform vec2 center;
uniform float radius;
uniform sampler2D inputTex;

layout(location = 1) out vec4 color1;

void main() {
	float dx = center.x - TexCoords.x;
	float dy = center.y - TexCoords.y;
	vec4 cur = texture(inputTex, TexCoords);
	color1 = cur + change * exp(-(dx * dx + dy * dy) / radius);
}