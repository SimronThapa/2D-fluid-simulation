# version 330 core
in vec2 TexCoords;

uniform float epsilonX;			// Distance between grid units
uniform float epsilonY;			// Distance between grid units
uniform sampler2D divergence;	// Divergence field of advected velocity, d
uniform sampler2D pressure;		// Pressure field from previous iteration, p^(k-1)

layout(location = 6) out vec4 pressure1;

float d(vec2 coord) {
	return texture2D(divergence, fract(coord)).x;
}

float p(vec2 coord) {
	return texture2D(pressure, fract(coord)).x;
}

void main() {
	float x0 = p(TexCoords + vec2(2.0 * epsilonX, 0.0));
	float x1 = p(TexCoords - vec2(2.0 * epsilonX, 0.0));
	float y0 = p(TexCoords + vec2(0.0, 2.0 * epsilonY));
	float y1 = p(TexCoords - vec2(0.0, 2.0 * epsilonY));
	pressure1 = vec4(0.25 * (d(TexCoords) + x0 + x1 + y0 + y1), 0.0, 0.0, 1.0);
}