/*
Práctica 6: Texturizado
*/
//para cargar imagen
#define STB_IMAGE_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <cmath>
#include <vector>
#include <math.h>

#include <glew.h>
#include <glfw3.h>

#include <glm.hpp>
#include <gtc\matrix_transform.hpp>
#include <gtc\type_ptr.hpp>

#include "Window.h"
#include "Mesh_tn.h"
#include "Shader_m.h"
#include "Camera.h"
#include "Texture.h"
#include "Sphere.h"
#include "Model.h"
#include "Skybox.h"

const float toRadians = 3.14159265f / 180.0f;

Window mainWindow;
std::vector<Mesh*> meshList; //solo recibe xyz
std::vector<MeshColor*> meshListColor; // recibe xyz rgb
std::vector<MeshModel*> meshListModel; // recibe xyz uv nx ny nz

std::vector<Shader> shaderList;

Camera camera;

Texture plainTexture;
Texture pisoTexture;
Texture dadoTexture;
Texture holocronTexture;

Model Kitt_M;
Model Llanta_M;
Model Dado_M;
Model Holocron_M;
Model Avion_M; // --- AGREGADO PARA EL EJERCICIO 3 ---

Skybox skybox;

GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;
static double limitFPS = 1.0 / 60.0;

// Vertex Shader
static const char* vShader = "shaders/shader_texture.vert";

// Fragment Shader
static const char* fShader = "shaders/shader_texture.frag";

void CreateObjects()
{
	unsigned int indices[] = {
		0, 3, 1,
		1, 3, 2,
		2, 3, 0,
		0, 1, 2
	};

	GLfloat vertices[] = {
		//	x      y      z			u	  v			nx	  ny    nz
			-1.0f, -1.0f, -0.6f,	0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
			0.0f, -1.0f, 1.0f,		0.5f, 0.0f,		0.0f, 0.0f, 0.0f,
			1.0f, -1.0f, -0.6f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f,		0.5f, 1.0f,		0.0f, 0.0f, 0.0f
	};

	unsigned int floorIndices[] = {
		0, 2, 1,
		1, 2, 3
	};

	GLfloat floorVertices[] = {
		-10.0f, 0.0f, -10.0f,	0.0f, 0.0f,		0.0f, -1.0f, 0.0f,
		10.0f, 0.0f, -10.0f,	10.0f, 0.0f,	0.0f, -1.0f, 0.0f,
		-10.0f, 0.0f, 10.0f,	0.0f, 10.0f,	0.0f, -1.0f, 0.0f,
		10.0f, 0.0f, 10.0f,		10.0f, 10.0f,	0.0f, -1.0f, 0.0f
	};

	unsigned int vegetacionIndices[] = {
		0, 1, 2,
		0, 2, 3,
		4,5,6,
		4,6,7
	};

	GLfloat vegetacionVertices[] = {
		-0.5f, -0.5f, 0.0f,		0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		0.5f, 0.5f, 0.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,
		-0.5f, 0.5f, 0.0f,		0.0f, 1.0f,		0.0f, 0.0f, 0.0f,

		0.0f, -0.5f, -0.5f,		0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.5f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.5f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, -0.5f,		0.0f, 1.0f,		0.0f, 0.0f, 0.0f,
	};

	MeshModel* obj1 = new MeshModel();
	obj1->CreateMeshModel(vertices, indices, 32, 12);
	meshListModel.push_back(obj1);

	MeshModel* obj2 = new MeshModel();
	obj2->CreateMeshModel(vertices, indices, 32, 12);
	meshListModel.push_back(obj2);

	MeshModel* obj3 = new MeshModel();
	obj3->CreateMeshModel(floorVertices, floorIndices, 32, 6);
	meshListModel.push_back(obj3);

	MeshModel* obj4 = new MeshModel();
	obj4->CreateMeshModel(vegetacionVertices, vegetacionIndices, 64, 12);
	meshListModel.push_back(obj4);
}

void CreateShaders()
{
	Shader* shader1 = new Shader();
	shader1->CreateFromFiles(vShader, fShader);
	shaderList.push_back(*shader1);
}

void CrearDado()
{
	unsigned int cubo_indices[] = {
		// front
		0, 1, 2,
		2, 3, 0,
		// back
		8, 9, 10,
		10, 11, 8,
		// left
		12, 13, 14,
		14, 15, 12,
		// bottom
		16, 17, 18,
		18, 19, 16,
		// top
		20, 21, 22,
		22, 23, 20,
		// right
		4, 5, 6,
		6, 7, 4,
	};

	// Coordenadas UV (S y T) ajustadas para mostrar 1 logo por cara 
	// tomando los primeros 6 logos de la cuadrícula
	GLfloat cubo_vertices[] = {
		// FRONT (Cara Frontal) - Logo 1
		//x		y		z		S		T			NX		NY		NZ
		-0.5f, -0.5f,  0.5f,	0.000f, 0.80f,		0.0f,	0.0f,	-1.0f,	//0
		 0.5f, -0.5f,  0.5f,	0.166f, 0.80f,		0.0f,	0.0f,	-1.0f,	//1
		 0.5f,  0.5f,  0.5f,	0.166f, 1.00f,		0.0f,	0.0f,	-1.0f,	//2
		-0.5f,  0.5f,  0.5f,	0.000f, 1.00f,		0.0f,	0.0f,	-1.0f,	//3

		// RIGHT (Cara Derecha) - Logo 2
		//x		y		z		S		T
		 0.5f, -0.5f,  0.5f,	0.166f, 0.80f,		-1.0f,	0.0f,	0.0f,
		 0.5f, -0.5f, -0.5f,	0.333f, 0.80f,		-1.0f,	0.0f,	0.0f,
		 0.5f,  0.5f, -0.5f,	0.333f, 1.00f,		-1.0f,	0.0f,	0.0f,
		 0.5f,  0.5f,  0.5f,	0.166f, 1.00f,		-1.0f,	0.0f,	0.0f,

		 // BACK (Cara Trasera) - Logo 3 (UV S invertido)
		 -0.5f, -0.5f, -0.5f,	0.500f, 0.80f,		0.0f,	0.0f,	1.0f,
		  0.5f, -0.5f, -0.5f,	0.333f, 0.80f,		0.0f,	0.0f,	1.0f,
		  0.5f,  0.5f, -0.5f,	0.333f, 1.00f,		0.0f,	0.0f,	1.0f,
		 -0.5f,  0.5f, -0.5f,	0.500f, 1.00f,		0.0f,	0.0f,	1.0f,

		 // LEFT (Cara Izquierda) - Logo 4
		 //x		y		z		S		T
		 -0.5f, -0.5f, -0.5f,	0.500f, 0.80f,		1.0f,	0.0f,	0.0f,
		 -0.5f, -0.5f,  0.5f,	0.666f, 0.80f,		1.0f,	0.0f,	0.0f,
		 -0.5f,  0.5f,  0.5f,	0.666f, 1.00f,		1.0f,	0.0f,	0.0f,
		 -0.5f,  0.5f, -0.5f,	0.500f, 1.00f,		1.0f,	0.0f,	0.0f,

		 // BOTTOM (Cara Inferior) - Logo 5
		 //x		y		z		S		T
		 -0.5f, -0.5f,  0.5f,	0.666f, 0.80f,		0.0f,	1.0f,	0.0f,
		  0.5f, -0.5f,  0.5f,	0.833f, 0.80f,		0.0f,	1.0f,	0.0f,
		  0.5f, -0.5f, -0.5f,	0.833f, 1.00f,		0.0f,	1.0f,	0.0f,
		 -0.5f, -0.5f, -0.5f,	0.666f, 1.00f,		0.0f,	1.0f,	0.0f,

		 // UP (Cara Superior) - Logo 6
		 //x		y		z		S		T
		 -0.5f,  0.5f,  0.5f,	0.833f, 0.80f,		0.0f,	-1.0f,	0.0f,
		  0.5f,  0.5f,  0.5f,	1.000f, 0.80f,		0.0f,	-1.0f,	0.0f,
		  0.5f,  0.5f, -0.5f,	1.000f, 1.00f,		0.0f,	-1.0f,	0.0f,
		 -0.5f,  0.5f, -0.5f,	0.833f, 1.00f,		0.0f,	-1.0f,	0.0f,
	};

	MeshModel* dado = new MeshModel();
	dado->CreateMeshModel(cubo_vertices, cubo_indices, 192, 36);
	meshListModel.push_back(dado);
}

void CrearHolocron()
{
	unsigned int holocron_indices[] = {
	 12, 1, 0,
	 12, 4, 15,
	 14, 9, 2,
	 2, 12, 15,
	 8,  3,  4,
	 15,  4,  3,
	 3, 6, 14,
	 4, 5,  8,
	 6, 7, 13,
	 9, 10, 11,
	 11, 2, 9,
	 10, 7, 5,
	 13, 14, 6,
	 2, 3, 14,
	 2, 15, 3,
	 11, 12, 2,
	 13, 9, 14,
	 8, 6, 3,
	 12, 0, 4,
	 12, 11, 1,
	 4, 0, 5,
	 6, 8, 7,
	 9, 13, 10,
	 5, 0, 1,
	 1, 11, 10,
	 10, 13, 7,
	 7, 8, 5,
	 5, 1, 10
	};

	GLfloat holocron_vertices[] = {
		// x			y			z			S		T		NX		NY		NZ
		 -2.517274,		-2.579095,	0.002115,	0.02f,	0.02f,	0.0f,	0.0f,	0.0f,
		-1.185055,		-2.530499,	-1.198261,	0.27f,	0.03f,	0.0f,	0.0f,	0.0f,
		 0.049649,		 2.440128,	-2.417217,	0.50f,	0.98f,	0.0f,	0.0f,	0.0f,
		 0.051152,		 2.399781,	 2.545043,	0.50f,	0.97f,	0.0f,	0.0f,	0.0f,
		-2.646786,		-0.139047,	 2.493757,	0.00f,	0.49f,	0.0f,	0.0f,	0.0f,
		-1.328995,		-2.578386,	 1.297838,	0.25f,	0.02f,	0.0f,	0.0f,	0.0f,
		 2.593003,		-0.085222,	 2.505755,	0.99f,	0.50f,	0.0f,	0.0f,	0.0f,
		 1.385028,		-2.549476,	 1.308677,	0.76f,	0.03f,	0.0f,	0.0f,	0.0f,
		-0.140696,		-2.601934,	 2.558561,	0.47f,	0.00f,	0.0f,	0.0f,	0.0f,
		 2.534944,		-0.061146,	-2.580280,	0.98f,	0.50f,	0.0f,	0.0f,	0.0f,
		 1.344182,		-2.542571,	-1.294387,	0.75f,	0.03f,	0.0f,	0.0f,	0.0f,
		 0.081432,		-2.513204,	-2.426005,	0.51f,	0.04f,	0.0f,	0.0f,	0.0f,
		-2.453980,		-0.063388,	-2.477451,	0.03f,	0.50f,	0.0f,	0.0f,	0.0f,
		 2.632201,		-2.553892,	 0.007226,	1.00f,	0.03f,	0.0f,	0.0f,	0.0f,
		 2.521206,		 2.448241,	 0.063388,	0.98f,	0.98f,	0.0f,	0.0f,	0.0f,
		-2.451372,		 2.421759,	 0.077538,	0.03f,	0.97f,	0.0f,	0.0f,	0.0f,
	};

	MeshModel* holocron = new MeshModel();
	holocron->CreateMeshModel(holocron_vertices, holocron_indices, 120, 84);
	meshListModel.push_back(holocron);
}

int main()
{
	mainWindow = Window(1366, 768);
	mainWindow.Initialise();

	CreateObjects();
	CrearDado();
	CrearHolocron();
	CreateShaders();

	camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -60.0f, 0.0f, 0.3f, 0.5f);

	plainTexture = Texture("Textures/plain.png");
	plainTexture.LoadTextureA();
	pisoTexture = Texture("Textures/piso.tga");
	pisoTexture.LoadTextureA();

	// Ejercicio 1: Textura del dado cargada correctamente
	dadoTexture = Texture("Textures/star wars logos.png");
	dadoTexture.LoadTextureA();

	holocronTexture = Texture("Textures/star wars logos.png");
	holocronTexture.LoadTextureA();

	Holocron_M = Model();
	Holocron_M.LoadModel("Models/Holo1.obj");

	// Ejercicio 2: Modelo importado desde Assimp
	Dado_M = Model();
	Dado_M.LoadModel("Models/Cubo_sw.obj"); 

	// Ejercicio 3
	Avion_M = Model();
	Avion_M.LoadModel("Models/PlaneSam.obj"); 
	
	std::vector<std::string> skyboxFaces;
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_rt.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_lf.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_dn.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_up.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_bk.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_ft.tga");

	skybox = Skybox(skyboxFaces);

	GLuint uniformProjection = 0, uniformModel = 0, uniformView = 0, uniformEyePosition = 0,
		uniformSpecularIntensity = 0, uniformShininess = 0;
	GLuint uniformColor = 0;
	glm::mat4 projection = glm::perspective(45.0f, (GLfloat)mainWindow.getBufferWidth() / mainWindow.getBufferHeight(), 0.1f, 1000.0f);

	glm::mat4 model(1.0);
	glm::mat4 modelaux(1.0);
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);

	// Loop mientras no se cierra la ventana
	while (!mainWindow.getShouldClose())
	{
		GLfloat now = glfwGetTime();
		deltaTime = now - lastTime;
		deltaTime += (now - lastTime) / limitFPS;
		lastTime = now;

		// Recibir eventos del usuario
		glfwPollEvents();
		camera.keyControl(mainWindow.getsKeys(), deltaTime);
		camera.mouseControl(mainWindow.getXChange(), mainWindow.getYChange());

		// Clear the window
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		skybox.DrawSkybox(camera.calculateViewMatrix(), projection);
		shaderList[0].UseShader();
		uniformModel = shaderList[0].GetModelLocation();
		uniformProjection = shaderList[0].GetProjectionLocation();
		uniformView = shaderList[0].GetViewLocation();
		uniformColor = shaderList[0].getColorLocation();
		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));
		glUniform3f(uniformEyePosition, camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);

		color = glm::vec3(1.0f, 1.0f, 1.0f); // color blanco

		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
		model = glm::scale(model, glm::vec3(30.0f, 1.0f, 30.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));

		pisoTexture.UseTexture();
		meshListModel[2]->RenderMeshModel();

		// Ejercicio 1: Dado de OpenGL con la textura optimizada de Star Wars
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-1.5f, 4.5f, -2.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		dadoTexture.UseTexture();
		meshListModel[4]->RenderMeshModel();

		// Ejercicio 2: Dado importado desde Blender/Maya con Assimp
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-3.0f, 3.0f, -2.0f));
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f)); // Ajusta esta escala si el cubo se ve muy grande o pequeño
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		Dado_M.RenderModel();

		// Holocrones
		color = glm::vec3(0.0f, 1.0f, 0.0f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-8.5f, 4.5f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		holocronTexture.UseTexture();
		meshListModel[5]->RenderMeshModel();

		color = glm::vec3(1.0f, 1.0f, 1.0f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-4.5f, 2.5f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		Holocron_M.RenderModel();

		// --- AGREGADO PARA EL EJERCICIO 3: Renderizar el avión ---
		color = glm::vec3(1.0f, 1.0f, 1.0f); // Base blanca para no alterar la textura
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(5.0f, 5.0f, -2.0f)); // Posicionado en el aire, a la derecha
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f)); // Modifica este valor si el avión sale muy grande
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		Avion_M.RenderModel();
		// ---------------------------------------------------------

		glUseProgram(0);

		mainWindow.swapBuffers();
	}

	return 0;
}