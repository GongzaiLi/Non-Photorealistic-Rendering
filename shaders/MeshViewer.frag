#version 330

uniform sampler2D textureSimple[3];
uniform int wireMode;
uniform int toggleRenderStyle;
uniform int multiTexturingModel;

in vec3 vecNormal;
in vec3 lgtVec;
in vec4 halfVec;
in vec2 TexCoord;

flat in int isDrewEdge;

out vec4 outColor;

void threeTone(float diffTerm, float specTerm)
{
    if (specTerm > 0.98) outColor = vec4(1.0);
    else if (diffTerm < 0.0)
        outColor = vec4(0.2, 0.2, 0.2, 1.0);
    else if (diffTerm < 0.6)
        outColor = vec4(0.0, 0.5, 0.5, 1.0);
    else outColor = vec4(0.0, 1.0, 1.0, 1.0);
}

void multiTexturing(float diffTerm)
{
    if (diffTerm >= 0.4) {
        outColor = mix(texture(textureSimple[1], TexCoord), texture(textureSimple[0], TexCoord),  (diffTerm - 0.4) / 0.4);
    } else if (diffTerm <= 0.2) {
        outColor = mix(texture(textureSimple[2], TexCoord), texture(textureSimple[1], TexCoord),  (diffTerm - 0.2) / 0.2);
    } else {
        outColor = texture(textureSimple[1], TexCoord);
    }
}

void pencilShading(float diffTerm, float specTerm)
{
    if (specTerm > 0.98) {
        outColor = vec4(1.0);
    } else if (diffTerm >= 0.6) { // 0.8 to 1.0
        outColor = texture(textureSimple[0], TexCoord);
    } else if (diffTerm <= 0.0) {
        outColor = texture(textureSimple[2], TexCoord);
    } else {
        if (multiTexturingModel == 1) {
            multiTexturing(diffTerm);
        } else {
            outColor = texture(textureSimple[1], TexCoord);
        }
    }
}

void showRender() {

    float diffTerm = max(dot(lgtVec, vecNormal), 0.2);// 0.2 n.l
    float specTerm = max(dot(halfVec, vec4(vecNormal, 0)), 0);

    if (toggleRenderStyle == 1) {
        threeTone(diffTerm, specTerm);
    } else {
        pencilShading(diffTerm, specTerm);
    }
    
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
            showRender();
       }
    }
}
