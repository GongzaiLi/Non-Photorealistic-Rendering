#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 mvMatrix;
uniform mat4 mvpMatrix;
uniform mat4 norMatrix;
uniform vec4 lightPos;
 
out float diffTerm;

void main()
{
	vec4 posnEye = mvMatrix * vec4(position, 1);
	vec4 normalEye = norMatrix * vec4(normal, 0);
        vec3 unitNormal = normalize(normalEye.xyz);
	vec3 lgtVec = normalize(lightPos.xyz - posnEye.xyz); 

	diffTerm = max(dot(lgtVec, unitNormal), 0.2);

	gl_Position = mvpMatrix * vec4(position, 1);
}