//  ========================================================================
//  COSC422: Advanced Computer Graphics;  University of Canterbury (2021)
//
//  FILE NAME: MeshViewer.cpp
//  Triangle mesh viewer using OpenMesh and OpenGL-4
//  This program assumes that the mesh consists of only triangles.
//  The model is scaled and translated to the origin to fit within the view frustum
//
//  Use arrow keys to rotate the model
//  Use 'w' key to toggle between wireframe and solid fill modes
//  ========================================================================

#define _USE_MATH_DEFINES // for C++  
#include <cmath>  
#include <iostream>
#include <fstream>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/freeglut.h>
#include "loadTGA.h" // upload tga image
using namespace std;
// todo list
// 1. pencil shading with mipmap
// 2. pencil shading


typedef OpenMesh::TriMesh_ArrayKernelT<> MyMesh;
MyMesh mesh;
float modelScale;
float xc, yc, zc;
float rotn_x = 0.0, rotn_y = 0.0;
GLuint vaoID;
GLuint mvpMatrixLoc, mvMatrixLoc, norMatrixLoc, lgtLoc, wireLoc;
glm::mat4 view, projView;
int num_Elems;
bool wireframe = false;

string shaderPath = "./src/shaders/";
string modelPath = "./src/models/";

GLuint creaseEdgeThresholdLoc, creaseSizeLoc, silhoutteSizeLoc;
float creaseEdgeThreshold = 20;
float zoomScale = 1.0;

// Edge size todo 
glm::vec2 creaseSize = glm::vec2(1.0, 1.0);
glm::vec2 silhoutteSize = glm::vec2(0.0, 3.0);

//Loads a shader file and returns the reference to a shader object
GLuint loadShader(GLenum shaderType, string filename)
{
	ifstream shaderFile(filename.c_str());
	if (!shaderFile.good()) cout << "Error opening shader file." << endl;
	stringstream shaderData;
	shaderData << shaderFile.rdbuf();
	shaderFile.close();
	string shaderStr = shaderData.str();
	const char* shaderTxt = shaderStr.c_str();

	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderTxt, NULL);
	glCompileShader(shader);
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
		GLchar* strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);
		const char* strShaderType = NULL;
		cerr << "Compile failure in shader: " << strInfoLog << endl;
		delete[] strInfoLog;
	}
	return shader;
}

// Gets the min max values along x, y, z for scaling and centering the model in the view frustum
void getBoundingBox(float& xmin, float& xmax, float& ymin, float& ymax, float& zmin, float& zmax)
{
	MyMesh::VertexIter vit = mesh.vertices_begin();
	MyMesh::Point pmin, pmax;

	pmin = pmax = mesh.point(*vit);

	//Iterate over the mesh using a vertex iterator
	for (vit = mesh.vertices_begin()+1; vit != mesh.vertices_end(); vit++)
	{
		pmin.minimize(mesh.point(*vit));
		pmax.maximize(mesh.point(*vit));
	}
	xmin = pmin[0];  ymin = pmin[1];  zmin = pmin[2];
	xmax = pmax[0];  ymax = pmax[1];  zmax = pmax[2];
}

//Initialisation function for OpenMesh, shaders and OpenGL
void initialize()
{
	// todo add load textures()

	float xmin, xmax, ymin, ymax, zmin, zmax;
	float CDR = M_PI / 180.0f;

	//============= Load mesh ==================
	if (!OpenMesh::IO::read_mesh(mesh, modelPath + "Dolphin.obj")) //Dolphin.obj  Camel.off
	{
		cerr << "Mesh file read error.\n";
	}
	getBoundingBox(xmin, xmax, ymin, ymax, zmin, zmax);

	xc = (xmin + xmax)*0.5f;
	yc = (ymin + ymax)*0.5f;
	zc = (zmin + zmax)*0.5f;
	OpenMesh::Vec3f box = { xmax - xc,  ymax - yc, zmax - zc };
	modelScale = 1.0 / box.max();

	//============= Load shaders ==================
	GLuint shaderv = loadShader(GL_VERTEX_SHADER, shaderPath + "MeshViewer.vert");
	GLuint shaderg = loadShader(GL_GEOMETRY_SHADER, shaderPath + "MeshViewer.geom");
	GLuint shaderf = loadShader(GL_FRAGMENT_SHADER, shaderPath + "MeshViewer.frag");


	GLuint program = glCreateProgram();
	glAttachShader(program, shaderv);
	glAttachShader(program, shaderg);
	glAttachShader(program, shaderf);

	glLinkProgram(program);

	//============= Create program object ============
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
		GLchar* strInfoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
		fprintf(stderr, "Linker failure: %s\n", strInfoLog);
		delete[] strInfoLog;
	}
	glUseProgram(program);

	//==============Get vertex, normal data from mesh=========
	int num_verts = mesh.n_vertices();
	int num_faces = mesh.n_faces();
	float* vertPos = new float[num_verts * 3];
	float* vertNorm = new float[num_verts * 3];
	//num_Elems = num_faces * 3;
	num_Elems = num_faces * 6; // 3 half edge + 3 counter-clockwise helf edge
	short* elems = new short[num_Elems];   //Asumption: Triangle mesh

	if (!mesh.has_vertex_normals())
	{
		mesh.request_face_normals();
		mesh.request_vertex_normals();
		mesh.update_normals();
	}

	MyMesh::VertexIter vit;  //A vertex iterator
	MyMesh::FaceIter fit;    //A face iterator
	MyMesh::FaceVertexIter fvit; //Face-vertex iterator
	OpenMesh::VertexHandle verH1, verH2; // verH2 using Half edge handle vertex
	OpenMesh::FaceHandle facH;
	MyMesh::Point pos;
	MyMesh::Normal norm;
	int indx;

	// Use a vertex iterator to get vertex positions and vertex normal vectors
	indx = 0;
	for (vit = mesh.vertices_begin(); vit != mesh.vertices_end(); vit++)
	{
		verH1 = *vit;				//Vertex handle
		pos = mesh.point(verH1);
		norm = mesh.normal(verH1);
		for (int j = 0; j < 3; j++)
		{
			vertPos[indx] = pos[j];
			vertNorm[indx] = norm[j];
			indx++;
		}
	}
	/*
	//Use a face iterator to get the vertex indices for each face 
	// Computing the element array for triangles:
	indx = 0;
	for (fit = mesh.faces_begin(); fit != mesh.faces_end(); fit++)
	{
		facH = *fit;
		for (fvit = mesh.fv_iter(facH); fvit.is_valid(); fvit++)
		{
			verH2 = *fvit;				 //Vertex handle
			elems[indx] = verH2.idx();
			indx++;
		}
	}
	*/

	// Book Page 31 ---- Doing Half edge handle
	OpenMesh::HalfedgeHandle heH, oheH;
	MyMesh::FaceHalfedgeIter fhit;

	indx = 0; //Vertex element index
	for (fit = mesh.faces_begin(); fit != mesh.faces_end(); fit++)
	{
		facH = *fit; //Face handle
		for (fhit = mesh.fh_iter(facH); fhit.is_valid(); fhit++)
		{
			heH = *fhit; //Halfedge handle
			oheH = mesh.opposite_halfedge_handle(heH);
			verH2 = mesh.from_vertex_handle(heH);
			elems[indx] = verH2.idx();
			if (!mesh.is_boundary(oheH)) //Interior edge
			{
				verH2 = mesh.opposite_vh(oheH);
				elems[indx + 1] = verH2.idx();
			}
			else { //Boundary edge
				elems[indx + 1] = elems[indx]; //Repeated vertex
			}
			indx += 2;

		}
	}

	/*
	The triangle adjacency primitive finds applications in non-photorealistic rendering of a mesh model, 
	where the prominent edges of the model are highlighted and a special type of shading model or texture 
	mapping applied to create an artistic/expressive style in the way the model is displayed. 
	The implementation of a non-photorealistic rendering algorithm using the geometry shader is discussed in detail in Chap. 4.
	*/
 
	mesh.release_vertex_normals();

	//============== Load buffer data ==============
	glGenVertexArrays(1, &vaoID);
	glBindVertexArray(vaoID);

	GLuint vboID[3];
	glGenBuffers(3, vboID);

	glBindBuffer(GL_ARRAY_BUFFER, vboID[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)* num_verts * 3, vertPos, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);  // Vertex position

	glBindBuffer(GL_ARRAY_BUFFER, vboID[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_verts * 3, vertNorm, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);  // Vertex normal

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboID[2]);
	// glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(short) * num_faces * 3, elems, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(short) * num_faces * 6, elems, GL_STATIC_DRAW);

	glBindVertexArray(0);

	//============== Create uniform variables ==============
	mvpMatrixLoc = glGetUniformLocation(program, "mvpMatrix");
	mvMatrixLoc = glGetUniformLocation(program, "mvMatrix");
	norMatrixLoc = glGetUniformLocation(program, "norMatrix");
	wireLoc = glGetUniformLocation(program, "wireMode");
	lgtLoc = glGetUniformLocation(program, "lightPos");
	creaseEdgeThresholdLoc = glGetUniformLocation(program, "creaseEdgeThreshold");
	silhoutteSizeLoc = glGetUniformLocation(program, "silhoutteSize");
	creaseSizeLoc = glGetUniformLocation(program, "creaseSize");


	glm::vec4 light = glm::vec4(5.0, 5.0, 10.0, 1.0);
	glm::mat4 proj;
	proj = glm::perspective(60.0f * CDR, 1.0f, 2.0f, 10.0f);  //perspective projection matrix
	view = glm::lookAt(glm::vec3(0, 0, 4.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0)); //view matrix
	projView = proj * view;
	glm::vec4 lightEye = view * light;
	glUniform4fv(lgtLoc, 1, &lightEye[0]);

	//============== Initialize OpenGL state ==============
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);   
}

void zoomInOrOut(int distance)
{
	if ((distance < 0 && zoomScale >= 1.0) || (distance > 0 && zoomScale < 10.0)) {
		zoomScale += 0.1 * distance;
	}

}

//Callback function for special keyboard events
void special(int key, int x, int y)
{
	switch (key)
	{
		case GLUT_KEY_LEFT:
			rotn_y -= 5.0;
			break;
		case GLUT_KEY_RIGHT:
			rotn_y += 5.0;
			break;
		case GLUT_KEY_UP:
			rotn_x -= 5.0;
			break;
		case GLUT_KEY_DOWN:
			rotn_x += 5.0;
			break;
		case GLUT_KEY_PAGE_DOWN:
			zoomInOrOut(-1);
			break;
		case GLUT_KEY_PAGE_UP:
			zoomInOrOut(1);
			break;
	}
	
	glutPostRedisplay();
}

void silhouetteThinckness(float step)
{
	// todo update size bounder
	//silhoutteSize[0] -= 0.1 * step;
	silhoutteSize[1] += 0.1 * step;

	/*
	if (silhoutteSize[1] >= 26.0) {
		silhoutteSize[1] = 26.0;
	}

	if (silhoutteSize[1] <= 4.0) {
		silhoutteSize[1] = 4.0;
	}
	cout << "1" << silhoutteSize[0] << endl;
	cout << "2" << silhoutteSize.y << endl;
	*/
}

void creaseThinckness(float step)
{
	// todo update size bounder
	creaseSize[0] += 0.1 * step;
	creaseSize[1] += 0.1 * step;
}

//Callback function for keyboard events
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
		case '1':
			wireframe = !wireframe;
			break;
		case 'q': //  i thickness of silhouette
			silhouetteThinckness(1.0);
			break;
		case 'a': // d thickness of silhouette
			silhouetteThinckness(-1.0);
			break;
		case 'w': // i thickness of crease 
			creaseThinckness(1.0);
			break;
		case 's': // d thickness of crease 
			creaseThinckness(-1.0);
			break;

	}


	if(wireframe) 	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else 			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glutPostRedisplay();
}

//The main display callback function
void display()
{
	float CDR = M_PI / 180.0;
	glm::mat4 matrix = glm::mat4(1.0);
	matrix = glm::rotate(matrix, rotn_x * CDR, glm::vec3(1.0, 0.0, 0.0));  //rotation about x
	matrix = glm::rotate(matrix, rotn_y * CDR, glm::vec3(0.0, 1.0, 0.0));  //rotation about y
	matrix = glm::scale(matrix, glm::vec3(modelScale, modelScale, modelScale) * zoomScale);
	matrix = glm::translate(matrix, glm::vec3(-xc, -yc, -zc));

	glm::mat4 viewMatrix = view * matrix;		//The model-view matrix
	glUniformMatrix4fv(mvMatrixLoc, 1, GL_FALSE, &viewMatrix[0][0]);

	glm::mat4 prodMatrix = projView * matrix;   //The model-view-projection matrix
	glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, &prodMatrix[0][0]);

	glm::mat4 invMatrix = glm::inverse(viewMatrix);  //Inverse of model-view matrix
	glUniformMatrix4fv(norMatrixLoc, 1, GL_TRUE, &invMatrix[0][0]);

	glUniform1f(creaseEdgeThresholdLoc, creaseEdgeThreshold);
	
	glUniform2fv(silhoutteSizeLoc, 1, &silhoutteSize[0]); // creaseSize silhoutteSize
	glUniform2fv(creaseSizeLoc, 1, &creaseSize[0]);

	if (wireframe) glUniform1i(wireLoc, 1);
	else		   glUniform1i(wireLoc, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(vaoID);
	//glDrawElements(GL_TRIANGLES, num_Elems, GL_UNSIGNED_SHORT, NULL);
	glDrawElements(GL_TRIANGLES_ADJACENCY, num_Elems, GL_UNSIGNED_SHORT, NULL);

	glFlush();
}


int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB |GLUT_DEPTH);
	glutInitWindowSize(600, 600);
	glutInitWindowPosition(10, 10);
	glutCreateWindow("Mesh Viewer (OpenMesh)-gli65");
	glutInitContextVersion(4, 2);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	if (glewInit() == GLEW_OK)
	{
		cout << "GLEW initialization successful! " << endl;
		cout << " Using GLEW version " << glewGetString(GLEW_VERSION) << endl;
	}
	else
	{
		cerr << "Unable to initialize GLEW  ...exiting." << endl;
		exit(EXIT_FAILURE);
	}

	initialize();
	glutDisplayFunc(display);
	glutSpecialFunc(special);
	glutKeyboardFunc(keyboard);
	glutMainLoop();
	return 0;
}

