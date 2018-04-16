# version 330 core
in vec2 TexCoords;

uniform vec4 change;
uniform vec2 center;
uniform float radius;
uniform sampler2D inputTex;

out vec4 velocity0;

void main() {
	float dx = center.x - TexCoords.x;
	float dy = center.y - TexCoords.y;
	vec4 cur = texture2D(inputTex, TexCoords);
	velocity0 = cur + change * exp(-(dx * dx + dy * dy) / radius);
}