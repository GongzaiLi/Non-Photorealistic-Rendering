#version 330


uniform int wireMode;

in vec3 vecNormal;
in vec3 lgtVec;
in vec4 halfVec;
flat in int isDrewEdge;

out vec4 outColor;

void threeTone()
{
    float diffTerm = max(dot(lgtVec, vecNormal), 0.2);// 0.2 not sure
    float specTerm = max(dot(halfVec, vec4(vecNormal, 0)), 0);
    if (specTerm > 0.98) outColor = vec4(1.0);
    else if (diffTerm < 0.0)
        outColor = vec4(0.2, 0.2, 0.2, 1.0);
    else if (diffTerm < 0.6)
        outColor = vec4(0.0, 0.5, 0.5, 1.0);
    else outColor = vec4(0.0, 1.0, 1.0, 1.0);
}

void main() 
{
   if(wireMode == 1) {    //Wireframe
       outColor = vec4(0, 0, 1, 1);
   } else {			//Fill + lighting
       //outColor = diffTerm * vec4(0, 1, 1, 1);
       if (isDrewEdge == 1) {
            outColor = vec4(0.0);
       } else {
            threeTone();
       }
    }
}
