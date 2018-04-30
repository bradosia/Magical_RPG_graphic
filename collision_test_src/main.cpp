#define M_PI           3.14159265358979323846

#include <math.h>
#include <iostream>
#include <fstream>      // std::ofstream

//to map image filenames to textureIds
#include <string.h>
#include <map>
#include <chrono>

/* GLEW
 * The OpenGL Extension Wrangler Library
 */
#define GLEW_STATIC
#include <GL/glew.h>
/* GLFW 3.2.1
 * Open Source, multi-platform library for OpenGL
 */
#include <GLFW/glfw3.h>
//#include <SDL2/SDL_mixer.h>
/* ReactPhysics3D
 * Open-source physics engine
 */
#include <reactphysics3d/reactphysics3d.h>
#include <soil/soil.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "cMesh.h"

// Shader sources
const GLchar* vertexSource =
		R"glsl(
    #version 150 core
    in vec3 position;
    in vec3 color;
    in vec2 texcoord;
    out vec3 Color;
    out vec2 Texcoord;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 proj;
    uniform vec3 overrideColor;
    void main()
    {
        Color = overrideColor * color;
        Texcoord = texcoord;
        gl_Position = proj * view * model * vec4(position, 1.0);
    }
)glsl";
const GLchar* fragmentSource =
		R"glsl(
    #version 150 core
    in vec3 Color;
    in vec2 Texcoord;
    out vec4 outColor;
    uniform sampler2D texKitten;
    uniform sampler2D texPuppy;
    void main()
    {
        outColor = vec4(Color, 1.0) * mix(texture(texKitten, Texcoord), texture(texPuppy, Texcoord), 0.5);
    }
)glsl";

class WorldCollisionCallback: public rp3d::CollisionCallback {
public:

	bool boxCollideWithSphere1;
	bool boxCollideWithCylinder;
	bool sphere1CollideWithCylinder;
	bool sphere1CollideWithSphere2;

	rp3d::CollisionBody* boxBody;
	rp3d::CollisionBody* sphere1Body;
	rp3d::CollisionBody* sphere2Body;
	rp3d::CollisionBody* cylinderBody;
	rp3d::CollisionBody* convexBody;

	WorldCollisionCallback() {
	}

	// This method will be called for contact
	virtual void notifyContact(const rp3d::ContactPointInfo& contactPointInfo) {
		std::cout << "shape1->getBody()->getID()="
				<< contactPointInfo.shape1->getBody()->getID()
				<< " shape2->getBody()->getID()="
				<< contactPointInfo.shape2->getBody()->getID() << std::endl;
	}

};

static void glfw_error_callback(int error, const char* description) {
	std::cout << "Error " << error << ": " << description << std::endl;
}

void camera() {
	/*double cx = 0;
	 double cy = 0;
	 double cz = 0;
	 double angley = 0;
	 double anglex = 0;
	 double r = 0.1;
	 // move camera a distance r away from the center
	 glTranslatef(0, 0, -r);

	 // rotate
	 glRotatef(angley, 0, 1, 0);
	 glRotatef(anglex, 1, 0, 0);

	 // move to center of circle
	 glTranslatef(-cx, -cy, -cz);*/

	glViewport(0, 0, 900, 600);
	//glScale(0.1, 0.1, 0.1);
}

static GLuint axes_list;

void axis() {
	/* Create a display list for drawing axes */
	axes_list = glGenLists(1);
	glNewList(axes_list, GL_COMPILE);

	glColor4ub(0, 0, 255, 255);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.75f, 0.25f, 0.0f);
	glVertex3f(0.75f, -0.25f, 0.0f);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.75f, 0.0f, 0.25f);
	glVertex3f(0.75f, 0.0f, -0.25f);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, 0.75f, 0.25f);
	glVertex3f(0.0f, 0.75f, -0.25f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.25f, 0.75f, 0.0f);
	glVertex3f(-0.25f, 0.75f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.25f, 0.0f, 0.75f);
	glVertex3f(-0.25f, 0.0f, 0.75f);
	glVertex3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 0.25f, 0.75f);
	glVertex3f(0.0f, -0.25f, 0.75f);
	glVertex3f(0.0f, 0.0f, 1.0f);
	glEnd();

	glColor4ub(255, 255, 0, 255);
	glRasterPos3f(1.1f, 0.0f, 0.0f);

	glEndList();
}

int main() {
// ---------- Atributes ---------- //

// Physics world
	rp3d::CollisionWorld* mWorld;

// Bodies
	rp3d::CollisionBody* mConeBody;
	rp3d::CollisionBody* mBoxBody;
	rp3d::CollisionBody* mSphere1Body;
	rp3d::CollisionBody* mSphere2Body;
	rp3d::CollisionBody* mCylinderBody;

// Collision shapes
	rp3d::BoxShape* mBoxShape;
	rp3d::ConeShape* mConeShape;
	rp3d::SphereShape* mSphereShape;
	rp3d::CylinderShape* mCylinderShape;

// Proxy shapes
	rp3d::ProxyShape* mBoxProxyShape;
	rp3d::ProxyShape* mSphere1ProxyShape;
	rp3d::ProxyShape* mSphere2ProxyShape;
	rp3d::ProxyShape* mCylinderProxyShape;

// Collision callback class
	WorldCollisionCallback mCollisionCallback;

// Create the world
	mWorld = new rp3d::CollisionWorld();

// Create the bodies
	mConeShape = new rp3d::ConeShape(2, 5); //radius, height
	rp3d::Transform coneTransform(rp3d::Vector3(12.5, 0, 0),
			rp3d::Quaternion(0, 0.5 * M_PI, 0));
	mConeBody = mWorld->createCollisionBody(coneTransform);
	mConeBody->addCollisionShape(mConeShape, rp3d::Transform::identity());
	std::cout << "mConeBody->getID()=" << mConeBody->getID() << std::endl;

	rp3d::Transform boxTransform(rp3d::Vector3(10, 0, 0),
			rp3d::Quaternion::identity());
	mBoxBody = mWorld->createCollisionBody(boxTransform);
	mBoxShape = new rp3d::BoxShape(rp3d::Vector3(3, 3, 3));
	mBoxProxyShape = mBoxBody->addCollisionShape(mBoxShape,
			rp3d::Transform::identity());

	mSphereShape = new rp3d::SphereShape(3.0);
	rp3d::Transform sphere1Transform(rp3d::Vector3(10, 5, 0),
			rp3d::Quaternion::identity());
	mSphere1Body = mWorld->createCollisionBody(sphere1Transform);
	mSphere1ProxyShape = mSphere1Body->addCollisionShape(mSphereShape,
			rp3d::Transform::identity());

	rp3d::Transform sphere2Transform(rp3d::Vector3(30, 10, 10),
			rp3d::Quaternion::identity());
	mSphere2Body = mWorld->createCollisionBody(sphere2Transform);
	mSphere2ProxyShape = mSphere2Body->addCollisionShape(mSphereShape,
			rp3d::Transform::identity());

	mCylinderShape = new rp3d::CylinderShape(1, 2); //radius, height
	rp3d::Transform cylinderTransform(rp3d::Vector3(-0.3, 0, 0),
			rp3d::Quaternion(0, 0 * M_PI, 0));
	mCylinderBody = mWorld->createCollisionBody(cylinderTransform);
	mCylinderProxyShape = mCylinderBody->addCollisionShape(mCylinderShape,
			rp3d::Transform::identity());
	std::cout << "mCylinderBody->getID()=" << mCylinderBody->getID()
			<< std::endl;

	mCollisionCallback.boxBody = mBoxBody;
	mCollisionCallback.sphere1Body = mSphere1Body;
	mCollisionCallback.sphere2Body = mSphere2Body;
	mCollisionCallback.cylinderBody = mCylinderBody;

	/* window */
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	GLFWwindow* window = glfwCreateWindow(900, 600, "hey",
	NULL,
	NULL);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	//GLEW initialization
	GLint GlewInitResult = glewInit();
	if (GLEW_OK != GlewInitResult) {
		printf("ERROR: %s\n", glewGetErrorString(GlewInitResult));
		exit(
		EXIT_FAILURE);
	}

	cMesh *mesh1 = new cMesh("cone.obj");
	cMeshEntry* meshEntry1 = mesh1->meshEntries.at(0);
	// Vertex and Indices array for the triangle mesh (data in shared and not copied)
	rp3d::TriangleVertexArray* vertexArray = new rp3d::TriangleVertexArray(
			meshEntry1->nbVertices, (void*) meshEntry1->vertices1D,
			sizeof(float), meshEntry1->nbFaces,
			(void*) meshEntry1->faceIndexes1D, sizeof(int),
			rp3d::TriangleVertexArray::VERTEX_FLOAT_TYPE,
			rp3d::TriangleVertexArray::INDEX_INTEGER_TYPE);

	/*for (unsigned int i = 0; i < meshEntry1->nbVertices * 3; i++) {
	 std::cout << "v1D: " << i << " " << meshEntry1->nbVertices << " "
	 << meshEntry1->vertices1D[i] << std::endl;
	 }
	 for (unsigned int i = 0; i < meshEntry1->nbVertices; i++) {
	 std::cout << "v2D: " << meshEntry1->vertices2D[i][0] << ","
	 << meshEntry1->vertices2D[i][1] << ","
	 << meshEntry1->vertices2D[i][2] << std::endl;
	 }
	 for (unsigned int i = 0; i < meshEntry1->nbFaces * 3; i++) {
	 std::cout << "f1D: " << meshEntry1->faceIndexes1D[i] << std::endl;
	 }*/
	// Create the collision shape for the rigid body (convex mesh shape) and
	// do not forget to delete it at the end
	rp3d::ConvexMeshShape* mConvexShape1 = new rp3d::ConvexMeshShape(
			vertexArray, true);
	rp3d::Transform mConvexShapeTransform1(rp3d::Vector3(1, 0, 0),
			rp3d::Quaternion(0.0 * M_PI, 0.0 * M_PI, 1 * M_PI));
	rp3d::CollisionBody* mConvexBody1 = mWorld->createCollisionBody(
			mConvexShapeTransform1);
	rp3d::ProxyShape* mConvexProxyShape1 = mConvexBody1->addCollisionShape(
			mConvexShape1, rp3d::Transform::identity());

	cMesh *mesh2 = new cMesh("cylinder.obj");
	cMeshEntry* meshEntry2 = mesh2->meshEntries.at(0);
	// Vertex and Indices array for the triangle mesh (data in shared and not copied)
	rp3d::TriangleVertexArray* vertexArray2 = new rp3d::TriangleVertexArray(
			meshEntry2->nbVertices, (void*) meshEntry2->vertices1D,
			sizeof(float), meshEntry2->nbFaces,
			(void*) meshEntry2->faceIndexes1D, sizeof(int),
			rp3d::TriangleVertexArray::VERTEX_FLOAT_TYPE,
			rp3d::TriangleVertexArray::INDEX_INTEGER_TYPE);
	rp3d::ConvexMeshShape* mConvexShape2 = new rp3d::ConvexMeshShape(
			vertexArray2, true);
	rp3d::Transform mConvexShapeTransform2(rp3d::Vector3(3.00, 0, 0),
			rp3d::Quaternion(0, 0, 0));
	rp3d::CollisionBody* mConvexBody2 = mWorld->createCollisionBody(
			mConvexShapeTransform2);
	rp3d::ProxyShape* mConvexProxyShape2 = mConvexBody2->addCollisionShape(
			mConvexShape2, rp3d::Transform::identity());

	mWorld->testCollision(&mCollisionCallback);

	axis();

	glm::vec3 vector1 = glm::vec3(0.0f, 1.0f, 0.0f);
	/* load an image file directly as a new OpenGL texture */
	GLuint tex_2d = SOIL_load_OGL_texture("SpiderTex.jpg", SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB
					| SOIL_FLAG_COMPRESS_TO_DXT);

	int imgW[10] = { 0 };
	int imgH[10] = { 0 };
	int imgCH[10] = { 0 };
	int imgWTemp, imgHTemp, imgCHTemp;
	unsigned char** imgHeightMap = new unsigned char*[10];
	unsigned char** imgMemblock = new unsigned char*[10];
	unsigned int fileBytes[10] = { 0 };
	unsigned int filePosition[10] = { 0 };
	std::vector<std::string> imgFiles;
	imgFiles.push_back("SpiderTex.jpg");
	imgFiles.push_back("Gordunni_Ogre.jpg");
	imgFiles.push_back("GoodShrekImage.png");
	imgFiles.push_back("Gordunni_Ogre.jpg");
	imgFiles.push_back("SpiderTex.jpg");
	unsigned int i, n, uintSize, fileBytesTotal, spacingBytes;
	spacingBytes = 10;
	uintSize = sizeof(unsigned char);
	n = imgFiles.size();
	for (i = 0; i < n; i++) {
		imgHeightMap[i] = SOIL_load_image(imgFiles.at(i).c_str(), &imgWTemp,
				&imgHTemp, &imgCHTemp,
				(imgFiles.at(i).find(".png") != std::string::npos) ?
						SOIL_LOAD_RGBA : SOIL_LOAD_RGB);
		imgW[i] = imgWTemp;
		imgH[i] = imgHTemp;
		imgCH[i] = imgCHTemp;
		fileBytes[i] = imgW[i] * imgH[i] * imgCH[i] * uintSize;
		fileBytesTotal += fileBytes[i];
		if (i > 0)
			filePosition[i] = filePosition[i - 1] + fileBytes[i - 1]
					+ spacingBytes;
		else
			filePosition[i] = 0;
	}

	std::ofstream OutFile;
	OutFile.open("assets.data", std::ios::out | std::ios::binary);
	for (i = 0; i < n; i++) {
		OutFile.write((char *) &imgHeightMap[i][0], fileBytes[i]);
		OutFile.write("          ", spacingBytes);
	}
	OutFile.close();

	std::ifstream InFile;
	InFile.open("assets.data", std::ios::in | std::ios::binary);
	if (InFile.is_open()) {
		for (i = 0; i < n; i++) {
			imgMemblock[i] = new unsigned char[fileBytes[i]];
			InFile.seekg(filePosition[i]);
			InFile.read((char *) &imgMemblock[i][0], fileBytes[i]);
		}
		InFile.close();
	}

	for (i = 0; i < n; i++) {
		std::string outputFile = "";
		outputFile += "output";
		outputFile += std::to_string(i);
		outputFile += ".bmp";
		SOIL_save_image(outputFile.c_str(), SOIL_SAVE_TYPE_BMP, imgW[i],
				imgH[i], imgCH[i], imgMemblock[i]);
	}

	// Create Vertex Array Object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create a Vertex Buffer Object and copy the vertex data to it
	GLuint vbo;
	glGenBuffers(1, &vbo);

	GLfloat vertices[] = { -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
			0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.5f, 0.5f, -0.5f,
			1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, -0.5f,
			-0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,

			-0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.5f,
			1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -0.5f,
			0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, -0.5f, -0.5f, 0.5f, 1.0f,
			1.0f, 1.0f, 0.0f, 0.0f,

			-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, -0.5f, 0.5f, -0.5f,
			1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f,
			0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
			-0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, -0.5f, 0.5f, 0.5f,
			1.0f, 1.0f, 1.0f, 1.0f, 0.0f,

			0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.5f, 0.5f, -0.5f,
			1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f,
			0.0f, 1.0f, 0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.5f,
			-0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f,
			1.0f, 1.0f, 1.0f, 0.0f,

			-0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.5f, -0.5f,
			-0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, -0.5f, 0.5f, 1.0f, 1.0f,
			1.0f, 1.0f, 0.0f, 0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
			-0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, -0.5f, -0.5f,
			-0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,

			-0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.5f, 0.5f, -0.5f,
			1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f,
			1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, -0.5f,
			0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, -0.5f, 0.5f, -0.5f, 1.0f,
			1.0f, 1.0f, 0.0f, 1.0f,

			-1.0f, -1.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, -1.0f,
			-0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 1.0f, 1.0f, 1.0f, 1.0f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f,
			-0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Create and compile the vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	// Create and compile the fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);

	// Link the vertex and fragment shader into a shader program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	// Specify the layout of the vertex data
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
			0);

	GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
	glEnableVertexAttribArray(colAttrib);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
			(void*) (3 * sizeof(GLfloat)));

	GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
			(void*) (6 * sizeof(GLfloat)));

	// Load textures
	GLuint textures[2];
	glGenTextures(2, textures);

	int width, height;
	unsigned char* image;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	image = SOIL_load_image("SpiderTex.jpg", &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
	GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	glUniform1i(glGetUniformLocation(shaderProgram, "texKitten"), 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	image = SOIL_load_image("SpiderTex.jpg", &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
	GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	glUniform1i(glGetUniformLocation(shaderProgram, "texPuppy"), 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLint uniModel = glGetUniformLocation(shaderProgram, "model");

	// Set up projection
	glm::mat4 view = glm::lookAt(glm::vec3(2.5f, 2.5f, 2.0f),
			glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	GLint uniView = glGetUniformLocation(shaderProgram, "view");
	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

	glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f,
			1.0f, 10.0f);
	GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

	GLint uniColor = glGetUniformLocation(shaderProgram, "overrideColor");

	auto t_start = std::chrono::high_resolution_clock::now();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		camera();
		glPushMatrix();
		glCallList(axes_list);
		glPopMatrix();

		// Clear the screen to black
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Calculate transformation
		auto t_now = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration_cast<std::chrono::duration<float>>(
				t_now - t_start).count();

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, time * glm::radians(180.0f),
				glm::vec3(0.0f, 0.0f, 1.0f));
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

		// Draw cube
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glEnable(GL_STENCIL_TEST);

		// Draw floor
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glStencilMask(0xFF);
		glDepthMask(GL_FALSE);
		glClear(GL_STENCIL_BUFFER_BIT);

		glDrawArrays(GL_TRIANGLES, 36, 6);

		// Draw cube reflection
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		glStencilMask(0x00);
		glDepthMask(GL_TRUE);

		model = glm::scale(glm::translate(model, glm::vec3(0, 0, -1)),
				glm::vec3(1, 1, -1));
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

		glUniform3f(uniColor, 0.3f, 0.3f, 0.3f);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glUniform3f(uniColor, 1.0f, 1.0f, 1.0f);

		glDisable(GL_STENCIL_TEST);

		mesh1->render();
		/*if (keys[1]) { // f1
		 keys[1] = false;
		 KillGLWindow();
		 fullscreen = !fullscreen;
		 if (!CreateGLWindow(windowTitle, 640, 480, 16, fullscreen,
		 window)) {
		 return 0;
		 }
		 }*/

		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;
}
