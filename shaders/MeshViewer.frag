#version 330

in float diffTerm;
uniform int wireMode;

void main() 
{
   if(wireMode == 1)    //Wireframe
       gl_FragColor = vec4(0, 0, 1, 1);
   else			//Fill + lighting
       gl_FragColor = diffTerm * vec4(0, 1, 1, 1);
}
