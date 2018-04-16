# version 330 core
in vec2 TexCoords;

uniform float deltaT;		// Time between steps
uniform float rho;			// Density
uniform float epsilonX;		// Distance between grid units
uniform float epsilonY;		// Distance between grid units
uniform sampler2D velocity;	// Advected velocity field, u_a

out vec4 divergence;

vec2 u(vec2 coord) {
	return texture2D(velocity, fract(coord)).xy;
}

void main() {
	float cx = -2.0 * epsilonX * rho / deltaT;
	float cy = -2.0 * epsilonY * rho / deltaT;

	divergence = vec4(( cx * (u(TexCoords + vec2(epsilonX, 0)).x - u(TexCoords - vec2(epsilonX, 0)).x) +
		cy * (u(TexCoords + vec2(0, epsilonY)).y - u(TexCoords - vec2(0, epsilonY)).y) ), 0.0, 0.0, 1.0);
}