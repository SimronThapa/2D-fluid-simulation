# version 330 core
in vec2 TexCoords;
in vec2 pos;

uniform float deltaT;
uniform sampler2D inputTexture;
uniform sampler2D velocity;

out vec4 FragColor;

void main() {
	vec2 u = texture2D(velocity, TexCoords).xy;
	vec2 pastCoord = fract(TexCoords - (deltaT * u)); // fract - compute fractional part
	FragColor = texture2D(inputTexture, pastCoord);

	//vec2 px1 = vec2(1, 800/600);

	//vec2 FragPos = pos - deltaT * (texture(velocity, pos).xy);
	//FragColor = texture2D(inputTexture, TexCoords - texture2D(velocity, TexCoords).xy * deltaT * px1) * 1.0;
	//velocity0 = texture2D(inputTexture, FragPos);
}