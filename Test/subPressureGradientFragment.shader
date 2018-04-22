# version 330 core
in vec2 TexCoords;

uniform float deltaT;		// Time between steps
uniform float rho;			// Density
uniform float epsilonX;		// Distance between grid units
uniform float epsilonY;		// Distance between grid units
uniform sampler2D velocity;	// Advected velocity field, u_a
uniform sampler2D pressure;	// Solved pressure field

layout(location = 3) out vec4 velocity1;

float p(vec2 coord) {
	return texture2D(pressure, fract(coord)).x;
}

void main() {
	vec2 u_a = texture(velocity, TexCoords).xy;
	float diff_p_x = (p(TexCoords + vec2(epsilonX, 0.0)) - p(TexCoords - vec2(epsilonX, 0.0)));
	float u_x = u_a.x - deltaT / (2.0 * rho * epsilonX) * diff_p_x;
	float diff_p_y = (p(TexCoords + vec2(0.0, epsilonY)) - p(TexCoords - vec2(0.0, epsilonY)));
	float u_y = u_a.y - deltaT / (2.0 * rho * epsilonY) * diff_p_y;
	velocity1 = vec4(u_x, u_y, 0.0, 0.0);
}