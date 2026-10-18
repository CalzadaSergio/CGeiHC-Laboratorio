/*
Práctica 8: Iluminación - Nave espacial (modelo propio) con 2 motores
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
#include "Shader_light.h"
#include "Camera.h"
#include "Texture.h"
#include "Model.h"
#include "Skybox.h"

#include "CommonValues.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Material.h"

const float toRadians = 3.14159265f / 180.0f;

//  AJUSTES DEL MODELO: edita SOLO este bloque para acomodar tu nave

const char* RUTA_MODELO = "Models/Spacechip.obj"; // tu .obj exportado

// Medidas reales de tu modelo (largo ~13.8 en Z, envergadura ~12.1 en X):
// la NARIZ apunta a +Z y las 2 toberas estan atras, en Z = -7.37
const float ESCALA_NAVE = 0.4f;       // el modelo original es grande; con 0.4 mide ~5.5 de largo
const float ROTACION_NAVE_Y = 180.0f; // 180 = la nariz mira hacia -Z del mundo y los motores hacia la camara
const float ALTURA_FLOTE = 3.5f;      // altura base sobre el piso
const float AMPLITUD_FLOTE = 0.15f;   // cuanto sube y baja (0 = quieta)

// Toberas (extremo trasero del jet)
const glm::vec3 LOCAL_MOTOR1(-0.61f, -0.17f, -7.37f);  // motor 1 (izquierdo)
const glm::vec3 LOCAL_MOTOR2(0.61f, -0.17f, -7.37f);   // motor 2 (derecho)
const glm::vec3 LOCAL_DIR_ESCAPE(0.0f, 0.0f, -1.0f);   // el fuego sale hacia atras (-Z local)

// Luz puntual de la nave 
const glm::vec3 LOCAL_LUZ_CABINA(0.3, 0.05, 0.01);
const glm::vec3 LOCAL_SPOT_POS(0.0f, -0.6f, 6.0f);
const glm::vec3 LOCAL_SPOT_DIR(0.0f, -1.0f, 1.0f);

// Tamaño del fuego en unidades LOCALES del modelo (cada tobera mide ~0.8 de diametro)
const float ANCHO_FUEGO = 0.9f;
const float LARGO_FUEGO = 3.0f;


Window mainWindow;
std::vector<MeshModel*> meshListModel;
std::vector<Shader> shaderList;

Camera camera;

Texture pisoTexture;
Texture PlasmaAzulTexture; // fuego/plasma azul de los reactores

Model NaveSpace; // modelo de la nave

Skybox skybox;

Material Material_brillante;
Material Material_opaco;

GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;
static double limitFPS = 1.0 / 60.0;

// --- VARIABLES DE CONTROL ---
bool lucesNaveEncendidas = true; // Tecla B: PointLight y SpotLight de la nave al mismo tiempo
bool motor1Encendido = false;    // Tecla N: fuego y luz azul del motor 1
bool motor2Encendido = false;    // Tecla M: fuego y luz azul del motor 2

DirectionalLight mainLight;
PointLight pointLights[MAX_POINT_LIGHTS];
SpotLight spotLights[MAX_SPOT_LIGHTS];

static const char* vShader = "shaders/shader_light.vert";
static const char* fShader = "shaders/shader_light.frag";

void calcAverageNormals(unsigned int* indices, unsigned int indiceCount, GLfloat* vertices, unsigned int verticeCount, unsigned int vLength, unsigned int normalOffset) {
	for (size_t i = 0; i < indiceCount; i += 3) {
		unsigned int in0 = indices[i] * vLength;
		unsigned int in1 = indices[i + 1] * vLength;
		unsigned int in2 = indices[i + 2] * vLength;
		glm::vec3 v1(vertices[in1] - vertices[in0], vertices[in1 + 1] - vertices[in0 + 1], vertices[in1 + 2] - vertices[in0 + 2]);
		glm::vec3 v2(vertices[in2] - vertices[in0], vertices[in2 + 1] - vertices[in0 + 1], vertices[in2 + 2] - vertices[in0 + 2]);
		glm::vec3 normal = glm::cross(v1, v2);
		normal = glm::normalize(normal);

		in0 += normalOffset; in1 += normalOffset; in2 += normalOffset;
		vertices[in0] += normal.x; vertices[in0 + 1] += normal.y; vertices[in0 + 2] += normal.z;
		vertices[in1] += normal.x; vertices[in1 + 1] += normal.y; vertices[in1 + 2] += normal.z;
		vertices[in2] += normal.x; vertices[in2 + 1] += normal.y; vertices[in2 + 2] += normal.z;
	}

	for (size_t i = 0; i < verticeCount / vLength; i++) {
		unsigned int nOffset = i * vLength + normalOffset;
		glm::vec3 vec(vertices[nOffset], vertices[nOffset + 1], vertices[nOffset + 2]);
		vec = glm::normalize(vec);
		vertices[nOffset] = vec.x; vertices[nOffset + 1] = vec.y; vertices[nOffset + 2] = vec.z;
	}
}

void CreateObjects() {
	// meshListModel[0]: Piso
	unsigned int floorIndices[] = { 0, 2, 1, 1, 2, 3 };
	GLfloat floorVertices[] = {
		-10.0f, 0.0f, -10.0f,	0.0f, 0.0f,		0.0f, -1.0f, 0.0f,
		10.0f, 0.0f, -10.0f,	10.0f, 0.0f,	0.0f, -1.0f, 0.0f,
		-10.0f, 0.0f, 10.0f,	0.0f, 10.0f,	0.0f, -1.0f, 0.0f,
		10.0f, 0.0f, 10.0f,		10.0f, 10.0f,	0.0f, -1.0f, 0.0f
	};

	// meshListModel[1]: Planos cruzados para el fuego/plasma azul
	unsigned int plasmaIndices[] = { 0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7 };
	GLfloat plasmaVertices[] = {
		-0.5f, -0.5f, 0.0f,		0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		0.5f, 0.5f, 0.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,
		-0.5f, 0.5f, 0.0f,		0.0f, 1.0f,		0.0f, 0.0f, 0.0f,

		0.0f, -0.5f, -0.5f,		0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.5f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.5f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, -0.5f,		0.0f, 1.0f,		0.0f, 0.0f, 0.0f,
	};

	calcAverageNormals(plasmaIndices, 12, plasmaVertices, 64, 8, 5);

	MeshModel* piso = new MeshModel(); piso->CreateMeshModel(floorVertices, floorIndices, 32, 6); meshListModel.push_back(piso);
	MeshModel* mallaPlasma = new MeshModel(); mallaPlasma->CreateMeshModel(plasmaVertices, plasmaIndices, 64, 12); meshListModel.push_back(mallaPlasma);
}

void CreateShaders() {
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

	camera = Camera(glm::vec3(0.0f, 4.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, 0.3f, 0.5f);

	// Texturas
	pisoTexture = Texture("Textures/piso.tga"); pisoTexture.LoadTextureA();
	PlasmaAzulTexture = Texture("Textures/plasma_azul.png"); PlasmaAzulTexture.LoadTextureA();

	// 1.- Importar el modelo de la nave (la textura se carga desde el .mtl)
	NaveSpace = Model(); NaveSpace.LoadModel(RUTA_MODELO);

	std::vector<std::string> skyboxFaces;
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_rt.tga"); skyboxFaces.push_back("Textures/Skybox/cupertin-lake_lf.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_dn.tga"); skyboxFaces.push_back("Textures/Skybox/cupertin-lake_up.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_bk.tga"); skyboxFaces.push_back("Textures/Skybox/cupertin-lake_ft.tga");
	skybox = Skybox(skyboxFaces);

	Material_brillante = Material(4.0f, 256);
	Material_opaco = Material(0.3f, 4);

	// Luz direccional casi apagada (solo ambiente) para apreciar las luces de la nave
	mainLight = DirectionalLight(1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.0f, 0.0f, -1.0f); // ambiente 0.2 para ver la textura

	// 3 PointLights (1 cabina + 2 motores) y 1 SpotLight
	unsigned int pointLightCount = 3;
	unsigned int spotLightCount = 1;

	GLuint uniformProjection = 0, uniformModel = 0, uniformView = 0, uniformEyePosition = 0, uniformSpecularIntensity = 0, uniformShininess = 0;
	GLuint uniformColor = 0;
	glm::mat4 projection = glm::perspective(45.0f, (GLfloat)mainWindow.getBufferWidth() / mainWindow.getBufferHeight(), 0.1f, 1000.0f);

	while (!mainWindow.getShouldClose())
	{
		GLfloat now = glfwGetTime();
		deltaTime = now - lastTime;
		deltaTime += (now - lastTime) / limitFPS;
		lastTime = now;

		glfwPollEvents();
		camera.keyControl(mainWindow.getsKeys(), deltaTime);
		camera.mouseControl(mainWindow.getXChange(), mainWindow.getYChange());

		// 2.- Tecla B: prender/apagar al mismo tiempo la PointLight y la SpotLight de la nave
		if (mainWindow.getsKeys()[GLFW_KEY_B]) {
			lucesNaveEncendidas = !lucesNaveEncendidas;
			mainWindow.getsKeys()[GLFW_KEY_B] = false;
		}

		// 4.- Teclas N y M independientes: 4 estados posibles de los motores
		if (mainWindow.getsKeys()[GLFW_KEY_N]) {
			motor1Encendido = !motor1Encendido;
			mainWindow.getsKeys()[GLFW_KEY_N] = false;
		}
		if (mainWindow.getsKeys()[GLFW_KEY_M]) {
			motor2Encendido = !motor2Encendido;
			mainWindow.getsKeys()[GLFW_KEY_M] = false;
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		skybox.DrawSkybox(camera.calculateViewMatrix(), projection);
		shaderList[0].UseShader();
		uniformModel = shaderList[0].GetModelLocation();
		uniformProjection = shaderList[0].GetProjectionLocation();
		uniformView = shaderList[0].GetViewLocation();
		uniformEyePosition = shaderList[0].GetEyePositionLocation();
		uniformColor = shaderList[0].getColorLocation();
		uniformSpecularIntensity = shaderList[0].GetSpecularIntensityLocation();
		uniformShininess = shaderList[0].GetShininessLocation();

		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));
		glUniform3f(uniformEyePosition, camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);

		// --- POSICION DE LA NAVE (flotando) Y SU MATRIZ ---
		float flote = AMPLITUD_FLOTE * sin(now * 1.5f);
		glm::vec3 posicionNave = glm::vec3(0.0f, ALTURA_FLOTE + flote, 0.0f);
		glm::mat4 rotNave = glm::rotate(glm::mat4(1.0f), ROTACION_NAVE_Y * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 matNave = glm::translate(glm::mat4(1.0f), posicionNave) * rotNave * glm::scale(glm::mat4(1.0f), glm::vec3(ESCALA_NAVE));

		// Convierte un punto local del modelo a coordenadas del mundo
		auto aMundo = [&](const glm::vec3& p) { return glm::vec3(matNave * glm::vec4(p, 1.0f)); };

		glm::vec3 posCabina = aMundo(LOCAL_LUZ_CABINA);
		glm::vec3 posSpotNave = aMundo(LOCAL_SPOT_POS);
		glm::vec3 dirSpotNave = glm::normalize(glm::mat3(rotNave) * LOCAL_SPOT_DIR);
		glm::vec3 posMotor1 = aMundo(LOCAL_MOTOR1);
		glm::vec3 posMotor2 = aMundo(LOCAL_MOTOR2);

		// El fuego sale por la tobera en la direccion LOCAL_DIR_ESCAPE
		float anchoFuego = ANCHO_FUEGO * ESCALA_NAVE;
		float largoFuego = LARGO_FUEGO * ESCALA_NAVE;
		glm::vec3 atras = glm::normalize(glm::mat3(rotNave) * LOCAL_DIR_ESCAPE);
		glm::vec3 centroFuego1 = posMotor1 + atras * (largoFuego * 0.5f);
		glm::vec3 centroFuego2 = posMotor2 + atras * (largoFuego * 0.5f);

		// --- LUCES ---

		// POINT LIGHT 0: luz de la cabina (Tecla B)
		glm::vec3 colorLuzNave = lucesNaveEncendidas ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 0.0f);
		pointLights[0] = PointLight(colorLuzNave.x, colorLuzNave.y, colorLuzNave.z, 0.5f, 1.0f, posCabina.x, posCabina.y, posCabina.z, 0.3f, 0.2f, 0.1f);

		// POINT LIGHT 1: luz azul en la posicion del fuego del motor 1 (Tecla N)
		glm::vec3 colorMotor1 = motor1Encendido ? glm::vec3(0.0f, 0.5f, 1.0f) : glm::vec3(0.0f, 0.0f, 0.0f);
		pointLights[1] = PointLight(colorMotor1.x, colorMotor1.y, colorMotor1.z, 1.0f, 1.5f, centroFuego1.x, centroFuego1.y, centroFuego1.z, 0.3f, 0.2f, 0.1f);

		// POINT LIGHT 2: luz azul en la posicion del fuego del motor 2 (Tecla M)
		glm::vec3 colorMotor2 = motor2Encendido ? glm::vec3(0.0f, 0.5f, 1.0f) : glm::vec3(0.0f, 0.0f, 0.0f);
		pointLights[2] = PointLight(colorMotor2.x, colorMotor2.y, colorMotor2.z, 1.0f, 1.5f, centroFuego2.x, centroFuego2.y, centroFuego2.z, 0.3f, 0.2f, 0.1f);

		// SPOT LIGHT 0: faro frontal de la nave (Tecla B)
		glm::vec3 colorSpotNave = lucesNaveEncendidas ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 0.0f);
		spotLights[0] = SpotLight(colorSpotNave.x, colorSpotNave.y, colorSpotNave.z, 1.0f, 2.0f, posSpotNave.x, posSpotNave.y, posSpotNave.z, dirSpotNave.x, dirSpotNave.y, dirSpotNave.z, 1.0f, 0.0f, 0.0f, 22.0f);

		shaderList[0].SetDirectionalLight(&mainLight);
		shaderList[0].SetPointLights(pointLights, pointLightCount);
		shaderList[0].SetSpotLights(spotLights, spotLightCount);

		// --- RENDERIZADO ---
		glm::mat4 model(1.0);
		glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));

		// Piso
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(30.0f, 1.0f, 30.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		pisoTexture.UseTexture();
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshListModel[0]->RenderMeshModel();

		// Nave flotando
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(matNave));
		Material_brillante.UseMaterial(uniformSpecularIntensity, uniformShininess);
		NaveSpace.RenderModel();

		// 3 y 4.- Fuego/plasma azul de cada motor (con Blending, sin escribir profundidad)
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);

		auto dibujarFuego = [&](const glm::vec3& centro) {
			glm::mat4 m(1.0f);
			m = glm::translate(m, centro);
			m = m * rotNave;                                          // sigue la orientacion de la nave
			m = glm::rotate(m, 90.0f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f)); // tumba los planos a lo largo de Z
			m = glm::scale(m, glm::vec3(anchoFuego, largoFuego, anchoFuego));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(m));
			PlasmaAzulTexture.UseTexture();
			Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
			meshListModel[1]->RenderMeshModel();
			};

		if (motor1Encendido) dibujarFuego(centroFuego1); // Tecla N
		if (motor2Encendido) dibujarFuego(centroFuego2); // Tecla M

		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);

		glUseProgram(0);
		mainWindow.swapBuffers();
	}

	return 0;
}