# version 330 core
in vec2 TexCoords;

out vec4 FragColor;

void main() {
	float x = 2.0 * TexCoords.x - 1.0;
	float y = 2.0 * TexCoords.y - 1.0;
	float temp = step(1.0, mod(floor((x + 1.0) / 0.2) + floor((y + 1.0) / 0.2), 2.0));
	FragColor = vec4(sin(2.0 * 3.1415 * y), sin(2.0 * 3.1415 * x), 0.0, 0.0);
	//FragColor = vec4(1.0, 0.0, 0.0, 0.0);
}