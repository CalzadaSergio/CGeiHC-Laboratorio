/*
Actividad:
 1.- Nave de carreras que recorre la pista (rectas, curvas, rampas de subida y bajada, salto)
 2.- Satélite que vuela por una trayectoria con sus componentes animados automáticamente
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
#include "Shader_light.h"
#include "Camera.h"
#include "Texture.h"
#include "Model.h"
#include "Skybox.h"

//para iluminación
#include "CommonValues.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Material.h"

#include "PlanetaryRaceTrack.h"

const float toRadians = 3.14159265f / 180.0f;

// --- NAVE DE CARRERAS (sigue la pista) ---
float navePos = 0.0f;           // posición sobre la pista, medida en "muestras" de pathData (0 .. total)
float velNaveBase = 0.30f;      // muestras avanzadas por unidad de deltaTime en plano
float alturaNave = 1.2f;        // qué tan arriba del asfalto flota la nave
float escalaNave = 1.0f;        // ajusta según el tamaño de tu modelo
float ajusteYawNave = 0.0f;     // grados: pon 90, -90 o 180 si tu nave "mira" hacia otro lado

// --- SATÉLITE (trayectoria elíptica + componentes animados) ---
float anguloSatelite = 0.0f;
float velSatelite = 0.35f;      // grados por unidad de deltaTime sobre la órbita
float fasePaneles = 0.0f;       // fase del vaivén de los paneles solares
float velPaneles = 2.0f;
float anguloAntena = 0.0f;      // giro continuo de la antena
float velAntena = 3.0f;
float escalaSatelite = 4.0f;    // el satélite mide pocos metros en Blender; ajusta hasta que se vea bien
float ajusteRumboSatelite = -90.0f; // grados: las alas van sobre Z del modelo, así que se gira 90° para volar "de lado"

// Puntos de giro de cada componente, tal como los lees en Blender (Location del objeto con el origen
// puesto en el punto de giro). Se convierten a OpenGL con (x, y, z) -> (x, z, -y).
// Los modelos se exportan SIN limpiar la ubicación (sin Alt+G): el .obj ya trae la posición real de cada pieza.
glm::vec3 pivotPanelIzqB(-0.31558f, -0.23761f, 1.0f);
glm::vec3 pivotPanelDerB(0.31558f, 0.23761f, 1.0f);
glm::vec3 pivotAntenaB(0.33937f, 0.013013f, 0.99107f);   // centro del aro del plato (coordenadas Global de Blender)
glm::vec3 BlenderAGL(const glm::vec3& b) { return glm::vec3(b.x, b.z, -b.y); }

Window mainWindow;
std::vector<MeshModel*> meshList;
std::vector<Shader> shaderList;

Camera camera;

Texture dirtTexture;            // textura de la pista (cámbiala por una de asfalto cuando tengas una)
Texture pisoTexture;

// --- MODELOS ---
Model Nave_M;
Model SateliteCuerpo_M;
Model SatelitePanelIzq_M;
Model SatelitePanelDer_M;
Model SateliteAntena_M;

Skybox skybox;

//materiales
Material Material_brillante;
Material Material_opaco;

GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;
static double limitFPS = 1.0 / 60.0;

// luces
DirectionalLight mainLight;
PointLight pointLights[MAX_POINT_LIGHTS];
SpotLight spotLights[MAX_SPOT_LIGHTS];

// Shaders
static const char* vShader = "shaders/shader_light.vert";
static const char* fShader = "shaders/shader_light.frag";

void CreateObjects()
{
	// Solo el piso (meshList[0])
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

	MeshModel* piso = new MeshModel();
	piso->CreateMeshModel(floorVertices, floorIndices, 32, 6);
	meshList.push_back(piso);
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

	// La pista necesita el contexto OpenGL ya creado, por eso se construye aquí.
	// Es variable local de main para que sus buffers se liberen antes de cerrar la ventana.
	PlanetaryRaceTrack track;
	track.buildMesh();
	glm::mat4 modeloPista = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, 0.0f));

	camera = Camera(glm::vec3(0.0f, 5.0f, 20.0f), glm::vec3(0.0f, 1.0f, 0.0f), -60.0f, 0.0f, 0.5f, 0.5f);

	dirtTexture = Texture("Textures/dirt.png");
	dirtTexture.LoadTextureA();
	pisoTexture = Texture("Textures/piso.tga");
	pisoTexture.LoadTextureA();

	// --- CARGA DE MODELOS ---
	// Cambia los nombres por los de tus archivos. Si uno no existe, Model::LoadModel solo
	// imprime el error y ese modelo no se dibuja.
	Nave_M = Model();
	Nave_M.LoadModel("Models/Spacechip.obj");

	SateliteCuerpo_M = Model();
	SateliteCuerpo_M.LoadModel("Models/SateliteCuerpo.obj");

	SatelitePanelIzq_M = Model();
	SatelitePanelIzq_M.LoadModel("Models/PanelI.obj");

	SatelitePanelDer_M = Model();
	SatelitePanelDer_M.LoadModel("Models/PanelD.obj");

	SateliteAntena_M = Model();
	SateliteAntena_M.LoadModel("Models/SateliteAntena.obj");

	std::vector<std::string> skyboxFaces;
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_rt.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_lf.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_dn.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_up.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_bk.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_ft.tga");

	skybox = Skybox(skyboxFaces);

	Material_brillante = Material(4.0f, 256);
	Material_opaco = Material(0.3f, 4);

	mainLight = DirectionalLight(1.0f, 1.0f, 1.0f,
		0.3f, 0.3f,
		0.0f, 0.0f, -1.0f);

	unsigned int pointLightCount = 0;
	pointLights[0] = PointLight(1.0f, 0.0f, 0.0f,
		0.0f, 1.0f,
		0.0f, 2.5f, 1.5f,
		0.3f, 0.2f, 0.1f);
	pointLightCount++;

	unsigned int spotLightCount = 0;
	spotLights[0] = SpotLight(1.0f, 1.0f, 1.0f,
		0.0f, 2.0f,
		0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		5.0f);
	spotLightCount++;

	spotLights[1] = SpotLight(0.0f, 0.0f, 1.0f,
		1.0f, 2.0f,
		5.0f, 10.0f, 0.0f,
		0.0f, -5.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		15.0f);
	spotLightCount++;

	GLuint uniformProjection = 0, uniformModel = 0, uniformView = 0, uniformEyePosition = 0,
		uniformSpecularIntensity = 0, uniformShininess = 0, uniformTextureOffset = 0;
	GLuint uniformColor = 0;
	glm::mat4 projection = glm::perspective(45.0f, (GLfloat)mainWindow.getBufferWidth() / mainWindow.getBufferHeight(), 0.1f, 1000.0f);

	glm::vec3 lowerLight(0.0f, 0.0f, 0.0f);
	glm::mat4 model(1.0);
	glm::mat4 modelaux(1.0);
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec2 toffset = glm::vec2(0.0f, 0.0f);

	////Loop mientras no se cierra la ventana
	while (!mainWindow.getShouldClose())
	{
		GLfloat now = glfwGetTime();
		deltaTime = now - lastTime;
		deltaTime += (now - lastTime) / limitFPS;
		lastTime = now;

		// =========================================================================
		// ANIMACIÓN DEL SATÉLITE (trayectoria + componentes)
		// =========================================================================
		anguloSatelite += velSatelite * deltaTime;
		if (anguloSatelite >= 360.0f) anguloSatelite -= 360.0f;

		// Antes los paneles y la antena se movían con teclado; ahora lo hacen solos:
		fasePaneles += velPaneles * deltaTime;     // paneles: vaivén con seno (se usa en el render)
		if (fasePaneles >= 360.0f) fasePaneles -= 360.0f;
		anguloAntena += velAntena * deltaTime;     // antena: giro continuo
		if (anguloAntena >= 360.0f) anguloAntena -= 360.0f;

		//Recibir eventos del usuario
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
		uniformEyePosition = shaderList[0].GetEyePositionLocation();
		uniformColor = shaderList[0].getColorLocation();
		uniformTextureOffset = shaderList[0].getOffsetLocation();

		uniformSpecularIntensity = shaderList[0].GetSpecularIntensityLocation();
		uniformShininess = shaderList[0].GetShininessLocation();

		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));
		glUniform3f(uniformEyePosition, camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);

		lowerLight = camera.getCameraPosition();
		lowerLight.y -= 0.3f;
		spotLights[0].SetFlash(lowerLight, camera.getCameraDirection());

		shaderList[0].SetDirectionalLight(&mainLight);
		shaderList[0].SetPointLights(pointLights, pointLightCount);
		shaderList[0].SetSpotLights(spotLights, spotLightCount);

		// Valores comunes para toda la escena (sin textura animada, color blanco)
		color = glm::vec3(1.0f, 1.0f, 1.0f);
		toffset = glm::vec2(0.0f, 0.0f);
		glUniform2fv(uniformTextureOffset, 1, glm::value_ptr(toffset));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));

		// Renderizado del Piso
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
		model = glm::scale(model, glm::vec3(30.0f, 1.0f, 30.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		pisoTexture.UseTexture();
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[0]->RenderMeshModel();

		// =========================================================================
		// PISTA DE CARRERAS
		// =========================================================================
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modeloPista));
		dirtTexture.UseTexture();
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		track.draw();

		// =========================================================================
		// NAVE DE CARRERAS SOBRE LA PISTA
		// Rectas, curvas, rampas de subida y bajada y salto salen de los puntos de control de la pista:
		// la nave lee pathData (posición, dirección, "arriba" y "derecha" de cada muestra).
		// =========================================================================
		{
			int nMuestras = (int)track.pathData.size();
			if (nMuestras > 1)
			{
				int i0 = ((int)std::floor(navePos)) % nMuestras;
				int i1 = (i0 + 1) % nMuestras;
				float f = navePos - std::floor(navePos);

				const PlanetaryRaceTrack::PathSample& a = track.pathData[i0];
				const PlanetaryRaceTrack::PathSample& b = track.pathData[i1];

				glm::vec3 posN = glm::mix(a.pos, b.pos, f);
				glm::vec3 fwdN = glm::normalize(glm::mix(a.fwd, b.fwd, f));
				glm::vec3 upN = glm::normalize(glm::mix(a.up, b.up, f));
				// Base ortonormal (right = fwd x up, up = right x fwd)
				glm::vec3 rightN = glm::normalize(glm::cross(fwdN, upN));
				upN = glm::normalize(glm::cross(rightN, fwdN));

				// Velocidad según la pendiente: más lenta al subir la rampa, más rápida al bajar
				float factorVel = glm::clamp(1.0f - 0.8f * fwdN.y, 0.6f, 1.5f);
				navePos += velNaveBase * factorVel * deltaTime;
				if (navePos >= (float)nMuestras) navePos -= (float)nMuestras;

				// Orientación: X local = izquierda de la nave, Y = arriba, Z = hacia adelante
				glm::mat4 orient(1.0f);
				orient[0] = glm::vec4(-rightN, 0.0f);
				orient[1] = glm::vec4(upN, 0.0f);
				orient[2] = glm::vec4(fwdN, 0.0f);

				model = modeloPista;
				model = glm::translate(model, posN + upN * alturaNave);
				model = model * orient;
				model = glm::rotate(model, ajusteYawNave * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
				model = glm::scale(model, glm::vec3(escalaNave, escalaNave, escalaNave));

				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				Material_brillante.UseMaterial(uniformSpecularIntensity, uniformShininess);
				Nave_M.RenderModel();
			}
		}

		// =========================================================================
		// SATÉLITE: órbita elíptica inclinada alrededor de la pista
		// Orientación: sigue la tangente de la trayectoria (rumbo + cabeceo).
		// Componentes jerárquicos: cuerpo (padre), 2 paneles solares y antena (hijos).
		// =========================================================================
		{
			float a = anguloSatelite * toRadians;
			glm::vec3 centroOrbita(10.0f, 40.0f, 65.0f);   // centro aproximado de la pista
			float radioX = 110.0f, radioZ = 85.0f, ondaY = 8.0f;

			glm::vec3 posSat(centroOrbita.x + radioX * cos(a),
				centroOrbita.y + ondaY * sin(2.0f * a),
				centroOrbita.z + radioZ * sin(a));
			// Derivada de la trayectoria = dirección de vuelo
			glm::vec3 tanSat(-radioX * sin(a), 2.0f * ondaY * cos(2.0f * a), radioZ * cos(a));
			float rumboSat = atan2(tanSat.x, tanSat.z);
			float cabeceoSat = asin(tanSat.y / glm::length(tanSat));

			model = glm::mat4(1.0);
			model = glm::translate(model, posSat);
			model = glm::rotate(model, rumboSat, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, -cabeceoSat, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, ajusteRumboSatelite * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(escalaSatelite, escalaSatelite, escalaSatelite));
			modelaux = model;

			Material_brillante.UseMaterial(uniformSpecularIntensity, uniformShininess);

			// Cuerpo (padre)
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			SateliteCuerpo_M.RenderModel();

			// Los paneles oscilan como si siguieran al sol (antes: teclado; ahora: seno).
			// Giran alrededor de la dirección de los brazos (eje Y de Blender = eje Z en OpenGL),
			// pasando por su punto de giro: T(pivote) * R * T(-pivote).
			float anguloPanel = 60.0f * sin(fasePaneles * toRadians);
			glm::vec3 pivIzq = BlenderAGL(pivotPanelIzqB);
			glm::vec3 pivDer = BlenderAGL(pivotPanelDerB);

			// Panel izquierdo
			model = modelaux;
			model = glm::translate(model, pivIzq);
			model = glm::rotate(model, anguloPanel * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::translate(model, -pivIzq);
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			SatelitePanelIzq_M.RenderModel();

			// Panel derecho
			model = modelaux;
			model = glm::translate(model, pivDer);
			model = glm::rotate(model, anguloPanel * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::translate(model, -pivDer);
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			SatelitePanelDer_M.RenderModel();

			// Antena con giro continuo alrededor del eje vertical del cuerpo (eje Z de Blender = Y en OpenGL)
			glm::vec3 pivAnt = BlenderAGL(pivotAntenaB);
			model = modelaux;
			model = glm::translate(model, pivAnt);
			model = glm::rotate(model, anguloAntena * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::translate(model, -pivAnt);
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			SateliteAntena_M.RenderModel();
		}

		glUseProgram(0);

		mainWindow.swapBuffers();
	}

	return 0;
}