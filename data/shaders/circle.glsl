#version 120
precision highp float;

uniform mat4 Proj;
uniform mat4 View;

varying vec2 Position;
varying vec2 Center;
varying float Radius;

//#vertex
attribute vec2 in_Position;
attribute vec2 in_Center;
attribute float in_Radius;

void main() {
    gl_Position = Proj * View * vec4(in_Position, 0.0, 1.0);

    Position = in_Position;
	Center = in_Center;
	Radius = in_Radius;
}

//#fragment
void main() {
    const float width = 3.0;
    const vec3 Color = vec3(1.0, 1.0, 0.4);
	float dist = distance(Center, Position);
    float alpha = smoothstep(Radius, Radius - 1.5, dist);
	gl_FragColor = vec4(Color.rgb, alpha);
}

// void main() {
// 	const float width = 3.0;
// 	float inner = Radius - width;
// 	float middle = Radius - 0.5 * width;
// 	float dist = distance(Center, Position);
// 	float alpha = (Fill == 1.0 || dist > middle) ?
// 		smoothstep(Radius, middle, dist) :
// 		smoothstep(inner, middle, dist);
// 	gl_FragColor = vec4(Color.rgb, alpha);
// }