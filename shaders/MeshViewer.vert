#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 norMatrix;
 
out vec3 unitNormal;

void main()
{
	vec4 normalEye = norMatrix * vec4(normal, 0);
    unitNormal = normalize(normalEye.xyz);
	
	gl_Position = vec4(position, 1);
}