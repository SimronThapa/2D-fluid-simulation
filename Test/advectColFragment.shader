# version 330 core
in vec2 TexCoords;

uniform float deltaT;
uniform sampler2D inputTexture;
uniform sampler2D velocity;

layout(location = 1) out vec4 color1;

void main() {
	vec2 u = texture(velocity, TexCoords).xy;
	vec2 pastCoord = fract(TexCoords - (0.5 * deltaT * u)); // fract - compute fractional part
	color1 = texture(inputTexture, pastCoord);
}