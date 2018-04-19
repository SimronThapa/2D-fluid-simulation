# version 330 core
in vec2 TexCoords;

uniform float deltaT;
uniform sampler2D inputTexture;
uniform sampler2D velocity;

layout(location = 3) out vec4 velocity1;

void main() {
	vec2 u = texture(velocity, TexCoords).xy;
	vec2 pastCoord = fract(TexCoords - (0.5 * deltaT * u)); // fract - compute fractional part
	velocity1 = texture(inputTexture, pastCoord);

	/*vec2 px1 = vec2(1, 800.0 / 600 / 0);

	velocity1 = texture(inputTexture, TexCoords - texture2D(velocity, TexCoords).xy * deltaT * px1) * 1.0;*/
}