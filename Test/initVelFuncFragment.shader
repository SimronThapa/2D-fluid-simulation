# version 330 core
in vec2 TexCoords;

layout(location = 2) out vec4 velocity0;

void main() {
	float x = 2.0 * TexCoords.x - 1.0;
	float y = 2.0 * TexCoords.y - 1.0;
	float temp = step(1.0, mod(floor((x + 1.0) / 0.2) + floor((y + 1.0) / 0.2), 2.0));
	//velocity0 = vec4(sin(2.0 * 3.1415 * y), sin(2.0 * 3.1415 * x), 0.0, 0.0);
	//velocity0 = vec4(1, sin(2 * 3.142 * y), 0, 0);
	velocity0 = vec4(1, 0, 0, 0);
}