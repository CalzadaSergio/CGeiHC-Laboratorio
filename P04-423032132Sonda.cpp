/*Práctica 4: Modelado Jerárquico.
Se implementa el uso de matrices adicionales para almacenar información de transformaciones geométricas que se quiere
heredar entre diversas instancias para que estén unidas
Teclas de la F a la L para rotaciones de articulaciones.
N y M para rotar la sonda entera horizontalmente.
U e I para girar la sonda sobre su propio eje.
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
		-0.5f, -0.5f,  0.5f,
		0.5f, -0.5f,  0.5f,
		0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		// back
		-0.5f, -0.5f, -0.5f,
		0.5f, -0.5f, -0.5f,
		0.5f,  0.5f, -0.5f,
		-0.5f,  0.5f, -0.5f
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
	mainWindow = Window(800, 800);
	mainWindow.Initialise();

	CrearCubo();                // índice 0 
	CrearPiramideTriangular();  // índice 1 
	CrearCilindro(18, 1.0f);    // índice 2 
	CrearCono(25, 2.0f);        // índice 3 
	CrearPiramideCuadrangular();// índice 4 
	CreateShaders();

	// Se aleja un poco la cámara para ver bien la sonda
	camera = Camera(glm::vec3(0.0f, 0.0f, 8.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, 0.3f, 0.3f);

	GLuint uniformProjection = 0;
	GLuint uniformModel = 0;
	GLuint uniformView = 0;
	GLuint uniformColor = 0;
	glm::mat4 projection = glm::perspective(glm::radians(60.0f), mainWindow.getBufferWidth() / mainWindow.getBufferHeight(), 0.1f, 100.0f);

	sp.init();
	sp.load();

	glm::mat4 model(1.0);
	glm::mat4 modelRoot(1.0); // Nodo Padre
	glm::mat4 modelAux(1.0);  // Nodos Hijos/Nietos

	glm::vec3 color = glm::vec3(0.0f, 0.0f, 0.0f);

	// Variables para las articulaciones de la sonda
	GLfloat rotPaneles = 0.0f;
	GLfloat rotAntena = 0.0f;
	GLfloat rotBrazo = 0.0f;
	GLfloat orbitaGeneral = 0.0f;
	GLfloat giroPropio = 0.0f; // <-- NUEVA VARIABLE PARA EL GIRO EN EL EJE Z

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
		float velGiro = 10.0f * deltaTime;

		// ===============================================
		// Controles Jerárquicos de la Sonda
		// ===============================================
		// F/G: Paneles Solares
		if (keys[GLFW_KEY_F]) rotPaneles += velGiro;
		if (keys[GLFW_KEY_G]) rotPaneles -= velGiro;
		// H/J: Antena 
		if (keys[GLFW_KEY_H]) rotAntena += velGiro;
		if (keys[GLFW_KEY_J]) rotAntena -= velGiro;
		// K/L: Brazo de instrumentos
		if (keys[GLFW_KEY_K]) rotBrazo += velGiro;
		if (keys[GLFW_KEY_L]) rotBrazo -= velGiro;

		// N/M: Girar la sonda entera horizontalmente (Yaw)
		if (keys[GLFW_KEY_N]) orbitaGeneral += velGiro;
		if (keys[GLFW_KEY_M]) orbitaGeneral -= velGiro;

		// U/I: Girar la sonda sobre su propio eje (Roll/Barril)
		if (keys[GLFW_KEY_U]) giroPropio += velGiro;
		if (keys[GLFW_KEY_I]) giroPropio -= velGiro;

		glClearColor(0.05f, 0.05f, 0.1f, 1.0f); // Color de fondo (espacio exterior)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shaderList[0].useShader();
		uniformModel = shaderList[0].getModelLocation();
		uniformProjection = shaderList[0].getProjectLocation();
		uniformView = shaderList[0].getViewLocation();
		uniformColor = shaderList[0].getColorLocation();

		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));

		// ==========================================
		// 1. NODO PADRE: CUERPO CENTRAL DE LA SONDA
		// ==========================================
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, -4.0f));

		// Rotación horizontal (M/N)
		model = glm::rotate(model, glm::radians(orbitaGeneral), glm::vec3(0.0f, 1.0f, 0.0f));

		// Rotación sobre su propio eje Z (giro espacial U/I)
		model = glm::rotate(model, glm::radians(giroPropio), glm::vec3(0.0f, 0.0f, 1.0f));

		modelRoot = model; // <-- GUARDAMOS EL ESTADO DEL NODO PADRE (Todos heredan de aquí)

		model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f)); // Tamaño del cuerpo principal
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(0.8f, 0.7f, 0.2f); // Color dorado brillante (aislante térmico)
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		meshList[0]->RenderMesh(); // Cubo


		// ==========================================
		// 2. HIJO 1: PANELES SOLARES (Izquierdo y Derecho)
		// ==========================================
		// Panel Izquierdo
		model = modelRoot; // Hereda la posición/rotación del cuerpo central
		model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 0.0f)); // Borde izquierdo
		model = glm::rotate(model, glm::radians(rotPaneles), glm::vec3(1.0f, 0.0f, 0.0f)); // Articulación
		model = glm::translate(model, glm::vec3(-2.0f, 0.0f, 0.0f)); // Distancia desde la bisagra
		model = glm::scale(model, glm::vec3(4.0f, 0.1f, 1.5f)); // Forma del panel
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(0.1f, 0.2f, 0.6f); // Azul oscuro (Celdas solares)
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		meshList[0]->RenderMesh(); // Cubo aplastado

		// Panel Derecho
		model = modelRoot;
		model = glm::translate(model, glm::vec3(1.0f, 0.0f, 0.0f)); // Borde derecho
		model = glm::rotate(model, glm::radians(-rotPaneles), glm::vec3(1.0f, 0.0f, 0.0f)); // Articulación opuesta
		model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f)); // Distancia desde la bisagra
		model = glm::scale(model, glm::vec3(4.0f, 0.1f, 1.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		meshList[0]->RenderMesh();


		// ==========================================
		// 3. HIJO 2: ANTENA DE COMUNICACIÓN (Incluye un Nieto)
		// ==========================================
		// Mástil de la antena (Hijo del Cuerpo)
		model = modelRoot;
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // Se ubica arriba del cubo
		model = glm::rotate(model, glm::radians(rotAntena), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotación (Articulación)
		modelAux = model; // <-- GUARDAMOS PARA EL NIETO (El plato de la antena se mueve con el mástil)

		model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
		model = glm::scale(model, glm::vec3(0.2f, 1.0f, 0.2f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(0.5f, 0.5f, 0.5f); // Gris
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		meshList[2]->RenderMeshGeometry(); // Cilindro

		// Plato de la antena (Nieto del Cuerpo / Hijo del Mástil)
		model = modelAux; // Partimos de la base del mástil
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // Nos movemos a la punta del mástil
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Acostamos el cono
		model = glm::scale(model, glm::vec3(1.0f, 0.3f, 1.0f)); // Aplastamos para simular un plato parabólico
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(0.9f, 0.9f, 0.9f); // Blanco 
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		meshList[3]->RenderMeshGeometry(); // Cono


		// ==========================================
		// 4. HIJO 3: PROPULSOR INFERIOR
		// ==========================================
		model = modelRoot; // Hereda del padre
		model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f)); // Cara inferior
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Invertimos el cono
		model = glm::scale(model, glm::vec3(0.8f, 1.0f, 0.8f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(0.25f, 0.25f, 0.25f); // Gris oscuro
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		meshList[3]->RenderMeshGeometry(); // Cono invertido


		// ==========================================
		// 5. HIJO 4: BRAZO DE INSTRUMENTOS CIENTÍFICOS
		// ==========================================
		// Brazo Extensor (Hijo)
		model = modelRoot;
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 1.0f)); // Cara frontal
		model = glm::rotate(model, glm::radians(rotBrazo), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotación lateral (F->K)
		modelAux = model; // <-- GUARDAMOS PARA EL NIETO (La cámara en la punta)

		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.75f)); // Extensión hacia adelante
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Acostamos el cilindro
		model = glm::scale(model, glm::vec3(0.2f, 1.5f, 0.2f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(0.7f, 0.7f, 0.7f); // Gris metálico
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		meshList[2]->RenderMeshGeometry(); // Cilindro

		// Lente/Sensor en la punta (Nieto del Cuerpo / Hijo del Brazo)
		model = modelAux;
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 1.5f)); // Punta del brazo extensor
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.6f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(0.2f, 0.8f, 0.2f); // Cristal Verde / Instrumento de luz
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		meshList[0]->RenderMesh(); // Cubo

		glUseProgram(0);
		mainWindow.swapBuffers();
	}
	return 0;
}