#version 400

layout (triangles_adjacency) in;
layout (triangle_strip , max_vertices = 27) out;

// uniform
uniform mat4 mvMatrix;
uniform mat4 mvpMatrix; // prohMatrix
uniform mat4 norMatrix;
uniform vec4 lightPos;

uniform float creaseEdgeThreshold;
uniform float edgeMinimizeGap;

uniform vec2 silhoutteSize;
uniform vec2 creaseSize;
uniform int drawSilhoutte;
uniform int drawCrease;

// in
in vec3 unitNormal[];

// out
out vec3 vecNormal;
out vec3 lgtVec;
out vec4 halfVec;
out vec2 TexCoord;

flat out int isDrewEdge;

// global variable
vec4 normMain;
float PI = 3.14159265;
float T;

void lightingCalculation(int index)
{
	vec4 posnEye = mvMatrix *  gl_in[index].gl_Position;
	lgtVec = normalize(lightPos.xyz - posnEye.xyz);
	vecNormal = unitNormal[index];
	
	vec4 viewVec = normalize(vec4(-posnEye.xyz, 0));
	halfVec = normalize(vec4(lgtVec.xyz, 0) + viewVec);
}

vec4 calculateNormal(int indexA, int indexB, int indexC){

	vec3 u = gl_in[indexA].gl_Position.xyz - gl_in[indexC].gl_Position.xyz;
	vec3 v = gl_in[indexB].gl_Position.xyz - gl_in[indexC].gl_Position.xyz;

    return vec4(normalize(cross(u, v)), 0);
}

void drawSilhouetteEdge(vec4 posnA, vec4 posnB, vec4 n1, vec4 n2) 
{
	vec4 normal = normalize(n1 + n2);

	vec4 p1, p2, q1, q2;
	float d1 = silhoutteSize[0], d2 = silhoutteSize[1];


	vec4 a_to_b = edgeMinimizeGap * normalize(posnB - posnA); 
    vec4 b_to_a = edgeMinimizeGap * normalize(posnA - posnB);
    p1 = (posnA + b_to_a) + d1 * normal;
    p2 = (posnA + b_to_a) + d2 * normal;
    q1 = (posnB + a_to_b) + d1 * normal;
    q2 = (posnB + a_to_b) + d2 * normal;

	/*
	p1 = posnA + d1 * normal;
    p2 = posnA + d2 * normal;
    q1 = posnB + d1 * normal;
    q2 = posnB + d2 * normal;
	*/
	isDrewEdge = 1;
	
	gl_Position = mvpMatrix * p1;
	EmitVertex();

	gl_Position = mvpMatrix * p2;
	EmitVertex();

	gl_Position = mvpMatrix * q1;
	EmitVertex();

	gl_Position = mvpMatrix * q2;
	EmitVertex();

	EndPrimitive();

}

void drawCreaseEdge(vec4 posnA, vec4 posnB, vec4 n1, vec4 n2) {
	vec4 u = normalize(posnB - posnA);
    vec4 v = normalize(n1 + n2);
	vec4 w = vec4(normalize(cross(vec3(u.xyz), vec3(v.xyz))), 0);

	vec4 p1, p2, q1, q2;

	vec4 a_to_b = edgeMinimizeGap * normalize(posnB - posnA); 
    vec4 b_to_a = edgeMinimizeGap * normalize(posnA - posnB);

	float d1 = creaseSize[0], d2 = creaseSize[1];
    p1 = (posnA + b_to_a) + d1 * v + d2 * w;
    p2 = (posnA + b_to_a) + d1 * v - d2 * w;
    q1 = (posnB + a_to_b) + d1 * v + d2 * w;
    q2 = (posnB + a_to_b) + d1 * v - d2 * w;

    isDrewEdge = 1;

    gl_Position = mvpMatrix * p1;
    EmitVertex();
    
	gl_Position = mvpMatrix * p2;
    EmitVertex();
    
	gl_Position = mvpMatrix * q1;
    EmitVertex();
    
	gl_Position = mvpMatrix * q2;
    EmitVertex();
    EndPrimitive();

}

void drawEdge(int index)
{
	vec4 normAdj = calculateNormal(index, (index + 1) % 6, (index + 2) % 6);
	
	if (drawSilhoutte == 1) {
		if ((mvMatrix * normMain).z > 0 && (mvMatrix * normAdj).z < 0) { //mvMatrix
			drawSilhouetteEdge(gl_in[index].gl_Position, gl_in[(index + 2) % 6].gl_Position, normMain, normAdj);
		}
	}
	

	if (drawCrease == 1) {
		T = cos(creaseEdgeThreshold * PI / 180.0);
		if (dot(normMain, normAdj) < T) {
			drawCreaseEdge(gl_in[index].gl_Position, gl_in[(index + 2) % 6].gl_Position, normMain, normAdj);
		}
	}
	

}

void getTexCoord(int index)
{
	if (index == 0) {
		TexCoord.s = 0.0;
		TexCoord.t = 1.0;
	} else if (index == 2) {
		TexCoord.s = 0.0;
		TexCoord.t = 0.0;
	} else if (index == 4) {
		TexCoord.s = 1.0;
		TexCoord.t = 0.5;
	}
	
}


void main()
{
	normMain = calculateNormal(0, 2, 4);
	
	for(int i = 0; i < gl_in.length(); i++)
    {    
        if (i == 0 || i == 2 || i == 4) 
		{
			lightingCalculation(i);

			getTexCoord(i);
			isDrewEdge = 0;
            gl_Position = mvpMatrix * gl_in[i].gl_Position;
			EmitVertex();
        }
    }
    EndPrimitive();

	
	
	for(int i = 0; i < gl_in.length(); i++)
    {
		if (i == 0 || i == 2 || i == 4) {
			drawEdge(i);
		}
    }
	
	
}