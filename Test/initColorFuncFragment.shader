# version 330 core
in vec2 TexCoords;

layout(location = 0) out vec4 color0;

void main() {
	float x = 2.0 * TexCoords.x - 1.0;
	float y = 2.0 * TexCoords.y - 1.0;
	float temp = step(1.0, mod(floor((x + 1.0) / 0.2) + floor((y + 1.0) / 0.2), 2.0));
	color0 = vec4(temp, temp, temp, 0.0);
}