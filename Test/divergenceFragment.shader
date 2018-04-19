# version 330 core
in vec2 TexCoords;

uniform float deltaT;		// Time between steps
uniform float rho;			// Density
uniform float epsilonX;		// Distance between grid units
uniform float epsilonY;		// Distance between grid units
uniform sampler2D velocity;	// Advected velocity field, u_a

layout(location = 4) out vec4 divergence;

vec2 u(vec2 coord) {
	return texture2D(velocity, fract(coord)).xy;
}

void main() {
	float cx = -2.0 * epsilonX * rho / deltaT;
	float cy = -2.0 * epsilonY * rho / deltaT;

	float x1 = u(TexCoords + vec2(epsilonX, 0)).x;
	float x0 = u(TexCoords - vec2(epsilonX, 0)).x;
	float y1 = u(TexCoords + vec2(0, epsilonY)).y;
	float y0 = u(TexCoords - vec2(0, epsilonY)).y;
	float div = cx * (x1 - x0) + cy * (y1 - y0);

	divergence = vec4(div, 0.0, 0.0, 1.0);

	/*float x0 = texture(velocity, TexCoords - vec2(epsilonX, 0)).x;
	float x1 = texture(velocity, TexCoords + vec2(epsilonX, 0)).x;
	float y0 = texture(velocity, TexCoords - vec2(0, epsilonY)).y;
	float y1 = texture(velocity, TexCoords + vec2(0, epsilonY)).y;

	float div = cx * (x1 - x0) + cy * (y1 - y0);
	divergence = vec4(div, 0, 0, 1.0);*/
}