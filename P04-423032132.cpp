/*Práctica 4: Modelado Jerárquico.
Se implementa el uso de matrices adicionales para almacenar información de transformaciones geométricas que se quiere
heredar entre diversas instancias para que estén unidas
Teclas de la F a la K para rotaciones de articulaciones
*/
#include <stdio.h>
#include <string.h>
#include<cmath>
#include<vector>
#include <glew.h>
#include <glfw3.h>
//glm
#include<glm.hpp>
#include<gtc\matrix_transform.hpp>
#include<gtc\type_ptr.hpp>
#include <gtc\random.hpp>
//clases para dar orden y limpieza al còdigo
#include"Mesh.h"
#include"Shader.h"
#include"Sphere.h"
#include"Window.h"
#include"Camera.h"

using std::vector;

//Dimensiones de la ventana
const float toRadians = 3.14159265f / 180.0; //grados a radianes
const float PI = 3.14159265f;
GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;
static double limitFPS = 1.0 / 60.0;
Camera camera;
Window mainWindow;
vector<Mesh*> meshList;
vector<Shader>shaderList;
//Vertex Shader
static const char* vShader = "shaders/shader.vert";
static const char* fShader = "shaders/shader.frag";
Sphere sp = Sphere(1.0, 20, 20); //recibe radio, slices, stacks

void CrearCubo()
{
	unsigned int cubo_indices[] = {
		// front
		0, 1, 2,
		2, 3, 0,
		// right
		1, 5, 6,
		6, 2, 1,
		// back
		7, 6, 5,
		5, 4, 7,
		// left
		4, 0, 3,
		3, 7, 4,
		// bottom
		4, 5, 1,
		1, 0, 4,
		// top
		3, 2, 6,
		6, 7, 3
	};

	GLfloat cubo_vertices[] = {
		// front
		-0.5f, -0.5f,0.5f,
		0.5f, -0.5f,0.5f,
		0.5f,0.5f,0.5f,
		-0.5f,0.5f,0.5f,
		// back
		-0.5f, -0.5f, -0.5f,
		0.5f, -0.5f, -0.5f,
		0.5f,0.5f, -0.5f,
		-0.5f,0.5f, -0.5f
	};
	Mesh* cubo = new Mesh();
	cubo->CreateMesh(cubo_vertices, cubo_indices, 24, 36);
	meshList.push_back(cubo);
}

// Pirámide triangular regular
void CrearPiramideTriangular()
{
	unsigned int indices_piramide_triangular[] = {
			0,1,2,
			1,3,2,
			3,0,2,
			1,0,3
	};
	GLfloat vertices_piramide_triangular[] = {
		-0.5f, -0.5f,0.0f,	//0
		0.5f,-0.5f,0.0f,	//1
		0.0f,0.5f, -0.25f,	//2
		0.0f,-0.5f,-0.5f,	//3
	};
	Mesh* obj1 = new Mesh();
	obj1->CreateMesh(vertices_piramide_triangular, indices_piramide_triangular, 12, 12);
	meshList.push_back(obj1);
}

void CrearCilindro(int res, float R) {
	int n, i;
	GLfloat dt = 2 * PI / res, x, z, y = -0.5f;
	vector<GLfloat> vertices;
	vector<unsigned int> indices;

	for (n = 0; n <= (res); n++) {
		if (n != res) {
			x = R * cos((n)*dt);
			z = R * sin((n)*dt);
		}
		else {
			x = R * cos((0) * dt);
			z = R * sin((0) * dt);
		}
		for (i = 0; i < 6; i++) {
			switch (i) {
			case 0: vertices.push_back(x); break;
			case 1: vertices.push_back(y); break;
			case 2: vertices.push_back(z); break;
			case 3: vertices.push_back(x); break;
			case 4: vertices.push_back(0.5); break;
			case 5: vertices.push_back(z); break;
			}
		}
	}

	for (n = 0; n <= (res); n++) {
		x = R * cos((n)*dt);
		z = R * sin((n)*dt);
		for (i = 0; i < 3; i++) {
			switch (i) {
			case 0: vertices.push_back(x); break;
			case 1: vertices.push_back(-0.5f); break;
			case 2: vertices.push_back(z); break;
			}
		}
	}

	for (n = 0; n <= (res); n++) {
		x = R * cos((n)*dt);
		z = R * sin((n)*dt);
		for (i = 0; i < 3; i++) {
			switch (i) {
			case 0: vertices.push_back(x); break;
			case 1: vertices.push_back(0.5); break;
			case 2: vertices.push_back(z); break;
			}
		}
	}

	for (i = 0; i < vertices.size(); i++) indices.push_back(i);

	Mesh* cilindro = new Mesh();
	cilindro->CreateMeshGeometry(vertices, indices, vertices.size(), indices.size());
	meshList.push_back(cilindro);
}

void CrearCono(int res, float R) {
	int n, i;
	GLfloat dt = 2 * PI / res, x, z, y = -0.5f;
	vector<GLfloat> vertices;
	vector<unsigned int> indices;

	vertices.push_back(0.0);
	vertices.push_back(0.5);
	vertices.push_back(0.0);

	for (n = 0; n <= (res); n++) {
		x = R * cos((n)*dt);
		z = R * sin((n)*dt);
		for (i = 0; i < 3; i++) {
			switch (i) {
			case 0: vertices.push_back(x); break;
			case 1: vertices.push_back(y); break;
			case 2: vertices.push_back(z); break;
			}
		}
	}
	vertices.push_back(R * cos(0) * dt);
	vertices.push_back(-0.5);
	vertices.push_back(R * sin(0) * dt);

	for (i = 0; i < res + 2; i++) indices.push_back(i);

	Mesh* cono = new Mesh();
	cono->CreateMeshGeometry(vertices, indices, vertices.size(), res + 2);
	meshList.push_back(cono);
}

void CrearPiramideCuadrangular()
{
	vector<unsigned int> piramidecuadrangular_indices = {
		0,3,4,
		3,2,4,
		2,1,4,
		1,0,4,
		0,1,2,
		0,2,4
	};
	vector<GLfloat> piramidecuadrangular_vertices = {
		0.5f,-0.5f,0.5f,
		0.5f,-0.5f,-0.5f,
		-0.5f,-0.5f,-0.5f,
		-0.5f,-0.5f,0.5f,
		0.0f,0.5f,0.0f,
	};
	Mesh* piramide = new Mesh();
	piramide->CreateMeshGeometry(piramidecuadrangular_vertices, piramidecuadrangular_indices, 15, 18);
	meshList.push_back(piramide);
}

void CreateShaders()
{
	Shader* shader1 = new Shader();
	shader1->CreateFromFiles(vShader, fShader);
	shaderList.push_back(*shader1);
}

int main()
{
	mainWindow = Window(800, 600);
	mainWindow.Initialise();

	CrearCubo();//índice 0 
	CrearPiramideTriangular();//índice 1 
	CrearCilindro(18, 1.0f);//índice 2 
	CrearCono(25, 2.0f);//índice 3 
	CrearPiramideCuadrangular();//índice 4 
	CreateShaders();

	camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -60.0f, 0.0f, 0.3f, 0.3f);

	GLuint uniformProjection = 0;
	GLuint uniformModel = 0;
	GLuint uniformView = 0;
	GLuint uniformColor = 0;
	glm::mat4 projection = glm::perspective(glm::radians(60.0f), mainWindow.getBufferWidth() / mainWindow.getBufferHeight(), 0.1f, 100.0f);

	sp.init();
	sp.load();

	glm::mat4 model(1.0);
	glm::mat4 modelaux(1.0);
	glm::mat4 modelaux2(1.0);

	glm::vec3 color = glm::vec3(0.0f, 0.0f, 0.0f);

	GLfloat rotLlantas[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	GLfloat rotMuneca = 0.0f;
	GLfloat aperturaPinza = 0.0f;

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
		float velGiro = 150.0f * deltaTime; // Qué tan rápido giran las piezas

		// Llantas (1, 2, 3, 4, 5, 6 para adelante / Q, W, E, R, T, Y para atrás)
		if (keys[GLFW_KEY_1]) rotLlantas[0] += velGiro; // Frontal Derecha
		if (keys[GLFW_KEY_Q]) rotLlantas[0] -= velGiro;
		if (keys[GLFW_KEY_2]) rotLlantas[1] += velGiro; // Frontal Izquierda
		if (keys[GLFW_KEY_W]) rotLlantas[1] -= velGiro;
		if (keys[GLFW_KEY_3]) rotLlantas[2] += velGiro; // Media Derecha
		if (keys[GLFW_KEY_E]) rotLlantas[2] -= velGiro;
		if (keys[GLFW_KEY_4]) rotLlantas[3] += velGiro; // Media Izquierda
		if (keys[GLFW_KEY_R]) rotLlantas[3] -= velGiro;
		if (keys[GLFW_KEY_5]) rotLlantas[4] += velGiro; // Trasera Derecha
		if (keys[GLFW_KEY_T]) rotLlantas[4] -= velGiro;
		if (keys[GLFW_KEY_6]) rotLlantas[5] += velGiro; // Trasera Izquierda
		if (keys[GLFW_KEY_Y]) rotLlantas[5] -= velGiro;

		// Muñeca (Z / X)
		if (keys[GLFW_KEY_Z]) rotMuneca += velGiro;
		if (keys[GLFW_KEY_X]) rotMuneca -= velGiro;

		// Abrir/Cerrar Pinza (C / V)
		if (keys[GLFW_KEY_C]) { aperturaPinza += 1.0f * deltaTime; if (aperturaPinza > 0.4f) aperturaPinza = 0.4f; } // Límite para no desarmarla
		if (keys[GLFW_KEY_V]) { aperturaPinza -= 1.0f * deltaTime; if (aperturaPinza < 0.0f) aperturaPinza = 0.0f; } // Límite cerrado
		// -----------------------------------------------

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shaderList[0].useShader();
		uniformModel = shaderList[0].getModelLocation();
		uniformProjection = shaderList[0].getProjectLocation();
		uniformView = shaderList[0].getViewLocation();
		uniformColor = shaderList[0].getColorLocation();

		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, 4.0f, -4.0f));
		modelaux = model;

		// Cabina del rover 
		model = glm::translate(model, glm::vec3(1.0f, 2.0f, 0.0f));
		model = glm::scale(model, glm::vec3(8.0f, 4.0f, 4.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));
		color = glm::vec3(0.5f, 0.5f, 0.5f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		meshList[0]->RenderMesh();

		// BASE
		model = modelaux;
		model = glm::translate(model, glm::vec3(0.0f, -0.75f, 0.0f));
		modelaux = model;
		model = glm::scale(model, glm::vec3(10.0f, 1.5f, 8.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(0.7f, 0.7f, 0.7f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		meshList[0]->RenderMesh();

		model = modelaux;
		modelaux2 = model;

		//BASE PARA EL BRAZO
		model = modelaux2;
		model = glm::translate(model, glm::vec3(-3.75f, 1.25f, 3.0f));
		modelaux = model;
		color = glm::vec3(0.0f, 0.0f, 1.0f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		meshList[0]->RenderMesh();
		model = modelaux;

		//Eje de Rotación esfera
		model = glm::translate(model, glm::vec3(0.0f, 0.75f, 0.0f));
		model = glm::rotate(model, glm::radians(mainWindow.getarticulacion1()), glm::vec3(0.0f, 0.0f, 1.0f));
		modelaux = model;
		model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(0.0f, 0.0f, 1.0f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		sp.render();
		model = modelaux;

		//Brazo parte 1
		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::translate(model, glm::vec3(-0.25f, 1.5f, 0.0f));
		modelaux = model;
		model = glm::scale(model, glm::vec3(0.5f, 3.0f, 0.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(0.5f, 0.5f, 0.5f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		meshList[2]->RenderMeshGeometry();

		//Eje de Rotación esfera 2
		model = modelaux;
		model = glm::translate(model, glm::vec3(0.0, 1.5f, 0.0f));
		model = glm::rotate(model, glm::radians(mainWindow.getarticulacion2()), glm::vec3(0.0f, 0.0f, 1.0f));
		modelaux = model;
		model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(0.0f, 0.0f, 1.0f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		sp.render();
		model = modelaux;

		// Brazo parte 2
		model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0f));
		modelaux = model;
		model = glm::scale(model, glm::vec3(0.4f, 3.0f, 0.4f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(0.5f, 0.5f, 0.5f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		meshList[2]->RenderMeshGeometry();

		// Base de la pinza (Muñeca)
		model = modelaux;
		model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0f));
		model = glm::rotate(model, glm::radians(rotMuneca), glm::vec3(0.0f, 1.0f, 0.0f)); // <-- ROTACIÓN MUÑECA (Z/X)
		modelaux = model;
		model = glm::scale(model, glm::vec3(1.0f, 0.2f, 0.4f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(0.8f, 0.4f, 0.0f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		meshList[0]->RenderMesh();

		// Dedo 1 (Derecho)
		model = modelaux;
		model = glm::translate(model, glm::vec3(0.4f + aperturaPinza, 0.5f, 0.0f)); // <-- APERTURA (C/V)
		model = glm::scale(model, glm::vec3(0.2f, 1.0f, 0.4f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(0.3f, 0.3f, 0.3f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		meshList[0]->RenderMesh();

		// Dedo 2 (Izquierdo)
		model = modelaux;
		model = glm::translate(model, glm::vec3(-0.4f - aperturaPinza, 0.5f, 0.0f)); // <-- APERTURA (C/V)
		model = glm::scale(model, glm::vec3(0.2f, 1.0f, 0.4f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		meshList[0]->RenderMesh();


		// --- LLANTAS DEL ROVER ---

		color = glm::vec3(0.1f, 0.1f, 0.1f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));

		// BASE PARA LA LLANTA FRONTAL DERECHA (Usa 1 y Q)
		model = modelaux2;
		model = glm::translate(model, glm::vec3(4.0f, -0.75f, 4.5f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotLlantas[0]), glm::vec3(0.0f, 1.0f, 0.0f)); // <-- GIRO INDEPENDIENTE
		model = glm::scale(model, glm::vec3(2.0f, 1.0f, 2.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		meshList[2]->RenderMeshGeometry();

		// BASE PARA LA LLANTA FRONTAL IZQUIERDA (Usa 2 y W)
		model = modelaux2;
		model = glm::translate(model, glm::vec3(4.0f, -0.75f, -4.5f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotLlantas[1]), glm::vec3(0.0f, 1.0f, 0.0f)); // <-- GIRO INDEPENDIENTE
		model = glm::scale(model, glm::vec3(2.0f, 1.0f, 2.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		meshList[2]->RenderMeshGeometry();

		// BASE PARA LA LLANTA DE EN MEDIO DERECHA (Usa 3 y E)
		model = modelaux2;
		model = glm::translate(model, glm::vec3(0.0f, -0.75f, 4.5f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotLlantas[2]), glm::vec3(0.0f, 1.0f, 0.0f)); // <-- GIRO INDEPENDIENTE
		model = glm::scale(model, glm::vec3(2.0f, 1.0f, 2.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		meshList[2]->RenderMeshGeometry();

		// BASE PARA LA LLANTA DE EN MEDIO IZQUIERDA (Usa 4 y R)
		model = modelaux2;
		model = glm::translate(model, glm::vec3(0.0f, -0.75f, -4.5f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotLlantas[3]), glm::vec3(0.0f, 1.0f, 0.0f)); // <-- GIRO INDEPENDIENTE
		model = glm::scale(model, glm::vec3(2.0f, 1.0f, 2.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		meshList[2]->RenderMeshGeometry();

		// BASE PARA LA LLANTA DE ATRÁS DERECHA (Usa 5 y T)
		model = modelaux2;
		model = glm::translate(model, glm::vec3(-4.0f, -0.75f, 4.5f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotLlantas[4]), glm::vec3(0.0f, 1.0f, 0.0f)); // <-- GIRO INDEPENDIENTE
		model = glm::scale(model, glm::vec3(2.0f, 1.0f, 2.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		meshList[2]->RenderMeshGeometry();

		// BASE PARA LA LLANTA DE ATRÁS IZQUIERDA (Usa 6 y Y)
		model = modelaux2;
		model = glm::translate(model, glm::vec3(-4.0f, -0.75f, -4.5f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotLlantas[5]), glm::vec3(0.0f, 1.0f, 0.0f)); // <-- GIRO INDEPENDIENTE
		model = glm::scale(model, glm::vec3(2.0f, 1.0f, 2.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		meshList[2]->RenderMeshGeometry();

		glUseProgram(0);
		mainWindow.swapBuffers();
	}
	return 0;
} 