# version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out vec2 pos;

void main() {
	TexCoords = aTexCoords;
	//pos = vec2(aPos.x, aPos.y);
	pos = (vec2(aPos) + vec2(1, 1)) / 2.0;
	gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}