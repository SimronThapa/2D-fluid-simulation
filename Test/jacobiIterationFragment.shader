# version 330 core
in vec2 TexCoords;

uniform float epsilonX;		// Distance between grid units
uniform float epsilonY;		// Distance between grid units
uniform sampler2D divergence;	// Divergence field of advected velocity, d
uniform sampler2D pressure;		// Pressure field from previous iteration, p^(k-1)

out vec4 pressure0;

float d(vec2 coord) {
	return texture2D(divergence, fract(coord)).x;
}

float p(vec2 coord) {
	return texture2D(pressure, fract(coord)).x;
}

void main() {
	pressure0 = vec4(0.25 * (d(TexCoords)
		+ p(TexCoords + vec2(2.0 * epsilonX, 0.0)) + p(TexCoords - vec2(2.0 * epsilonX, 0.0))
		+ p(TexCoords + vec2(0.0, 2.0 * epsilonY)) + p(TexCoords - vec2(0.0, 2.0 * epsilonY))),
		0.0, 0.0, 1.0);
}