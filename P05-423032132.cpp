/*
* Práctica 5: Optimización y Carga de Modelos
*/
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
//#include "Sphere.h" // Comentado si no se usa
#include "Model.h"
#include "Skybox.h"
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32s.lib") 
// Asegúrese de usar el nombre correcto del archivo de Assimp que tenga en su carpeta
#pragma comment(lib, "assimp-vc140-mt.lib") 
// Vamos a usar SOLO la versión estática y a bloquear la versión dinámica
#pragma comment(lib, "glfw3.lib")
#pragma comment(linker, "/NODEFAULTLIB:glfw3dll.lib")

const float toRadians = 3.14159265f / 180.0f;

Window mainWindow;
std::vector<Mesh*> meshList;
std::vector<MeshColor*> meshListColor;
std::vector<MeshModel*> meshListModel;
std::vector<Shader> shaderList;

Camera camera;

// ==========================================
// LISTA DE MODELOS A IMPORTAR
// ==========================================
Model Rover_M;
Model Holocron_Centro;
Model Holocron_Esquina1;

// Modelos del Satélite (Separados para cumplir la rúbrica)
Model Satelite_Cuerpo;
Model Satelite_PanelIzq;
Model Satelite_PanelDer;
Model Satelite_Antena;

// ==========================================
// VARIABLES DE CONTROL POR TECLADO
// ==========================================
float anguloEsq1 = 0.0f;

// 4.- Movimiento del Satélite en ejes X, Y, Z
float satX = 0.0f;
float satY = 5.0f;
float satZ = 0.0f;

// 5.- Rotaciones independientes de 3 partes del satélite
float anguloPanelIzq = 0.0f;
float anguloPanelDer = 0.0f;
float anguloAntena = 0.0f;

Skybox skybox;

GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;
static double limitFPS = 1.0 / 60.0;

static const char* vShader = "shaders/shader_m.vert";
static const char* fShader = "shaders/shader_m.frag";

void CreateObjects()
{
	unsigned int indices[] = { 0, 3, 1, 1, 3, 2, 2, 3, 0, 0, 1, 2 };
	GLfloat vertices[] = {
		-1.0f, -1.0f, -0.6f,	0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 1.0f,		0.5f, 0.0f,		0.0f, 0.0f, 0.0f,
		1.0f, -1.0f, -0.6f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,		0.5f, 1.0f,		0.0f, 0.0f, 0.0f
	};

	unsigned int floorIndices[] = { 0, 2, 1, 1, 2, 3 };
	GLfloat floorVertices[] = {
		-50.0f, 0.0f, -50.0f,	0.0f, 0.0f,		0.0f, -1.0f, 0.0f,
		50.0f, 0.0f, -50.0f,	10.0f, 0.0f,	0.0f, -1.0f, 0.0f,
		-50.0f, 0.0f, 50.0f,	0.0f, 10.0f,	0.0f, -1.0f, 0.0f,
		50.0f, 0.0f, 50.0f,		10.0f, 10.0f,	0.0f, -1.0f, 0.0f
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
}

void CreateShaders()
{
	Shader* shader1 = new Shader();
	shader1->CreateFromFiles(vShader, fShader);
	shaderList.push_back(*shader1);
}

int main()
{
	mainWindow = Window(1366, 768);
	mainWindow.Initialise();

	CreateObjects();
	CreateShaders();

	camera = Camera(glm::vec3(0.0f, 4.0f, 18.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -10.0f, 0.3f, 0.3f);

	// ==========================================
	// CARGA DE MODELOS 3D
	// ==========================================
	Rover_M = Model();
	Rover_M.LoadModel("Models/RoverModificado.obj");

	Holocron_Centro = Model();
	Holocron_Centro.LoadModel("Models/Holocron.obj");

	Holocron_Esquina1 = Model();
	Holocron_Esquina1.LoadModel("Models/Holocron_Esquina1.obj");

	// 3.- IMPORTAR EL SATÉLITE CON JERARQUÍA (Archivos separados)
	Satelite_Cuerpo = Model();
	Satelite_Cuerpo.LoadModel("Models/satellite_obj.obj"); // El cuerpo base

	Satelite_PanelIzq = Model();
	Satelite_PanelIzq.LoadModel("Models/satellite_panel_izq.obj"); // Debe crearlo en Blender

	Satelite_PanelDer = Model();
	Satelite_PanelDer.LoadModel("Models/satellite_panel_der.obj"); // Debe crearlo en Blender

	Satelite_Antena = Model();
	Satelite_Antena.LoadModel("Models/satellite_antena.obj"); // Debe crearlo en Blender

	// Skybox
	std::vector<std::string> skyboxFaces;
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_rt.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_lf.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_dn.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_up.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_bk.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_ft.tga");
	skybox = Skybox(skyboxFaces);

	GLuint uniformProjection = 0, uniformModel = 0, uniformView = 0, uniformColor = 0;
	glm::mat4 projection = glm::perspective(45.0f, (GLfloat)mainWindow.getBufferWidth() / mainWindow.getBufferHeight(), 0.1f, 1000.0f);

	glm::mat4 model(1.0);
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);

	while (!mainWindow.getShouldClose())
	{
		GLfloat now = glfwGetTime();
		deltaTime = now - lastTime;
		deltaTime += (now - lastTime) / limitFPS;
		lastTime = now;

		glfwPollEvents();
		camera.keyControl(mainWindow.getsKeys(), deltaTime);
		camera.mouseControl(mainWindow.getXChange(), mainWindow.getYChange());

		bool* keys = mainWindow.getsKeys();

		// ==========================================
		// 4.- MOVIMIENTO DEL SATELITE EN X, Y, Z
		// ==========================================
		if (keys[GLFW_KEY_I]) satY += 0.1f; // Arriba
		if (keys[GLFW_KEY_K]) satY -= 0.1f; // Abajo
		if (keys[GLFW_KEY_J]) satX -= 0.1f; // Izquierda
		if (keys[GLFW_KEY_L]) satX += 0.1f; // Derecha
		if (keys[GLFW_KEY_U]) satZ -= 0.1f; // Adelante
		if (keys[GLFW_KEY_O]) satZ += 0.1f; // Atrás

		// ==========================================
		// 5.- ROTACIÓN DE 3 PARTES DEL SATÉLITE
		// ==========================================
		if (keys[GLFW_KEY_Z]) anguloPanelIzq += 1.5f;
		if (keys[GLFW_KEY_X]) anguloPanelDer += 1.5f;
		if (keys[GLFW_KEY_C]) anguloAntena += 1.5f;

		// Rotación del Holocrón
		if (keys[GLFW_KEY_1]) anguloEsq1 += 1.5f;

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

		// --- DIBUJO DEL PISO ---
		color = glm::vec3(0.5f, 0.5f, 0.5f);
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
		model = glm::scale(model, glm::vec3(30.0f, 1.0f, 30.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		meshListModel[2]->RenderMeshModel();

		// --- 1. DIBUJO DEL ROVER ---
		color = glm::vec3(0.8f, 0.8f, 0.8f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-8.0f, -2.0f, 0.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		Rover_M.RenderModel();

		// --- 2. DIBUJO DEL HOLOCRÓN ---
		color = glm::vec3(0.2f, 0.8f, 1.0f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		glm::mat4 modelHolocron = glm::mat4(1.0);
		modelHolocron = glm::translate(modelHolocron, glm::vec3(8.0f, 2.0f, 0.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelHolocron));
		Holocron_Centro.RenderModel();

		glm::mat4 modelEsq1 = modelHolocron;
		modelEsq1 = glm::rotate(modelEsq1, glm::radians(anguloEsq1), glm::vec3(0.0f, 1.0f, 0.0f));
		modelEsq1 = glm::translate(modelEsq1, glm::vec3(1.5f, 1.5f, 1.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelEsq1));
		Holocron_Esquina1.RenderModel();

		// =============================================================
		// 3. DIBUJO DEL SATÉLITE (Cumpliendo Jerarquía y Rotación Contextual)
		// =============================================================
		color = glm::vec3(0.9f, 0.7f, 0.2f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));

		// PADRE: Cuerpo del Satélite
		glm::mat4 modelSatelite = glm::mat4(1.0);
		modelSatelite = glm::translate(modelSatelite, glm::vec3(satX, satY, satZ)); // Ejes X, Y, Z manejados por teclado
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelSatelite));
		Satelite_Cuerpo.RenderModel();

		// HIJO 1: Panel Solar Izquierdo
		glm::mat4 modelPanIzq = modelSatelite; // Hereda la posición actual del satélite
		// Nos ubicamos en la articulación del panel (Debe ajustar el 2.0f según el tamaño de su modelo)
		modelPanIzq = glm::translate(modelPanIzq, glm::vec3(-2.0f, 0.0f, 0.0f));
		// Giramos JUSTO en esa conexión (Contexto Adecuado)
		modelPanIzq = glm::rotate(modelPanIzq, glm::radians(anguloPanelIzq), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelPanIzq));
		Satelite_PanelIzq.RenderModel();

		// HIJO 2: Panel Solar Derecho
		glm::mat4 modelPanDer = modelSatelite;
		// Nos ubicamos en la articulación derecha
		modelPanDer = glm::translate(modelPanDer, glm::vec3(2.0f, 0.0f, 0.0f));
		modelPanDer = glm::rotate(modelPanDer, glm::radians(anguloPanelDer), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelPanDer));
		Satelite_PanelDer.RenderModel();

		// HIJO 3: Antena / Disco de comunicaciones
		glm::mat4 modelAntena = modelSatelite;
		// Nos ubicamos en la parte superior del satélite donde se conecta la antena
		modelAntena = glm::translate(modelAntena, glm::vec3(0.0f, 1.5f, 0.0f));
		modelAntena = glm::rotate(modelAntena, glm::radians(anguloAntena), glm::vec3(0.0f, 1.0f, 0.0f)); // Rota como un radar
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelAntena));
		Satelite_Antena.RenderModel();

		glUseProgram(0);
		mainWindow.swapBuffers();
	}

	return 0;
}