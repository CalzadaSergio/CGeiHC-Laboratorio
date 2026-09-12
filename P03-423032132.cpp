//práctica 3: Modelado Geométrico y Cámara Sintética.
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <vector>
#include <glew.h>
#include <glfw3.h>
//glm
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtc/random.hpp>
//clases para dar orden y limpieza al código
#include "Mesh.h"
#include "Shader.h"
#include "Sphere.h"
#include "Window.h"
#include "Camera.h"

using std::vector;

//Dimensiones de la ventana
const float toRadians = 3.14159265f / 180.0f; //grados a radianes
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
static const char* vShaderColor = "shaders/shadercolor.vert";
Sphere sp = Sphere(1.0, 20, 20);

void CrearCubo()
{
	unsigned int cubo_indices[] = {
		0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1,
		7, 6, 5, 5, 4, 7, 4, 0, 3, 3, 7, 4,
		4, 5, 1, 1, 0, 4, 3, 2, 6, 6, 7, 3
	};
	GLfloat cubo_vertices[] = {
		-0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f
	};
	Mesh* cubo = new Mesh();
	cubo->CreateMesh(cubo_vertices, cubo_indices, 24, 36);
	meshList.push_back(cubo); // Índice 0
}

void CrearPiramideTriangular()
{
	unsigned int indices_piramide_triangular[] = { 0,1,2, 1,3,2, 3,0,2, 1,0,3 };
	GLfloat vertices_piramide_triangular[] = {
		-0.5f, -0.5f,  0.0f,  0.5f, -0.5f,  0.0f,  0.0f,  0.5f, -0.25f,  0.0f, -0.5f, -0.5f,
	};
	Mesh* piramidet = new Mesh();
	piramidet->CreateMesh(vertices_piramide_triangular, indices_piramide_triangular, 12, 12);
	meshList.push_back(piramidet); // Índice 1
}

void CrearPiramideCuadrangular()
{
	unsigned int piramidecuadrangular_indices[] = { 0,3,4, 3,2,4, 2,1,4, 1,0,4, 0,1,2, 0,2,3 };
	GLfloat piramidecuadrangular_vertices[] = {
		 0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f,  0.0f,  0.5f,  0.0f,
	};
	Mesh* piramidec = new Mesh();
	piramidec->CreateMesh(piramidecuadrangular_vertices, piramidecuadrangular_indices, 15, 18);
	meshList.push_back(piramidec); // Índice 4 (después de cono y cilindro)
}

void CrearCilindro(int res, float R) {
	int n, i;
	GLfloat dt = 2 * PI / res, x, z, y = -0.5f;
	vector<GLfloat> vertices;
	vector<unsigned int> indices;

	for (n = 0; n <= res; n++) {
		x = R * cos(n * dt); z = R * sin(n * dt);
		vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
		vertices.push_back(x); vertices.push_back(0.5); vertices.push_back(z);
	}
	for (n = 0; n <= res; n++) {
		x = R * cos(n * dt); z = R * sin(n * dt);
		vertices.push_back(x); vertices.push_back(-0.5f); vertices.push_back(z);
	}
	for (n = 0; n <= res; n++) {
		x = R * cos(n * dt); z = R * sin(n * dt);
		vertices.push_back(x); vertices.push_back(0.5); vertices.push_back(z);
	}
	for (i = 0; i < vertices.size() / 3; i++) indices.push_back(i);
	Mesh* cilindro = new Mesh();
	cilindro->CreateMeshGeometry(vertices, indices, (unsigned int)vertices.size(), (unsigned int)indices.size());
	meshList.push_back(cilindro); // Índice 2
}

void CrearCono(int res, float R) {
	int n, i;
	GLfloat dt = 2 * PI / res, x, z, y = -0.5f;
	vector<GLfloat> vertices;
	vector<unsigned int> indices;

	vertices.push_back(0.0); vertices.push_back(0.5); vertices.push_back(0.0);
	for (n = 0; n <= res; n++) {
		x = R * cos(n * dt); z = R * sin(n * dt);
		vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
	}
	vertices.push_back(R * cos(0) * dt); vertices.push_back(-0.5); vertices.push_back(R * sin(0) * dt);
	for (i = 0; i < vertices.size() / 3; i++) indices.push_back(i);
	Mesh* cono = new Mesh();
	cono->CreateMeshGeometry(vertices, indices, (unsigned int)vertices.size(), res + 2);
	meshList.push_back(cono); // Índice 3
}

void CrearPrismaTriangularEsquina() {
	unsigned int indices[] = { 0, 1, 2, 3, 5, 4, 0, 3, 4, 4, 1, 0, 0, 2, 5, 5, 3, 0, 1, 4, 5, 5, 2, 1 };
	GLfloat vertices[] = {
		0.0f, 0.0f,  0.5f,  1.0f, 0.0f,  0.5f,  0.0f, 1.0f,  0.5f,
		0.0f, 0.0f, -0.5f,  1.0f, 0.0f, -0.5f,  0.0f, 1.0f, -0.5f
	};
	Mesh* prisma = new Mesh();
	prisma->CreateMesh(vertices, indices, 18, 24);
	meshList.push_back(prisma); // Índice 5
}

// -------------------------------------------------------------
// NUEVA FUNCIÓN: Separar las caras para dar color a cada una
// -------------------------------------------------------------
void CrearPiramideCarasMultiColor() {
	GLfloat v[] = {
		 0.5f, -0.5f,  0.5f, // 0
		 0.5f, -0.5f, -0.5f, // 1
		-0.5f, -0.5f, -0.5f, // 2
		-0.5f, -0.5f,  0.5f, // 3
		 0.0f,  0.5f,  0.0f  // 4 (Punta)
	};

	// Cara Frontal (Roja)
	unsigned int ind1[] = { 0, 3, 4 };
	Mesh* m1 = new Mesh(); m1->CreateMesh(v, ind1, 15, 3); meshList.push_back(m1); // Índice 6

	// Cara Izquierda (Verde)
	unsigned int ind2[] = { 3, 2, 4 };
	Mesh* m2 = new Mesh(); m2->CreateMesh(v, ind2, 15, 3); meshList.push_back(m2); // Índice 7

	// Cara Trasera (Amarilla)
	unsigned int ind3[] = { 2, 1, 4 };
	Mesh* m3 = new Mesh(); m3->CreateMesh(v, ind3, 15, 3); meshList.push_back(m3); // Índice 8

	// Cara Derecha (Magenta)
	unsigned int ind4[] = { 1, 0, 4 };
	Mesh* m4 = new Mesh(); m4->CreateMesh(v, ind4, 15, 3); meshList.push_back(m4); // Índice 9

	// Cara Base Cuadrada (Azul)
	unsigned int ind5[] = { 0, 1, 2, 0, 2, 3 };
	Mesh* m5 = new Mesh(); m5->CreateMesh(v, ind5, 15, 6); meshList.push_back(m5); // Índice 10
}

void CreateShaders() {
	Shader* shader1 = new Shader();
	shader1->CreateFromFiles(vShader, fShader);
	shaderList.push_back(*shader1);

	Shader* shader2 = new Shader();
	shader2->CreateFromFiles(vShaderColor, fShader);
	shaderList.push_back(*shader2);
}

int main() {
	mainWindow = Window(800, 600);
	mainWindow.Initialise();

	CrearCubo();
	CrearPiramideTriangular();
	CrearCilindro(5, 1.0f);
	CrearCono(25, 2.0f);
	CrearPiramideCuadrangular();
	CrearPrismaTriangularEsquina();
	CrearPiramideCarasMultiColor(); // Se crean las mallas de los índices 6 al 10

	CreateShaders();

	camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -60.0f, 0.0f, 0.3f, 0.3f);

	GLuint uniformProjection = 0;
	GLuint uniformModel = 0;
	GLuint uniformView = 0;
	GLuint uniformColor = 0;

	glm::mat4 projection = glm::perspective(glm::radians(60.0f), (GLfloat)mainWindow.getBufferWidth() / (GLfloat)mainWindow.getBufferHeight(), 0.1f, 100.0f);

	sp.init();
	sp.load();

	glm::vec3 color = glm::vec3(0.0f, 0.0f, 0.0f);

	while (!mainWindow.getShouldClose())
	{
		GLfloat now = glfwGetTime();
		deltaTime = now - lastTime;
		deltaTime += (now - lastTime) / limitFPS;
		lastTime = now;

		glfwPollEvents();
		camera.keyControl(mainWindow.getsKeys(), deltaTime);
		camera.mouseControl(mainWindow.getXChange(), mainWindow.getYChange());

		// Fondo oscuro tenue para apreciar el alto contraste de los colores
		glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shaderList[0].useShader();
		uniformModel = shaderList[0].getModelLocation();
		uniformProjection = shaderList[0].getProjectLocation();
		uniformView = shaderList[0].getViewLocation();
		uniformColor = shaderList[0].getColorLocation();

		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));

		// --- TRANSFORMACIÓN BASE DE LA ESTRELLA MULTICOLOR ---
		glm::mat4 baseModel = glm::mat4(1.0);
		baseModel = glm::translate(baseModel, glm::vec3(0.0f, 0.0f, -8.0f));
		baseModel = glm::rotate(baseModel, glm::radians(mainWindow.getrotax()), glm::vec3(1.0f, 0.0f, 0.0f));
		baseModel = glm::rotate(baseModel, glm::radians(mainWindow.getrotay()), glm::vec3(0.0f, 1.0f, 0.0f));
		baseModel = glm::rotate(baseModel, glm::radians(mainWindow.getrotaz()), glm::vec3(0.0f, 0.0f, 1.0f));

		// ----------------------------------------------------
		// CICLO PARA CONSTRUIR LAS 8 PIRÁMIDES
		// ----------------------------------------------------
		for (int i = 0; i < 8; i++) {
			glm::mat4 model = baseModel;

			// Calcular los ángulos para apuntar cada pirámide hacia uno de los 8 octantes
			float yaw = 45.0f + (i % 4) * 90.0f;
			float pitch = (i < 4) ? 45.0f : 135.0f;

			model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));

			// Desplazamiento hacia afuera ("Vista Expandida") 
			// Permite ver la base cuadrada azul y apreciar cómo se conectan
			model = glm::translate(model, glm::vec3(0.0f, 0.8f, 0.0f));

			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));

			// A) MODO RELLENO PARA LOS COLORES SÓLIDOS
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			// 1. Frontal (Roja)
			color = glm::vec3(1.0f, 0.0f, 0.0f);
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			meshList[6]->RenderMesh();

			// 2. Izquierda (Verde)
			color = glm::vec3(0.0f, 1.0f, 0.0f);
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			meshList[7]->RenderMesh();

			// 3. Trasera (Amarilla)
			color = glm::vec3(1.0f, 1.0f, 0.0f);
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			meshList[8]->RenderMesh();

			// 4. Derecha (Magenta)
			color = glm::vec3(1.0f, 0.0f, 1.0f);
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			meshList[9]->RenderMesh();

			// 5. Base Cuadrada (Azul)
			color = glm::vec3(0.0f, 0.0f, 1.0f);
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			meshList[10]->RenderMesh();

			// B) MODO ALAMBRE PARA EL CONTORNO (Dibuja la arista negra de la pirámide completa)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			color = glm::vec3(0.0f, 0.0f, 0.0f);
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			meshList[4]->RenderMesh(); // Usa la pirámide completa (índice 4) original
		}

		// Se restaura el modo relleno por defecto
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glUseProgram(0);
		mainWindow.swapBuffers();
	}
	return 0;
}