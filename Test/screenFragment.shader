# version 330 core
in vec2 TexCoords;

uniform sampler2D screenTexture;

out vec4 FragColor;
//in vec2 pos;
//uniform vec2 mousePos;

void main() {
	vec3 col = vec3(0.5f);
	vec2 pos = vec2(0.1f, 0.1f);
	vec2 mousePos = vec2(0.3f, 0.3f);
	if (length(mousePos - pos) < 0.75f) {
		vec2 delta = 0.1f * normalize(mousePos - pos);
		col.r *= delta.x;
		col.g *= delta.x;
		col.b *= delta.x;
	}
	FragColor = vec4(col, 0.0);
	FragColor = texture(screenTexture, TexCoords);
}