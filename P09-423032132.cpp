/*
Animación:
Simple o básica: Por banderas y condicionales (más de 1 transformación geométrica se ve modificada)
Compleja: Por medio de funciones y algoritmos.
Textura Animada
*/
//para cargar imagen
#define STB_IMAGE_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>
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
#include "Sphere.h"
#include "Model.h"
#include "Skybox.h"

//para iluminación
#include "CommonValues.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Material.h"
const float toRadians = 3.14159265f / 180.0f;

// --- VARIABLES PARA ANIMACIÓN ---
bool avanza = true;

float toffsetflechau = 0.0f;
float toffsetflechav = 0.0f;
float toffsetnumerou = 0.0f;
float toffsetnumerov = 0.0f;
float toffsetnumerocambiau = 0.0f;
float angulovaria = 0.0f;

// Variables del Rover y Llantas
float movCoche = 0.0f;
float movOffset = 0.5f;         // Velocidad de traslación del Rover
float rotllanta = 0.0f;
float rotllantaOffset = 5.0f;   // Velocidad de giro de las llantas

// ---------- VUELO ACROBÁTICO DEL AVIÓN ----------
float radioAvion = 80.0f;          // Radio del circuito circular base
float tiempoVuelo = 0.0f;          // Segundos reales acumulados
float omegaAvion = 0.2f;           // rad/s del circuito base (v = radio * omega = 16 u/s)
float alturaBase = 25.0f;
float correccionNariz = -90.0f;    // grados: prueba -90, 90, 180 o 0 hasta que el avión mire al frente
const float CICLO_VUELO = 20.0f;   // Duración de la secuencia completa (s)
const float rLoop = 15.0f;         // Radio del looping
const float rTonel = 4.0f;         // Radio del tirabuzón del tonel
const float PI_F = 3.14159265f;

float suave(float x)
{
	x = glm::clamp(x, 0.0f, 1.0f);
	return x * x * (3.0f - 2.0f * x);
}

// ---------- HUMO DE LOS MOTORES ----------
struct Particula
{
	glm::vec3 pos;
	glm::vec3 vel;
	float edad;
	bool activa;
};

const int   MAX_PARTICULAS = 600;
const float VIDA_HUMO = 2.0f;            // El humo desaparece a los 2 s
const float INTERVALO_EMISION = 0.03f;   // Cada cuántos segundos sale humo por motor
Particula humo[MAX_PARTICULAS] = {};
float acumEmision = 0.0f;

// Posición de los motores en el sistema LOCAL del avión (AJÚSTALAS a tu modelo)
// Blender (x, y, z) -> OpenGL (x, z, -y)
glm::vec3 motorIzqLocal(-1.2f, 0.0f, -3.0f);
glm::vec3 motorDerLocal(1.2f, 0.0f, -3.0f);

// ---------- HOLOCRON (animación y apertura) ----------
const float CICLO_HOLO = 12.0f;                       // Duración del ciclo (s)
glm::vec3 posHolocron(25.0f, 5.0f, -10.0f);
float escalaHolocron = 3.0f;
float tiempoHolo = 0.0f;

MeshModel* cuboctaedroHolo = nullptr;   // Centro fijo (6 rombos + 8 triángulos)
MeshModel* esquinaHolo[8] = {};          // 8 pirámides de las esquinas (las que se separan)

// ---------- COHETE: despegue y trayectoria por ecuaciones ----------
enum FaseCohete { FASE_PLATAFORMA, FASE_ASCENSO, FASE_BALISTICO, FASE_ATERRIZAJE, FASE_EN_SUELO, FASE_CIMA };
struct EstadoCohete { glm::vec3 pos; glm::vec3 vel; glm::vec3 orient; float masa; float t; FaseCohete fase; };
EstadoCohete cohete = { glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 1.0f, 0.0f, FASE_PLATAFORMA };

const glm::vec3 PLATAFORMA(-100.0f, -1.5f, -120.0f);   // Posición de la plataforma de lanzamiento
const float G_COHETE = 3.0f;                          // Gravedad (unidades/s^2)
const float EMPUJE = 4.8f;                            // Empuje con masa inicial = 1 (1.6 g)
const float T_COUNTDOWN = 3.0f;                       // Cuenta regresiva en plataforma (s)
const float T_QUEMA = 14.0f;                          // Tiempo de combustión (s)
const float FLUJO_MASA = 0.55f / 14.0f;               // Pierde 55 % de masa durante la quema
const float T_KICK = 4.0f;                            // Instante del "pitch kick" (s de vuelo)
const float ANG_KICK = 0.0f;                          // Inclinación inicial (grados). 0 = vuelo vertical y regresa a la plataforma; >0 curva la trayectoria y aterriza lejos
const float KD = 0.0006f;                             // Coeficiente de arrastre
const float H_ATM = 250.0f;                           // Escala de altura atmosférica
const float T_REINICIO = 120.0f;                      // Tiempo máximo del ciclo (normalmente se reinicia al tocar el suelo)
const float EMPUJE_ATERR = 4.0f;                      // Empuje máximo del motor al aterrizar
const float TILT_MAX = 20.0f;                         // Inclinación máxima del empuje al aterrizar (grados)
const float T_SUELO = 3.0f;                           // Tiempo que se queda en el suelo antes de reiniciar (s)
bool motorCohete = false;                             // Motor encendido (para el humo)
float tSuelo = 0.0f;
const bool COHETE_REGRESA = false;                    
float escalaCohete = 1.0f;
glm::vec3 toberaCoheteLocal(0.0f, -2.0f, 0.0f);       
bool kickAplicado = false;

Window mainWindow;
std::vector<MeshModel*> meshList;
std::vector<Shader> shaderList;

Camera camera;

Texture brickTexture;
Texture dirtTexture;
Texture plainTexture;
Texture pisoTexture;
Texture AgaveTexture;
Texture FlechaTexture;
Texture NumerosTexture;
Texture Numero1Texture;
Texture Numero2Texture;
Texture HumoTexture;
Texture HolocronCaraTexture;

// --- MODELOS ---
Model Rover_M;
Model Llanta_M;
Model Avion_M;
Model Cohete_M;

Skybox skybox;

//materiales
Material Material_brillante;
Material Material_opaco;

GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;
static double limitFPS = 1.0 / 60.0;

// luz direccional
DirectionalLight mainLight;
PointLight pointLights[MAX_POINT_LIGHTS];
SpotLight spotLights[MAX_SPOT_LIGHTS];

// Vertex Shader
static const char* vShader = "shaders/shader_light.vert";

// Fragment Shader
static const char* fShader = "shaders/shader_light.frag";

//cálculo del promedio de las normales para sombreado de Phong
void calcAverageNormals(unsigned int* indices, unsigned int indiceCount, GLfloat* vertices, unsigned int verticeCount,
	unsigned int vLength, unsigned int normalOffset)
{
	for (size_t i = 0; i < indiceCount; i += 3)
	{
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

	for (size_t i = 0; i < verticeCount / vLength; i++)
	{
		unsigned int nOffset = i * vLength + normalOffset;
		glm::vec3 vec(vertices[nOffset], vertices[nOffset + 1], vertices[nOffset + 2]);
		vec = glm::normalize(vec);
		vertices[nOffset] = vec.x; vertices[nOffset + 1] = vec.y; vertices[nOffset + 2] = vec.z;
	}
}

// =============================================================================
// FUNCIÓN DE VUELO ACROBÁTICO: devuelve la matriz del avión en el instante t
// Secuencia (ciclo de 20 s): crucero 0-4, LOOPING 4-9, crucero 9-12,
// TONEL 12-15, crucero 15-20
// =============================================================================
glm::mat4 calcularVueloAvion(float t)
{
	float psi = omegaAvion * t;                 // Ángulo en el circuito base
	float v = radioAvion * omegaAvion;          // Velocidad lineal base

	glm::vec3 pos(radioAvion * cos(psi), alturaBase, radioAvion * sin(psi));
	glm::vec3 T(-sin(psi), 0.0f, cos(psi));     // Tangente (hacia donde vuela)
	glm::vec3 Rv(cos(psi), 0.0f, sin(psi));     // Radial (= eje X local)
	glm::vec3 up(0.0f, 1.0f, 0.0f);

	float pitch = 0.0f, roll = 0.0f;
	float banco = -15.0f * toRadians;           // Inclinación normal de crucero
	float tc = fmod(t, CICLO_VUELO);

	if (tc >= 4.0f && tc < 9.0f)                // ---- LOOPING (5 s) ----
	{
		float u = (tc - 4.0f) / 5.0f;
		float phi = 2.0f * PI_F * u;
		float dphi = 2.0f * PI_F / 5.0f;
		pos += T * (rLoop * sin(phi)) + up * (rLoop * (1.0f - cos(phi)));
		// Ángulo de la velocidad en el plano vertical (cabeceo)
		pitch = atan2(rLoop * dphi * sin(phi), v + rLoop * dphi * cos(phi));
		banco *= (1.0f - sin(PI_F * u));        // Sin banqueo durante el loop
	}
	else if (tc >= 12.0f && tc < 15.0f)         // ---- TONEL / BARREL ROLL (3 s) ----
	{
		float u = (tc - 12.0f) / 3.0f;
		float chi = 2.0f * PI_F * suave(u);
		pos += Rv * (rTonel * sin(chi)) + up * (rTonel * (1.0f - cos(chi)));
		roll = chi;                             // Giro completo sobre su eje
	}

	glm::mat4 m(1.0f);
	m = glm::translate(m, pos);
	m = glm::rotate(m, -psi, glm::vec3(0.0f, 1.0f, 0.0f));          // Rumbo
	m = glm::rotate(m, -pitch, glm::vec3(1.0f, 0.0f, 0.0f));        // Cabeceo (nariz arriba)
	m = glm::rotate(m, roll + banco, glm::vec3(0.0f, 0.0f, 1.0f));  // Alabeo
	m = glm::rotate(m, correccionNariz * toRadians, glm::vec3(0.0f, 1.0f, 0.0f)); // alinea la nariz con el rumbo
	return m;
}

// =============================================================================
// SISTEMA DE PARTÍCULAS DE HUMO
// =============================================================================
void emitirHumo(const glm::vec3& p)
{
	for (int i = 0; i < MAX_PARTICULAS; i++)
	{
		if (!humo[i].activa)
		{
			humo[i].pos = p;
			humo[i].vel = glm::vec3(((rand() % 100) / 100.0f - 0.5f) * 0.4f,
				0.3f,
				((rand() % 100) / 100.0f - 0.5f) * 0.4f);
			humo[i].edad = 0.0f;
			humo[i].activa = true;
			return;
		}
	}
}

void actualizarHumo(float dt, const glm::mat4& modelAvion)
{
	acumEmision += dt;
	while (acumEmision >= INTERVALO_EMISION)
	{
		acumEmision -= INTERVALO_EMISION;
		emitirHumo(glm::vec3(modelAvion * glm::vec4(motorIzqLocal, 1.0f)));
		emitirHumo(glm::vec3(modelAvion * glm::vec4(motorDerLocal, 1.0f)));
	}
	for (int i = 0; i < MAX_PARTICULAS; i++)
	{
		if (!humo[i].activa) continue;
		humo[i].edad += dt;
		if (humo[i].edad >= VIDA_HUMO)
			humo[i].activa = false;
		else
			humo[i].pos += humo[i].vel * dt;
	}
}

// =============================================================================
// HOLOCRON: factor de apertura 0..1 (0 = cerrado, 1 = abierto)
// 0-3 s flota y gira | 3-5 se abre | 5-9 abierto | 9-11 se cierra | 11-12 flota
// =============================================================================
float factorApertura(float t)
{
	float tc = fmod(t, CICLO_HOLO);
	if (tc < 3.0f)  return 0.0f;
	if (tc < 5.0f)  return suave((tc - 3.0f) / 2.0f);
	if (tc < 9.0f)  return 1.0f;
	if (tc < 11.0f) return 1.0f - suave((tc - 9.0f) / 2.0f);
	return 0.0f;
}

// =============================================================================
// COHETE: integración numérica (Euler semi-implícito) de las ecuaciones de
// movimiento:  a = (T/m) * u  -  g * j  -  (kd * rho(y) * |v| / m) * v
//   m(t) = 1 - flujo * t          (masa decreciente durante la quema)
//   rho(y) = exp(-y / H)          (densidad atmosférica)
//   u = v/|v|                     (empuje alineado con la velocidad: giro gravitacional)
// =============================================================================
void reiniciarCohete()
{
	cohete.pos = PLATAFORMA;
	cohete.vel = glm::vec3(0.0f);
	cohete.orient = glm::vec3(0.0f, 1.0f, 0.0f);
	cohete.masa = 1.0f;
	cohete.t = 0.0f;
	cohete.fase = FASE_PLATAFORMA;
	kickAplicado = false;
	motorCohete = false;
	tSuelo = 0.0f;
}

// Gira suavemente la nariz del cohete hacia un vector objetivo (k = rapidez de giro)
void orientarCohete(const glm::vec3& objetivo, float k, float dt)
{
	glm::vec3 o = glm::mix(cohete.orient, glm::normalize(objetivo), 1.0f - exp(-k * dt));
	if (glm::length(o) > 1e-4f) cohete.orient = glm::normalize(o);
}

void actualizarCohete(float dt)
{
	if (cohete.fase == FASE_CIMA) { motorCohete = false; return; }   // Ya llegó a la cima: se queda ahí
	cohete.t += dt;
	if (cohete.t >= T_REINICIO) { reiniciarCohete(); return; }
	glm::vec3 up(0.0f, 1.0f, 0.0f);

	if (cohete.fase == FASE_PLATAFORMA)
	{
		if (cohete.t < T_COUNTDOWN) return;      // Cuenta regresiva
		cohete.fase = FASE_ASCENSO;              // Ignición y despegue
	}
	if (cohete.fase == FASE_EN_SUELO)            // Ya aterrizó: espera y reinicia
	{
		motorCohete = false;
		tSuelo += dt;
		if (tSuelo >= T_SUELO) reiniciarCohete();
		return;
	}

	float tv = cohete.t - T_COUNTDOWN;           // Tiempo de vuelo
	float rapidez = glm::length(cohete.vel);
	float h = cohete.pos.y - PLATAFORMA.y;       // Altura sobre la plataforma
	glm::vec3 acc(0.0f, -G_COHETE, 0.0f);        // Gravedad
	motorCohete = false;

	if (cohete.fase == FASE_ASCENSO)
	{
		if (tv < T_QUEMA)
		{
			cohete.masa = 1.0f - FLUJO_MASA * tv;
			if (!kickAplicado && tv >= T_KICK)   // Pitch kick: inclina la velocidad hacia +X
			{
				glm::mat4 R = glm::rotate(glm::mat4(1.0f), -ANG_KICK * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
				cohete.vel = glm::vec3(R * glm::vec4(cohete.vel, 0.0f));
				kickAplicado = true;
			}
			glm::vec3 dir = (rapidez > 0.5f) ? cohete.vel / rapidez : up;
			acc += dir * (EMPUJE / cohete.masa); // Empuje
			cohete.orient = dir;                 // La nariz sigue la velocidad
			motorCohete = true;
		}
		else
		{
			cohete.fase = FASE_BALISTICO;        // Motor apagado: tiro parabólico
			cohete.masa = 1.0f - FLUJO_MASA * T_QUEMA;
		}
	}
	else if (cohete.fase == FASE_BALISTICO)
	{
		if (!COHETE_REGRESA && cohete.vel.y <= 0.0f)   // Cima de la trayectoria: se detiene
		{
			cohete.vel = glm::vec3(0.0f);
			cohete.fase = FASE_CIMA;
			motorCohete = false;
			return;
		}
		orientarCohete(up, 0.8f, dt);            // Se endereza: motores abajo, nariz arriba
		// Ignición del aterrizaje cuando la distancia de frenado alcanza la altura
		float aFren = 0.8f * (EMPUJE_ATERR / cohete.masa - G_COHETE);
		if (cohete.vel.y < 0.0f && h <= (cohete.vel.y * cohete.vel.y) / (2.0f * aFren) + 2.0f)
			cohete.fase = FASE_ATERRIZAJE;
	}
	else if (cohete.fase == FASE_ATERRIZAJE)
	{
		// Perfil de velocidad deseado: vObj = -sqrt(2 * aFren * h)  (mínimo -1 para tocar suave)
		float aMax = EMPUJE_ATERR / cohete.masa;
		float aFren = 0.8f * (aMax - G_COHETE);
		float vObj = -glm::max(sqrt(2.0f * aFren * glm::max(h, 0.0f)), 1.0f);
		float aUp = glm::clamp(G_COHETE + 3.0f * (vObj - cohete.vel.y), 0.0f, aMax);   // Control P vertical

		// Control PD lateral hacia la plataforma, limitado por la inclinación máxima
		float lim = aUp * tan(TILT_MAX * toRadians);
		float ax = glm::clamp(0.15f * (PLATAFORMA.x - cohete.pos.x) - 1.0f * cohete.vel.x, -lim, lim);
		float az = glm::clamp(0.15f * (PLATAFORMA.z - cohete.pos.z) - 1.0f * cohete.vel.z, -lim, lim);

		glm::vec3 aT(ax, aUp, az);               // Aceleración del empuje
		acc += aT;
		orientarCohete(aT, 4.0f, dt);            // La nariz apunta donde empuja
		motorCohete = true;
	}

	// Arrastre atmosférico
	float rho = exp(-glm::max(h, 0.0f) / H_ATM);
	acc -= (KD * rho * rapidez / cohete.masa) * cohete.vel;

	cohete.vel += acc * dt;                      // v = v + a dt
	cohete.pos += cohete.vel * dt;               // x = x + v dt

	// Toque de suelo
	if (cohete.pos.y <= PLATAFORMA.y && cohete.t > T_COUNTDOWN + 1.0f)
	{
		cohete.pos.y = PLATAFORMA.y;
		cohete.vel = glm::vec3(0.0f);
		cohete.orient = up;                      // Queda parado
		motorCohete = false;
		cohete.fase = FASE_EN_SUELO;
		tSuelo = 0.0f;
	}
}

// Matriz del cohete: posición + orientación de la nariz (cohete.orient)
glm::mat4 matrizCohete()
{
	glm::vec3 up(0.0f, 1.0f, 0.0f);
	glm::vec3 dir = glm::normalize(cohete.orient);
	glm::vec3 p = cohete.pos;
	if (cohete.fase == FASE_PLATAFORMA && cohete.t > T_COUNTDOWN - 2.0f)   // Vibración en la ignición
	{
		p.x += 0.05f * sin(80.0f * cohete.t);
		p.z += 0.05f * cos(70.0f * cohete.t);
	}
	glm::mat4 m = glm::translate(glm::mat4(1.0f), p);
	glm::vec3 eje = glm::cross(up, dir);
	if (glm::length(eje) > 1e-4f)
		m = glm::rotate(m, acos(glm::clamp(glm::dot(up, dir), -1.0f, 1.0f)), glm::normalize(eje));
	return m;
}

// =============================================================================
// MALLAS DEL HOLOCRON
// El cubo = cuboctaedro central (los rombos de cada cara + 8 triángulos)
//         + 8 pirámides (tetraedros) en las esquinas.
// Vértices: x y z | u v | nx ny nz  (sombreado plano)
// =============================================================================
struct MallaTmp { std::vector<GLfloat> v; std::vector<unsigned int> idx; };

// UV de un punto del cubo (|x|,|y|,|z| <= 0.5) sobre la cara 0:+Z 1:-Z 2:+X 3:-X 4:+Y 5:-Y
glm::vec2 uvCaraHolo(int cara, const glm::vec3& p)
{
	switch (cara)
	{
	case 0:  return glm::vec2(p.x + 0.5f, p.y + 0.5f);
	case 1:  return glm::vec2(-p.x + 0.5f, p.y + 0.5f);
	case 2:  return glm::vec2(-p.z + 0.5f, p.y + 0.5f);
	case 3:  return glm::vec2(p.z + 0.5f, p.y + 0.5f);
	case 4:  return glm::vec2(p.x + 0.5f, -p.z + 0.5f);
	default: return glm::vec2(p.x + 0.5f, p.z + 0.5f);
	}
}

// Agrega un triángulo con normal hacia afuera del sólido (centroSolido)
void agregarTri(MallaTmp& m, glm::vec3 a, glm::vec3 b, glm::vec3 c,
	glm::vec2 ua, glm::vec2 ub, glm::vec2 uc, const glm::vec3& centroSolido)
{
	glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
	glm::vec3 g = (a + b + c) / 3.0f;
	if (glm::dot(n, g - centroSolido) < 0.0f) { std::swap(b, c); std::swap(ub, uc); n = -n; }
	unsigned int base = (unsigned int)(m.v.size() / 8);
	glm::vec3 P[3] = { a, b, c };
	glm::vec2 U[3] = { ua, ub, uc };
	for (int i = 0; i < 3; i++)
	{
		m.v.push_back(P[i].x); m.v.push_back(P[i].y); m.v.push_back(P[i].z);
		m.v.push_back(U[i].x); m.v.push_back(U[i].y);
		m.v.push_back(n.x);    m.v.push_back(n.y);    m.v.push_back(n.z);
		m.idx.push_back(base + i);
	}
}

MeshModel* crearMeshDesde(MallaTmp& m)
{
	MeshModel* mesh = new MeshModel();
	mesh->CreateMeshModel(m.v.data(), m.idx.data(), (unsigned int)m.v.size(), (unsigned int)m.idx.size());
	return mesh;
}

void crearMallasHolocron()
{
	const float h = 0.5f;
	const glm::vec2 uvAzul(0.5f, 0.5f);          // Zona azul clara del centro de la textura
	const glm::vec3 origen(0.0f);

	// ---- Cuboctaedro central ----
	MallaTmp cubo;
	glm::vec3 ejes[6] = {
		glm::vec3(0, 0, 1), glm::vec3(0, 0, -1), glm::vec3(1, 0, 0),
		glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, -1, 0) };
	for (int i = 0; i < 6; i++)                    // 6 rombos (uno por cara)
	{
		glm::vec3 n = ejes[i], t1, t2;
		if (fabs(n.z) > 0.5f)      { t1 = glm::vec3(1, 0, 0); t2 = glm::vec3(0, 1, 0); }
		else if (fabs(n.x) > 0.5f) { t1 = glm::vec3(0, 1, 0); t2 = glm::vec3(0, 0, 1); }
		else                       { t1 = glm::vec3(1, 0, 0); t2 = glm::vec3(0, 0, 1); }
		glm::vec3 p0 = n * h + t1 * h, p1 = n * h + t2 * h, p2 = n * h - t1 * h, p3 = n * h - t2 * h;
		agregarTri(cubo, p0, p1, p2, uvCaraHolo(i, p0), uvCaraHolo(i, p1), uvCaraHolo(i, p2), origen);
		agregarTri(cubo, p0, p2, p3, uvCaraHolo(i, p0), uvCaraHolo(i, p2), uvCaraHolo(i, p3), origen);
	}
	for (int c = 0; c < 8; c++)                    // 8 triángulos (uno por esquina)
	{
		float sx = (c & 1) ? 1.0f : -1.0f, sy = (c & 2) ? 1.0f : -1.0f, sz = (c & 4) ? 1.0f : -1.0f;
		glm::vec3 M1(0.0f, sy * h, sz * h), M2(sx * h, 0.0f, sz * h), M3(sx * h, sy * h, 0.0f);
		agregarTri(cubo, M1, M2, M3, uvAzul, uvAzul, uvAzul, origen);
	}
	cuboctaedroHolo = crearMeshDesde(cubo);

	// ---- 8 pirámides de esquina ----
	for (int c = 0; c < 8; c++)
	{
		float sx = (c & 1) ? 1.0f : -1.0f, sy = (c & 2) ? 1.0f : -1.0f, sz = (c & 4) ? 1.0f : -1.0f;
		glm::vec3 C(sx * h, sy * h, sz * h);
		glm::vec3 M1(0.0f, sy * h, sz * h), M2(sx * h, 0.0f, sz * h), M3(sx * h, sy * h, 0.0f);
		glm::vec3 cen = (C + M1 + M2 + M3) / 4.0f;
		int cx = (sx > 0) ? 2 : 3, cy = (sy > 0) ? 4 : 5, cz = (sz > 0) ? 0 : 1;
		MallaTmp t;
		agregarTri(t, C, M1, M2, uvCaraHolo(cz, C), uvCaraHolo(cz, M1), uvCaraHolo(cz, M2), cen);  // triángulo sobre la cara Z
		agregarTri(t, C, M2, M3, uvCaraHolo(cx, C), uvCaraHolo(cx, M2), uvCaraHolo(cx, M3), cen);  // triángulo sobre la cara X
		agregarTri(t, C, M1, M3, uvCaraHolo(cy, C), uvCaraHolo(cy, M1), uvCaraHolo(cy, M3), cen);  // triángulo sobre la cara Y
		agregarTri(t, M1, M2, M3, uvAzul, uvAzul, uvAzul, cen);                                    // cara interior
		esquinaHolo[c] = crearMeshDesde(t);
	}
}

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
	   4, 5, 6,
	   4, 6, 7
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

	unsigned int flechaIndices[] = {
	   0, 1, 2,
	   0, 2, 3,
	};

	GLfloat flechaVertices[] = {
		-0.5f, 0.0f, 0.5f,		0.0f, 0.0f,		0.0f, -1.0f, 0.0f,
		0.5f, 0.0f, 0.5f,		1.0f, 0.0f,		0.0f, -1.0f, 0.0f,
		0.5f, 0.0f, -0.5f,		1.0f, 1.0f,		0.0f, -1.0f, 0.0f,
		-0.5f, 0.0f, -0.5f,		0.0f, 1.0f,		0.0f, -1.0f, 0.0f,
	};

	unsigned int scoreIndices[] = {
	   0, 1, 2,
	   0, 2, 3,
	};

	GLfloat scoreVertices[] = {
		-0.5f, 0.0f, 0.5f,		0.0f, 0.0f,		0.0f, -1.0f, 0.0f,
		0.5f, 0.0f, 0.5f,		1.0f, 0.0f,		0.0f, -1.0f, 0.0f,
		0.5f, 0.0f, -0.5f,		1.0f, 1.0f,		0.0f, -1.0f, 0.0f,
		-0.5f, 0.0f, -0.5f,		0.0f, 1.0f,		0.0f, -1.0f, 0.0f,
	};

	unsigned int numeroIndices[] = {
	   0, 1, 2,
	   0, 2, 3,
	};

	GLfloat numeroVertices[] = {
		-0.5f, 0.0f, 0.5f,		0.0f, 0.67f,	0.0f, -1.0f, 0.0f,
		0.5f, 0.0f, 0.5f,		0.25f, 0.67f,	0.0f, -1.0f, 0.0f,
		0.5f, 0.0f, -0.5f,		0.25f, 1.0f,	0.0f, -1.0f, 0.0f,
		-0.5f, 0.0f, -0.5f,		0.0f, 1.0f,		0.0f, -1.0f, 0.0f,
	};

	// Quad en el plano XY (se orienta hacia la cámara como billboard) para el humo
	unsigned int humoIndices[] = {
	   0, 1, 2,
	   0, 2, 3,
	};

	GLfloat humoVertices[] = {
		-0.5f, -0.5f, 0.0f,		0.0f, 0.0f,		0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.0f,		1.0f, 0.0f,		0.0f, 0.0f, 1.0f,
		0.5f, 0.5f, 0.0f,		1.0f, 1.0f,		0.0f, 0.0f, 1.0f,
		-0.5f, 0.5f, 0.0f,		0.0f, 1.0f,		0.0f, 0.0f, 1.0f,
	};

	MeshModel* obj1 = new MeshModel();
	obj1->CreateMeshModel(vertices, indices, 32, 12);
	meshList.push_back(obj1);

	MeshModel* obj2 = new MeshModel();
	obj2->CreateMeshModel(vertices, indices, 32, 12);
	meshList.push_back(obj2);

	MeshModel* obj3 = new MeshModel();
	obj3->CreateMeshModel(floorVertices, floorIndices, 32, 6);
	meshList.push_back(obj3);

	MeshModel* obj4 = new MeshModel();
	obj4->CreateMeshModel(vegetacionVertices, vegetacionIndices, 64, 12);
	meshList.push_back(obj4);

	MeshModel* obj5 = new MeshModel();
	obj5->CreateMeshModel(flechaVertices, flechaIndices, 32, 6);
	meshList.push_back(obj5);

	MeshModel* obj6 = new MeshModel();
	obj6->CreateMeshModel(scoreVertices, scoreIndices, 32, 6);
	meshList.push_back(obj6);

	MeshModel* obj7 = new MeshModel();
	obj7->CreateMeshModel(numeroVertices, numeroIndices, 32, 6);
	meshList.push_back(obj7);

	MeshModel* obj8 = new MeshModel();
	obj8->CreateMeshModel(humoVertices, humoIndices, 32, 6);
	meshList.push_back(obj8);   // meshList[7] = quad del humo
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
	crearMallasHolocron();
	CreateShaders();

	camera = Camera(glm::vec3(0.0f, 5.0f, 20.0f), glm::vec3(0.0f, 1.0f, 0.0f), -60.0f, 0.0f, 0.5f, 0.5f);

	brickTexture = Texture("Textures/brick.png");
	brickTexture.LoadTextureA();
	dirtTexture = Texture("Textures/dirt.png");
	dirtTexture.LoadTextureA();
	plainTexture = Texture("Textures/plain.png");
	plainTexture.LoadTextureA();
	pisoTexture = Texture("Textures/piso.tga");
	pisoTexture.LoadTextureA();
	AgaveTexture = Texture("Textures/Agave.tga");
	AgaveTexture.LoadTextureA();
	FlechaTexture = Texture("Textures/flechas.tga");
	FlechaTexture.LoadTextureA();
	NumerosTexture = Texture("Textures/numerosbase.tga");
	NumerosTexture.LoadTextureA();
	Numero1Texture = Texture("Textures/numero1.tga");
	Numero1Texture.LoadTextureA();
	Numero2Texture = Texture("Textures/numero2.tga");
	Numero2Texture.LoadTextureA();
	// Textura del humo: círculo blanco difuminado con canal alfa
	HumoTexture = Texture("Textures/humo.png");
	HumoTexture.LoadTextureA();
	// Textura de una cara del holocron (panel azul con marco)
	HolocronCaraTexture = Texture("Textures/holocron_cara.png");
	HolocronCaraTexture.LoadTextureA();

	// --- CARGA DE MODELOS ---
	Rover_M = Model();
	Rover_M.LoadModel("Models/RoberS.obj");

	Llanta_M = Model();
	Llanta_M.LoadModel("Models/LLantaR.obj");

	Avion_M = Model();
	Avion_M.LoadModel("Models/PlaneSam.obj");

	Cohete_M = Model();
	Cohete_M.LoadModel("Models/Saturne5.obj");   // Modelo vertical: nariz hacia +Y
	reiniciarCohete();

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
	glm::mat4 projection = glm::perspective(45.0f, (GLfloat)mainWindow.getBufferWidth() / mainWindow.getBufferHeight(), 0.1f, 3000.0f);

	glm::vec3 lowerLight(0.0f, 0.0f, 0.0f);
	glm::mat4 model(1.0);
	glm::mat4 modelaux(1.0);
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec2 toffset = glm::vec2(0.0f, 0.0f);

	////Loop mientras no se cierra la ventana
	while (!mainWindow.getShouldClose())
	{
		GLfloat now = glfwGetTime();
		GLfloat dtSeg = now - lastTime;          // Segundos REALES (para vuelo y humo)
		if (dtSeg > 0.1f) dtSeg = 0.1f;          // Evita saltos en el primer frame
		deltaTime = now - lastTime;
		deltaTime += (now - lastTime) / limitFPS;
		lastTime = now;

		angulovaria += 0.5f * deltaTime;

		// =========================================================================
		// 1. ANIMACIÓN DEL ROVER (Ciclo infinito de extremo a extremo: -290 a 290)
		// =========================================================================
		if (avanza)
		{
			if (movCoche < 290.0f)
			{
				movCoche += movOffset * deltaTime;
				rotllanta -= rotllantaOffset * deltaTime; // Giro hacia adelante
			}
			else
			{
				avanza = false;
			}
		}
		else
		{
			if (movCoche > -290.0f)
			{
				movCoche -= movOffset * deltaTime;
				rotllanta += rotllantaOffset * deltaTime; // Giro en reversa
			}
			else
			{
				avanza = true;
			}
		}

		// =========================================================================
		// 2. AVIÓN: ACROBACIAS (por función) + HUMO DE MOTORES (partículas)
		// =========================================================================
		tiempoVuelo += dtSeg;
		glm::mat4 modelAvion = calcularVueloAvion(tiempoVuelo);
		actualizarHumo(dtSeg, modelAvion);

		// 3. HOLOCRON Y COHETE
		tiempoHolo += dtSeg;
		actualizarCohete(dtSeg);
		if (motorCohete)   // Humo del cohete por la tobera (reusa las partículas)
			emitirHumo(glm::vec3(matrizCohete() * glm::vec4(toberaCoheteLocal * escalaCohete, 1.0f)));

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

		model = glm::mat4(1.0);
		modelaux = glm::mat4(1.0);
		color = glm::vec3(1.0f, 1.0f, 1.0f);
		toffset = glm::vec2(0.0f, 0.0f);

		// Renderizado del Piso
		glUniform2fv(uniformTextureOffset, 1, glm::value_ptr(toffset));
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
		model = glm::scale(model, glm::vec3(30.0f, 1.0f, 30.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		pisoTexture.UseTexture();
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[2]->RenderMeshModel();

		// =========================================================================
		// RENDERIZADO JERÁRQUICO: ROVER Y SUS 6 LLANTAS (Coordenadas de Blender)
		// =========================================================================
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(movCoche, -1.5f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
		modelaux = model; // Guardamos traslación + escala (0.5) para usar las coordenadas directas de Blender
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		Rover_M.RenderModel();

		// --- LADO 1 (Y negativa en Blender -> Z positiva en OpenGL) ---
		// Llanta 1: Blender (5.7121, -3.0551, 1.7052)
		model = modelaux;
		model = glm::translate(model, glm::vec3(5.7121f, 1.7052f, 3.0551f));
		model = glm::rotate(model, rotllanta * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		Llanta_M.RenderModel();

		// Llanta 2: Blender (-0.9444, -4.159, 1.4119)
		model = modelaux;
		model = glm::translate(model, glm::vec3(-0.9444f, 1.4119f, 4.159f));
		model = glm::rotate(model, rotllanta * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		Llanta_M.RenderModel();

		// Llanta 3: Blender (-5.1308, -4.144, 1.4123)
		model = modelaux;
		model = glm::translate(model, glm::vec3(-5.1308f, 1.4123f, 4.144f));
		model = glm::rotate(model, rotllanta * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		Llanta_M.RenderModel();

		// --- LADO 2 (Y positiva en Blender -> Z negativa en OpenGL) ---
		// Nota: Se rotan 180 grados en Y para que el rin mire hacia afuera, y por eso su giro local se invierte (-rotllanta)
		// Llanta 4: Blender (-5.1131, 4.144, 1.3859)
		model = modelaux;
		model = glm::translate(model, glm::vec3(-5.1131f, 1.3859f, -4.144f));
		model = glm::rotate(model, 180.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, -rotllanta * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		Llanta_M.RenderModel();

		// Llanta 5: Blender (-0.94595, 4.1213, 1.4128)
		model = modelaux;
		model = glm::translate(model, glm::vec3(-0.94595f, 1.4128f, -4.1213f));
		model = glm::rotate(model, 180.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, -rotllanta * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		Llanta_M.RenderModel();

		// Llanta 6: Blender (5.7102, 3.0551, 1.7052)
		model = modelaux;
		model = glm::translate(model, glm::vec3(5.7102f, 1.7052f, -3.0551f));
		model = glm::rotate(model, 180.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, -rotllanta * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		Llanta_M.RenderModel();

		// =========================================================================
		// RENDERIZADO DEL AVIÓN (acrobacias calculadas por función)
		// =========================================================================
		model = modelAvion;
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		Avion_M.RenderModel();

		// =========================================================================
		// RENDERIZADO DEL COHETE (orientado según su vector velocidad)
		// =========================================================================
		model = matrizCohete();
		model = glm::scale(model, glm::vec3(escalaCohete));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(1.0f, 1.0f, 1.0f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Cohete_M.RenderModel();

		// =========================================================================
		// RENDERIZADO DEL HOLOCRON: flota, gira, se abre en 6 caras y se cierra
		// =========================================================================
		{
			float abre = factorApertura(tiempoHolo);
			float pulso = 0.5f + 0.5f * sin(tiempoHolo * 3.0f);

			glm::mat4 baseHolo(1.0f);
			baseHolo = glm::translate(baseHolo, posHolocron + glm::vec3(0.0f, 0.4f * sin(tiempoHolo * 1.5f), 0.0f));
			baseHolo = glm::rotate(baseHolo, tiempoHolo * 30.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			baseHolo = glm::rotate(baseHolo, (15.0f + 5.0f * sin(tiempoHolo)) * toRadians, glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)));
			baseHolo = glm::scale(baseHolo, glm::vec3(escalaHolocron));

			toffset = glm::vec2(0.0f, 0.0f);
			glUniform2fv(uniformTextureOffset, 1, glm::value_ptr(toffset));
			Material_brillante.UseMaterial(uniformSpecularIntensity, uniformShininess);

			color = glm::vec3(0.7f + 0.3f * pulso);
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			HolocronCaraTexture.UseTexture();

			// Centro (cuboctaedro con los rombos): se queda fijo, solo sigue el flote y el giro
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(baseHolo));
			cuboctaedroHolo->RenderMeshModel();

			// 8 pirámides de las esquinas: se separan sobre las diagonales del cubo
			for (int i = 0; i < 8; i++)
			{
				glm::vec3 dir = glm::normalize(glm::vec3((i & 1) ? 1.0f : -1.0f, (i & 2) ? 1.0f : -1.0f, (i & 4) ? 1.0f : -1.0f));
				float sep = abre * (0.9f + 0.1f * sin(tiempoHolo * 2.0f + i));      // Separación
				glm::mat4 mc = glm::rotate(baseHolo, abre * 60.0f * toRadians, dir);  // Gira sobre su propia diagonal
				mc = glm::translate(mc, dir * sep);
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(mc));
				esquinaHolo[i]->RenderMeshModel();
			}
			color = glm::vec3(1.0f, 1.0f, 1.0f);
		}

		// =========================================================================
		// ELEMENTOS ADICIONALES DE LA ESCENA (Agave y Texturas Animadas)
		// =========================================================================
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, 0.5f, -3.0f));
		model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		AgaveTexture.UseTexture();
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[3]->RenderMeshModel();

		// Textura con movimiento (Flecha)
		toffsetflechau += 0.001f;
		toffsetflechav = 0.000f;
		if (toffsetflechau > 1.0f)
			toffsetflechau = 0.0f;
		toffset = glm::vec2(toffsetflechau, toffsetflechav);

		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-2.0f, 1.0f, -6.0f));
		model = glm::rotate(model, 90 * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(3.0f, 3.0f, 3.0f));
		glUniform2fv(uniformTextureOffset, 1, glm::value_ptr(toffset));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(1.0f, 0.0f, 0.0f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		FlechaTexture.UseTexture();
		Material_brillante.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[4]->RenderMeshModel();

		// Plano con todos los números
		toffsetnumerou = 0.0f;
		toffsetnumerov = 0.0f;
		toffset = glm::vec2(toffsetnumerou, toffsetnumerov);
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-6.0f, 2.0f, -6.0f));
		model = glm::rotate(model, 90 * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(3.0f, 3.0f, 3.0f));
		glUniform2fv(uniformTextureOffset, 1, glm::value_ptr(toffset));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(1.0f, 1.0f, 1.0f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		NumerosTexture.UseTexture();
		Material_brillante.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[5]->RenderMeshModel();

		// Número 1
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-10.0f, 2.0f, -6.0f));
		model = glm::rotate(model, 90 * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(3.0f, 3.0f, 3.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(1.0f, 1.0f, 1.0f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		NumerosTexture.UseTexture();
		Material_brillante.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[6]->RenderMeshModel();

		for (int i = 1; i < 4; i++)
		{
			// Números 2-4
			toffsetnumerou += 0.25f;
			toffsetnumerov = 0.0f;
			toffset = glm::vec2(toffsetnumerou, toffsetnumerov);
			model = glm::mat4(1.0);
			model = glm::translate(model, glm::vec3(-10.0f - (i * 3.0f), 2.0f, -6.0f));
			model = glm::rotate(model, 90 * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(3.0f, 3.0f, 3.0f));
			glUniform2fv(uniformTextureOffset, 1, glm::value_ptr(toffset));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			NumerosTexture.UseTexture();
			Material_brillante.UseMaterial(uniformSpecularIntensity, uniformShininess);
			meshList[6]->RenderMeshModel();
		}

		for (int j = 1; j < 5; j++)
		{
			// Números 5-8
			toffsetnumerou += 0.25f;
			toffsetnumerov = -0.33f;
			toffset = glm::vec2(toffsetnumerou, toffsetnumerov);
			model = glm::mat4(1.0);
			model = glm::translate(model, glm::vec3(-7.0f - (j * 3.0f), 5.0f, -6.0f));
			model = glm::rotate(model, 90 * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(3.0f, 3.0f, 3.0f));
			glUniform2fv(uniformTextureOffset, 1, glm::value_ptr(toffset));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			NumerosTexture.UseTexture();
			Material_brillante.UseMaterial(uniformSpecularIntensity, uniformShininess);
			meshList[6]->RenderMeshModel();
		}

		// Número cambiante
		toffsetnumerocambiau += 0.25f;
		if (toffsetnumerocambiau > 1.0f)
			toffsetnumerocambiau = 0.0f;
		toffsetnumerov = 0.0f;
		toffset = glm::vec2(toffsetnumerocambiau, toffsetnumerov);
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-10.0f, 10.0f, -6.0f));
		model = glm::rotate(model, 90 * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(3.0f, 3.0f, 3.0f));
		glUniform2fv(uniformTextureOffset, 1, glm::value_ptr(toffset));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(1.0f, 1.0f, 1.0f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		NumerosTexture.UseTexture();
		Material_brillante.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[6]->RenderMeshModel();

		// Cambiar automáticamente entre textura número 1 y número 2
		toffsetnumerou = 0.0f;
		toffsetnumerov = 0.0f;
		toffset = glm::vec2(toffsetnumerou, toffsetnumerov);
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-13.0f, 10.0f, -6.0f));
		model = glm::rotate(model, 90 * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(3.0f, 3.0f, 3.0f));
		glUniform2fv(uniformTextureOffset, 1, glm::value_ptr(toffset));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		color = glm::vec3(1.0f, 1.0f, 1.0f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Numero1Texture.UseTexture();
		Material_brillante.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[5]->RenderMeshModel();

		// =========================================================================
		// HUMO DE LOS MOTORES (billboards con textura alfa, se dibuja al final)
		// =========================================================================
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);   // El humo no escribe en el z-buffer

		glm::mat4 vista = camera.calculateViewMatrix();
		glm::vec3 camRight(vista[0][0], vista[1][0], vista[2][0]);
		glm::vec3 camUp(vista[0][1], vista[1][1], vista[2][1]);
		glm::vec3 camBack(vista[0][2], vista[1][2], vista[2][2]);

		toffset = glm::vec2(0.0f, 0.0f);
		glUniform2fv(uniformTextureOffset, 1, glm::value_ptr(toffset));
		color = glm::vec3(0.75f, 0.75f, 0.75f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		HumoTexture.UseTexture();
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);

		for (int i = 0; i < MAX_PARTICULAS; i++)
		{
			if (!humo[i].activa) continue;
			float k = humo[i].edad / VIDA_HUMO;                      // 0 -> 1
			float tam = 0.6f + 1.6f * glm::min(k / 0.7f, 1.0f);      // Crece al inicio
			if (k > 0.7f)
				tam *= 1.0f - suave((k - 0.7f) / 0.3f);              // Se encoge y desaparece a los 2 s

			glm::mat4 mB(1.0f);
			mB[0] = glm::vec4(camRight * tam, 0.0f);
			mB[1] = glm::vec4(camUp * tam, 0.0f);
			mB[2] = glm::vec4(camBack, 0.0f);
			mB[3] = glm::vec4(humo[i].pos, 1.0f);
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(mB));
			meshList[7]->RenderMeshModel();
		}
		glDepthMask(GL_TRUE);

		glDisable(GL_BLEND);

		glUseProgram(0);

		mainWindow.swapBuffers();
	}

	return 0;
}