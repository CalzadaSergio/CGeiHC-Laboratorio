/*
Proyecto final
Escenarios: 1 Espacio | 2 Valle helado | 3 Erid  | 4 Camara Libre | 5 Conducir Nave | 9 Recorrido
*/
//para cargar imagen
#define STB_IMAGE_IMPLEMENTATION

#include <stdio.h>
#include <string>
#include <cmath>
#include <vector>
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
#include "PlanetaryRaceTrack.h"
#include <array>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <initializer_list>
#include "EscenarioDebate.h"   // Escenario del Debate (se dibuja dentro del escenario 3, Erid)
#include "Avatar.h"            // Avatar jerárquico con movimiento (escenario 3)




namespace CicloDia {
// Un dia completo en segundos reales. Ambos planetas comparten este reloj.
double duracionSegundos = 60.0;//AQUI CAMBIAR LA VELOCIDAD DEL CICLO DEL DIAA
struct Estado {
    glm::vec3 haciaSol, colorLuz, tinteCielo;
    float ambiente, difusa, dia, crepusculo;
};
Estado evaluar(double segundos) {
    const double duracion = std::isfinite(duracionSegundos) && duracionSegundos > 0.0
        ? duracionSegundos : 120.0;
    if (!std::isfinite(segundos)) segundos = 0.0;
    const double angulo = 6.283185307179586 * (segundos / duracion + 0.25);
    Estado e;
    e.haciaSol = glm::normalize(glm::vec3(std::cos(angulo), std::sin(angulo), 0.25));
    e.dia = glm::smoothstep(-0.12f, 0.3f, e.haciaSol.y);
    e.crepusculo = (1.0f - glm::smoothstep(0.0f, 0.4f, std::abs(e.haciaSol.y)))
        * glm::smoothstep(-0.2f, 0.0f, e.haciaSol.y);
    e.colorLuz = glm::mix(glm::vec3(0.55f, 0.65f, 1.0f), glm::vec3(1.0f, 0.96f, 0.88f), e.dia);
    e.colorLuz = glm::mix(e.colorLuz, glm::vec3(1.0f, 0.48f, 0.22f), e.crepusculo * 0.8f);
    e.tinteCielo = glm::mix(glm::vec3(0.035f, 0.055f, 0.13f), glm::vec3(1.0f), e.dia);
    e.tinteCielo = glm::mix(e.tinteCielo, glm::vec3(0.65f, 0.27f, 0.22f), e.crepusculo * 0.65f);
    e.ambiente = glm::mix(0.09f, 0.48f, e.dia);
    e.difusa = 0.52f * glm::smoothstep(-0.04f, 0.3f, e.haciaSol.y);
    return e;
}
}


// Luces locales: el sol sigue siendo la unica luz direccional.
namespace LucesNocturnas {
    // 1. Farolas interactivas (Street_light.obj: prenden/apagan con X cuando el avatar esta cerca)
    const unsigned int NUM_FAROLAS = 8;
    const float DISTANCIA_INTERRUPTOR = 12.0f;
    Model StreetLight_M;
    MeshModel panelFarola;
    PointLight lucesFarolas[NUM_FAROLAS];
    PointLight lucesFarolasActivas[MAX_POINT_LIGHTS];
    unsigned int cantidadFarolasActivas = 0;
    bool farolasEncendidas[NUM_FAROLAS] = { true, true, true, true, true, true, true, true };
    bool teclaXAnterior = false;
    // 0..1: Valle Helado | 2..3: Erid (cerca del debate) | 4..7: Erid (recta principal de la pista)
    const int mundoFarola[NUM_FAROLAS] = { 2, 2, 3, 3, 3, 3, 3, 3 };
    const glm::vec3 posicionesFarolas[NUM_FAROLAS] = {
        glm::vec3(-22.0f,  0.0f,  20.0f), glm::vec3(  22.0f,  0.0f,  20.0f),
        glm::vec3(-18.0f, -1.0f,  15.0f), glm::vec3(  18.0f, -1.0f,  15.0f),
        glm::vec3(-165.0f, -1.0f, 126.0f), glm::vec3(-100.0f, -1.0f, 167.0f),
        glm::vec3( -30.0f, -1.0f, 131.0f), glm::vec3(  35.0f, -1.0f, 167.0f)
    };
    const float girosFarolas[NUM_FAROLAS] = {
        0.0f, 180.0f,
        0.0f, 180.0f,
        -90.0f, 90.0f,
        -90.0f, 90.0f
    };
    glm::mat4 matricesFarolas[NUM_FAROLAS];
    // Centro del panel luminoso medido en Street_light.obj (coordenadas locales).
    const glm::vec3 posicionFocoLocal(1.8293905f, 4.964443f, 0.0f);
    const glm::vec3 colorFarola(1.0f, 0.82f, 0.55f);

    // 2. Luces nocturnas automaticas (se prenden al oscurecer y se apagan al amanecer)
    enum IndiceLuzAuto {
        LUZ_EDIFICIO_1 = 0,
        LUZ_EDIFICIO_2,
        LUZ_HOSPITAL,
        LUZ_TORRE_NUCLEO,
        LUZ_TORRE_ALTA,
        LUZ_ANTENA_VALLE,
        LUZ_PODIO_IZQ,
        LUZ_PODIO_DER,
        LUZ_PISTA_CURVA_ESTE,
        LUZ_PISTA_RAMPA,
        LUZ_PISTA_CURVA_OESTE,
        NUM_LUCES_AUTO
    };
    struct ConfigLuzAuto {
        int escena;
        glm::vec3 color;
        float ambiente;
        float difusa;
        float con;
        float lin;
        float exp;
    };
    const ConfigLuzAuto configsAuto[NUM_LUCES_AUTO] = {
        // Escenario 2: Edificios, Torre Hextech y Antena
        { 2, glm::vec3(1.00f, 0.86f, 0.62f), 0.12f, 2.0f, 1.0f, 0.022f, 0.0028f }, // Edificio 1
        { 2, glm::vec3(1.00f, 0.90f, 0.72f), 0.12f, 2.0f, 1.0f, 0.022f, 0.0028f }, // Edificio 2
        { 2, glm::vec3(0.72f, 0.94f, 1.00f), 0.12f, 2.0f, 1.0f, 0.022f, 0.0028f }, // Hospital
        { 2, glm::vec3(0.15f, 0.75f, 1.00f), 0.12f, 2.0f, 1.0f, 0.025f, 0.0035f }, // Nucleo Torre Hextech
        { 2, glm::vec3(0.28f, 0.80f, 1.00f), 0.14f, 2.2f, 1.0f, 0.020f, 0.0025f }, // Parte alta Torre Hextech
        { 2, glm::vec3(1.00f, 0.62f, 0.28f), 0.10f, 1.8f, 1.0f, 0.025f, 0.0035f }, // Antena del valle
        // Escenario 3: Podios del Debate y tramos elevados de la Pista
        { 3, glm::vec3(1.00f, 0.88f, 0.68f), 0.10f, 1.7f, 1.0f, 0.028f, 0.0035f }, // Podio izquierdo
        { 3, glm::vec3(1.00f, 0.88f, 0.68f), 0.10f, 1.7f, 1.0f, 0.028f, 0.0035f }, // Podio derecho
        { 3, glm::vec3(0.25f, 0.82f, 1.00f), 0.12f, 2.4f, 1.0f, 0.018f, 0.0018f }, // Pista curva este elevada
        { 3, glm::vec3(1.00f, 0.68f, 0.25f), 0.12f, 2.4f, 1.0f, 0.018f, 0.0018f }, // Pista rampa de salto
        { 3, glm::vec3(0.45f, 0.75f, 1.00f), 0.12f, 2.4f, 1.0f, 0.018f, 0.0018f }  // Pista curvas oeste
    };
    PointLight lucesAuto[NUM_LUCES_AUTO];
    glm::vec3 posicionesLucesAuto[NUM_LUCES_AUTO] = {
        glm::vec3(-90.0f, 15.0f, -81.0f),
        glm::vec3(  5.0f, 18.0f, -99.0f),
        glm::vec3( 90.0f, 12.2f, -66.0f),
        glm::vec3( 10.5f,  8.6f, -30.0f),
        glm::vec3( 10.5f, 49.5f, -33.5f),
        glm::vec3( 70.0f, 25.5f,  25.0f),
        glm::vec3(-16.0f, 34.9f, -145.7f),
        glm::vec3( 16.0f, 34.9f, -145.7f),
        glm::vec3(162.5f, 49.5f, 362.5f),
        glm::vec3(-62.5f, 72.0f, 487.5f),
        glm::vec3(-275.0f, 22.0f, 337.5f)
    };

    // Actualizacion de posiciones en el mundo a partir de la matriz padre de cada modelo (como en P08320291197.cpp)
    void ActualizarEdificiosValle(const glm::mat4& modelEdificio1, const glm::mat4& modelEdificio2,
        const glm::mat4& modelHospital, const glm::mat4& modelAntena) {
        posicionesLucesAuto[LUZ_EDIFICIO_1]   = glm::vec3(modelEdificio1 * glm::vec4(-1.0732105f, 1.5f, 9.4602285f, 1.0f));
        posicionesLucesAuto[LUZ_EDIFICIO_2]   = glm::vec3(modelEdificio2 * glm::vec4( 0.0095675f, 1.8f, 4.7327020f, 1.0f));
        posicionesLucesAuto[LUZ_HOSPITAL]     = glm::vec3(modelHospital  * glm::vec4(-1.1102450f, 1.5f, 10.4765425f, 1.0f));
        posicionesLucesAuto[LUZ_ANTENA_VALLE] = glm::vec3(modelAntena    * glm::vec4( 0.0f, 140.0f, 0.0f, 1.0f));
    }

    void ActualizarTorreValle(const glm::mat4& modelNucleo, const glm::mat4& modelTorre) {
        posicionesLucesAuto[LUZ_TORRE_NUCLEO] = glm::vec3(modelNucleo * glm::vec4(0.0f, 0.45f,  0.0f, 1.0f));
        posicionesLucesAuto[LUZ_TORRE_ALTA]   = glm::vec3(modelTorre  * glm::vec4(0.0f, 1.80f, -0.6f, 1.0f));
    }

    void ActualizarEscenaErid(const glm::mat4& matrizPista, const glm::mat4& matrizPodioIzq, const glm::mat4& matrizPodioDer) {
        posicionesLucesAuto[LUZ_PODIO_IZQ]         = glm::vec3(matrizPodioIzq * glm::vec4(  0.0f,  7.8f,   1.8f, 1.0f));
        posicionesLucesAuto[LUZ_PODIO_DER]         = glm::vec3(matrizPodioDer * glm::vec4(  0.0f,  7.8f,   1.8f, 1.0f));
        posicionesLucesAuto[LUZ_PISTA_CURVA_ESTE]  = glm::vec3(matrizPista    * glm::vec4(105.0f, 19.0f,  85.0f, 1.0f));
        posicionesLucesAuto[LUZ_PISTA_RAMPA]       = glm::vec3(matrizPista    * glm::vec4( 15.0f, 28.0f, 135.0f, 1.0f));
        posicionesLucesAuto[LUZ_PISTA_CURVA_OESTE] = glm::vec3(matrizPista    * glm::vec4(-70.0f,  8.0f,  75.0f, 1.0f));
    }

    bool IniciarFarolas() {
        StreetLight_M.LoadModel("Models/Street_light.obj");
        if (!StreetLight_M.cargado()) return false;
        for (unsigned int i = 0; i < NUM_FAROLAS; ++i) {
            matricesFarolas[i] = glm::translate(glm::mat4(1.0f), posicionesFarolas[i]);
            matricesFarolas[i] = glm::rotate(matricesFarolas[i], glm::radians(girosFarolas[i]), glm::vec3(0,1,0));
            matricesFarolas[i] = glm::scale(matricesFarolas[i], glm::vec3(2.4f));
        }
        // Mismo panel de cuatro vertices del OBJ, separado solo para la emision.
        GLfloat vertices[] = {
            1.680446f,4.966124f,-.105330f, 0,0, 0,-1,0,
            1.680446f,4.966124f, .105328f, 0,1, 0,-1,0,
            1.978335f,5.018762f, .105328f, 1,1, 0,-1,0,
            1.978335f,5.018762f,-.105330f, 1,0, 0,-1,0
        };
        unsigned int indices[] = { 0,1,2, 0,2,3 };
        panelFarola.CreateMeshModel(vertices, indices, 32, 6);
        return true;
    }

    void ActualizarInterruptores(int escenario, const glm::vec3& posicionAvatar, bool teclaX, bool enNave) {
        bool pulsacion = teclaX && !teclaXAnterior;
        teclaXAnterior = teclaX;
        if (!pulsacion || enNave || (escenario != 2 && escenario != 3)) return;
        int cercana = -1;
        float distanciaMenor = DISTANCIA_INTERRUPTOR;
        for (unsigned int i = 0; i < NUM_FAROLAS; ++i) {
            if (mundoFarola[i] != escenario) continue;
            float distancia = glm::length(posicionAvatar - posicionesFarolas[i]);
            if (distancia <= distanciaMenor) { distanciaMenor = distancia; cercana = i; }
        }
        if (cercana >= 0) farolasEncendidas[cercana] = !farolasEncendidas[cercana];
    }
    // Se encienden gradualmente antes de ponerse el sol y se apagan al salir.
    float intensidad(const CicloDia::Estado& ciclo) {
        return 1.0f - glm::smoothstep(.02f, .22f, ciclo.haciaSol.y);
    }
    const char* glsl = R"GLSL(
uniform bool linternaActiva;
uniform vec3 linternaPos, linternaDir;
uniform vec3 emisionObjeto;
vec3 luzLocal(vec3 p, vec3 n) {
    vec3 resultado = vec3(0);
    if (linternaActiva) {
        vec3 delta = linternaPos-p;
        float d = length(delta);
        vec3 haciaFoco = delta/max(d,.001);
        float cono = smoothstep(cos(radians(25.0)),cos(radians(15.0)),dot(-haciaFoco,linternaDir));
        resultado += vec3(1.0,.96,.84) * cono * (0.12+6.0*abs(dot(n,haciaFoco)))
            / (1.0+.018*d+.0012*d*d);
    }
    return resultado;
}
)GLSL";
    // Ambas escenas comparten el mismo calculo, incluido el shader propio del valle.
    std::string agregarShader(std::string codigo) {
        codigo.insert(codigo.find("void main()"), glsl);
        return codigo;
    }
    void configurar(int escena, const CicloDia::Estado& ciclo, Camera& camara,
        bool linterna, bool ligadaAvatar) {
        GLint programa = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &programa);
        // Como en P08320291197: matriz padre, SetPos y arreglo compacto de luces activas sin huecos.
        cantidadFarolasActivas = 0;
        const float brillo = intensidad(ciclo);
        for (unsigned int i = 0; i < NUM_FAROLAS; ++i) {
            lucesFarolas[i] = PointLight(colorFarola.r, colorFarola.g, colorFarola.b,
                .10f * brillo, 1.8f * brillo, 0,0,0, 1.0f,.025f,.004f);
            lucesFarolas[i].SetPos(glm::vec3(matricesFarolas[i] * glm::vec4(posicionFocoLocal,1.0f)));
            if (mundoFarola[i] == escena && farolasEncendidas[i] && brillo > .001f && cantidadFarolasActivas < MAX_POINT_LIGHTS)
                lucesFarolasActivas[cantidadFarolasActivas++] = lucesFarolas[i];
        }
        for (unsigned int i = 0; i < NUM_LUCES_AUTO; ++i) {
            const auto& cfg = configsAuto[i];
            lucesAuto[i] = PointLight(cfg.color.r, cfg.color.g, cfg.color.b,
                cfg.ambiente * brillo, cfg.difusa * brillo, 0.0f, 0.0f, 0.0f,
                cfg.con, cfg.lin, cfg.exp);
            lucesAuto[i].SetPos(posicionesLucesAuto[i]);
            if (cfg.escena == escena && brillo > .001f && cantidadFarolasActivas < MAX_POINT_LIGHTS)
                lucesFarolasActivas[cantidadFarolasActivas++] = lucesAuto[i];
        }
        const auto direccion = glm::normalize(camara.getCameraDirection());
        const auto origen = ligadaAvatar ? Avatar::pos + glm::vec3(0,4,0) + direccion*1.8f
            : camara.getCameraPosition() + direccion*.5f;
        glUniform1i(glGetUniformLocation(programa,"linternaActiva"),linterna && escena != 1);
        glUniform3fv(glGetUniformLocation(programa,"linternaPos"),1,glm::value_ptr(origen));
        glUniform3fv(glGetUniformLocation(programa,"linternaDir"),1,glm::value_ptr(direccion));
        glUniform3f(glGetUniformLocation(programa,"emisionObjeto"),0,0,0);
    }
    void DibujarFarolas(int escena, const CicloDia::Estado& ciclo, GLint modelo, GLint color) {
        GLint programa = 0; glGetIntegerv(GL_CURRENT_PROGRAM,&programa);
        GLint emision = glGetUniformLocation(programa,"emisionObjeto");
        GLint usarTextura = glGetUniformLocation(programa,"usarTextura");
        for (unsigned int i = 0; i < NUM_FAROLAS; ++i) {
            if (mundoFarola[i] != escena) continue;
            glUniformMatrix4fv(modelo,1,GL_FALSE,glm::value_ptr(matricesFarolas[i]));
            glUniform3f(emision,0,0,0);
            glUniform3f(color,.4f,.45f,.5f);
            glUniform1i(usarTextura,1);
            StreetLight_M.RenderModel();
            glm::vec3 brillo = farolasEncendidas[i] ? colorFarola * intensidad(ciclo)*2.0f : glm::vec3(0);
            glUniform3fv(emision,1,glm::value_ptr(brillo));
            glUniform3f(color,.25f,.25f,.25f);
            glUniform1i(usarTextura,0);
            panelFarola.RenderMeshModel();
        }
        glUniform3f(emision,0,0,0);
        glUniform3f(color,1,1,1);
        glUniform1i(usarTextura,1);
    }

    // El shader del valle recibe los mismos PointLight y uniformes que Shader::SetPointLights.
    const char* puntosValle = R"GLSL(
struct Light { vec3 color; float ambientIntensity; float diffuseIntensity; };
struct PointLight { Light base; vec3 position; float constant; float linear; float exponent; };
uniform int pointLightCount;
uniform PointLight pointLights[16];
vec3 iluminarFarolas(vec3 p, vec3 n) {
    vec3 luz = vec3(0);
    for(int i=0;i<pointLightCount;++i) {
        vec3 delta = pointLights[i].position-p;
        float d=length(delta);
        float difusa=abs(dot(n,delta/max(d,.001)));
        float atenuacion=pointLights[i].constant+pointLights[i].linear*d+pointLights[i].exponent*d*d;
        luz+=pointLights[i].base.color*(pointLights[i].base.ambientIntensity+pointLights[i].base.diffuseIntensity*difusa)/atenuacion;
    }
    return luz;
}
)GLSL";

}

namespace CrepperDatos {
	struct Nodo { const char* nombre; const char* archivo; int padre; glm::vec3 traslacion; };
	const Nodo nodos[] = {
		{ "Crepper_Raiz", nullptr, -1, glm::vec3(0.0f, 0.0f, 0.0f) },
		{ "cuerpo_inferior", "Models/CrepperJerarquizado/cuerpo_inferior.obj", 0, glm::vec3(0.0f, 0.67f, 0.05f) },
		{ "cabeza", "Models/CrepperJerarquizado/cabeza.obj", 1, glm::vec3(0.0f, 0.24f, -1.44f) },
		{ "borde_frontal", "Models/CrepperJerarquizado/borde_frontal.obj", 1, glm::vec3(0.000000000f, 0.000000000f, -1.975000000f) },
		{ "placa_01", "Models/CrepperJerarquizado/placa_01.obj", 1, glm::vec3(0.0f, 0.000000000f, -1.660000000f) },
		{ "placa_02", "Models/CrepperJerarquizado/placa_02.obj", 1, glm::vec3(0.0f, 0.000000000f, -1.010000000f) },
		{ "placa_03", "Models/CrepperJerarquizado/placa_03.obj", 1, glm::vec3(0.000000000f, 0.000000000f, -0.360000000f) },
		{ "placa_04", "Models/CrepperJerarquizado/placa_04.obj", 1, glm::vec3(0.000000000f, 0.000000000f, 0.275000000f) },
		{ "placa_05", "Models/CrepperJerarquizado/placa_05.obj", 1, glm::vec3(0.000000000f, 0.000000000f, 0.875000000f) },
		{ "placa_06", "Models/CrepperJerarquizado/placa_06.obj", 1, glm::vec3(0.000000000f, 0.000000000f, 1.430000000f) },
		{ "placa_07", "Models/CrepperJerarquizado/placa_07.obj", 1, glm::vec3(0.000000000f, 0.000000000f, 1.915000000f) },
		{ "cola", "Models/CrepperJerarquizado/cola.obj", 1, glm::vec3(0.000000000f, 0.000000000f, 1.780000000f) },
		{ "pata_izquierda_1", "Models/CrepperJerarquizado/pata_izquierda_1.obj", 1, glm::vec3(-0.860000000f, -0.099394742f, -1.510781433f) },
		{ "pata_izquierda_2", "Models/CrepperJerarquizado/pata_izquierda_2.obj", 1, glm::vec3(-0.980000000f, -0.099394742f, -1.010781433f) },
		{ "pata_izquierda_3", "Models/CrepperJerarquizado/pata_izquierda_3.obj", 1, glm::vec3(-1.020000000f, -0.099394742f, -0.420781433f) },
		{ "pata_izquierda_4", "Models/CrepperJerarquizado/pata_izquierda_4.obj", 1, glm::vec3(-1.000000000f, -0.099394742f, 0.179218567f) },
		{ "pata_izquierda_5", "Models/CrepperJerarquizado/pata_izquierda_5.obj", 1, glm::vec3(-0.940000000f, -0.099394742f, 0.779218567f) },
		{ "pata_izquierda_6", "Models/CrepperJerarquizado/pata_izquierda_6.obj", 1, glm::vec3(-0.820000000f, -0.099394742f, 1.329218567f) },
		{ "pata_izquierda_7", "Models/CrepperJerarquizado/pata_izquierda_7.obj", 1, glm::vec3(-0.650000000f, -0.099394742f, 1.799218567f) },
		{ "apendice_exterior_izquierda", "Models/CrepperJerarquizado/apendice_exterior_izquierda.obj", 2, glm::vec3(-0.560000000f, -0.435932314f, -0.492195686f) },
		{ "apendice_central_izquierda", "Models/CrepperJerarquizado/apendice_central_izquierda.obj", 2, glm::vec3(-0.230000000f, -0.405409379f, -0.711704545f) },
		{ "ojo_izquierda", "Models/CrepperJerarquizado/ojo_izquierda.obj", 2, glm::vec3(-0.580000000f, 0.280000000f, -0.836000000f) },
		{ "pata_derecha_1", "Models/CrepperJerarquizado/pata_derecha_1.obj", 1, glm::vec3(0.860000000f, -0.099394742f, -1.510781433f) },
		{ "pata_derecha_2", "Models/CrepperJerarquizado/pata_derecha_2.obj", 1, glm::vec3(0.980000000f, -0.099394742f, -1.010781433f) },
		{ "pata_derecha_3", "Models/CrepperJerarquizado/pata_derecha_3.obj", 1, glm::vec3(1.020000000f, -0.099394742f, -0.420781433f) },
		{ "pata_derecha_4", "Models/CrepperJerarquizado/pata_derecha_4.obj", 1, glm::vec3(1.000000000f, -0.099394742f, 0.179218567f) },
		{ "pata_derecha_5", "Models/CrepperJerarquizado/pata_derecha_5.obj", 1, glm::vec3(0.940000000f, -0.099394742f, 0.779218567f) },
		{ "pata_derecha_6", "Models/CrepperJerarquizado/pata_derecha_6.obj", 1, glm::vec3(0.820000000f, -0.099394742f, 1.329218567f) },
		{ "pata_derecha_7", "Models/CrepperJerarquizado/pata_derecha_7.obj", 1, glm::vec3(0.650000000f, -0.099394742f, 1.799218567f) },
		{ "apendice_exterior_derecha", "Models/CrepperJerarquizado/apendice_exterior_derecha.obj", 2, glm::vec3(0.560000000f, -0.424067686f, -0.492195686f) },
		{ "apendice_central_derecha", "Models/CrepperJerarquizado/apendice_central_derecha.obj", 2, glm::vec3(0.230000000f, -0.405409379f, -0.711704545f) },
		{ "ojo_derecha", "Models/CrepperJerarquizado/ojo_derecha.obj", 2, glm::vec3(0.580000000f, 0.280000000f, -0.836000000f) },
	};
	constexpr int cantidad = sizeof(nodos) / sizeof(nodos[0]);
	using Matrices = std::array<glm::mat4, cantidad>;
	// rotaciones: matrices locales inicializadas a identidad. La raiz controla todo el animal.
	Matrices calcularMatrices(const glm::mat4& mundo, const Matrices& rotaciones) {
		Matrices resultado;
		for (int i = 0; i < cantidad; ++i) {
			const glm::mat4 padre = nodos[i].padre < 0 ? mundo : resultado[nodos[i].padre];
			resultado[i] = padre * glm::translate(glm::mat4(1.0f), nodos[i].traslacion) * rotaciones[i];
		}
		return resultado;
	}
} // namespace CrepperDatos


namespace CrepperNPC {
	constexpr double pi = 3.14159265358979323846;
	constexpr float radioRecorrido = 40, velocidad = 6, escala = 3;
	constexpr float limiteCamara = 170;
	struct Pose { glm::vec3 posicion, frente; CrepperDatos::Matrices matrices; };
	Pose evaluar(double tiempo) {
		double angulo = std::fmod(tiempo * velocidad / radioRecorrido, 2 * pi);
		double paso = tiempo * 2 * pi * 1.3;
		glm::vec3 posicion(radioRecorrido * std::sin(angulo), .22f + .035f * std::sin(2 * paso),
			-40 + radioRecorrido * std::cos(angulo));
		glm::vec3 frente(std::cos(angulo), 0, -std::sin(angulo));
		auto mundo = glm::translate(glm::mat4(1), posicion);
		mundo = glm::rotate(mundo, std::atan2(-frente.x, -frente.z), glm::vec3(0, 1, 0));
		mundo = glm::scale(mundo, glm::vec3(escala));
		CrepperDatos::Matrices rotaciones;
		for (int i = 0; i < CrepperDatos::cantidad; ++i) {
			rotaciones[i] = glm::mat4(1);
			const char* nombre = CrepperDatos::nodos[i].nombre;
			if (std::strncmp(nombre, "pata_", 5) == 0) {
				int numero = nombre[std::strlen(nombre) - 1] - '1';
				bool derecha = std::strstr(nombre, "derecha") != nullptr;
				float fase = static_cast<float>(paso - numero * .72 + (derecha ? pi : 0));
				rotaciones[i] = glm::rotate(glm::mat4(1), .18f * std::sin(fase), glm::vec3(1, 0, 0));
				rotaciones[i] = glm::rotate(rotaciones[i], (derecha ? -1 : 1) * .10f * (std::max)(0.f, std::sin(fase)), glm::vec3(0, 0, 1));
			}
			else if (std::strcmp(nombre, "cabeza") == 0)
				rotaciones[i] = glm::rotate(glm::mat4(1), .045f * std::sin(static_cast<float>(tiempo)), glm::vec3(0, 1, 0));
			else if (std::strncmp(nombre, "apendice_", 9) == 0)
				rotaciones[i] = glm::rotate(glm::mat4(1), .07f * std::sin(static_cast<float>(paso) * .5f + i), glm::vec3(1, 0, 0));
			else if (std::strcmp(nombre, "cola") == 0)
				rotaciones[i] = glm::rotate(glm::mat4(1), .04f * std::sin(static_cast<float>(paso)), glm::vec3(0, 1, 0));
		}
		return { posicion,frente,CrepperDatos::calcularMatrices(mundo,rotaciones) };
	}
	glm::vec3 limitar(glm::vec3 p) {
		float radio = glm::length(glm::vec2(p.x, p.z));
		if (radio > limiteCamara) { p.x *= limiteCamara / radio; p.z *= limiteCamara / radio; }
		p.y = std::clamp(p.y, 2.f, 65.f);
		return p;
	}
}


// Escenario 2: modelos de Planeta 1.
namespace ValleHelado {
	Model piezas[CrepperDatos::cantidad];
	Model aurelion;
	Model explorador;
	Model antenaMickey;
	Model edificioValle1;
	Model edificioValle2;
	Model hospitalValle;

	MeshModel terreno;
	Texture piso;
	Skybox cielo;
	Shader shader;
	double tiempo = 0;
	bool estabaActivo = false;
	GLint programa = 0;
	const char* vert = R"GLSL(#version 330 core
layout(location=0) in vec3 pos;
layout(location=1) in vec2 tex;
layout(location=2) in vec3 normal;
uniform mat4 model,view,projection;
out vec2 uv;
out vec3 n;
out vec3 posicionMundo;
void main(){vec4 p=model*vec4(pos,1); uv=tex; posicionMundo=p.xyz;
n=mat3(transpose(inverse(model)))*normal; gl_Position=projection*view*p;})GLSL";
	const char* frag = R"GLSL(#version 330 core
in vec2 uv;
in vec3 n;
in vec3 posicionMundo;
out vec4 salida;
uniform sampler2D texturaPiso;
uniform vec3 tinte;
uniform bool usarTextura;
uniform vec3 haciaSol, colorSolar;
uniform float ambienteSolar, difusaSolar;
void main(){
vec3 norm=normalize(n);
float luz=ambienteSolar+difusaSolar*max(dot(norm,haciaSol),0.0);
vec3 base=(usarTextura?texture(texturaPiso,uv).rgb:vec3(1))*tinte;
salida=vec4(base*(luz*colorSolar+luzLocal(posicionMundo,norm)+iluminarFarolas(posicionMundo,norm))+emisionObjeto,1);
})GLSL";
	bool cargarModelo(Model& destino, std::initializer_list<const char*> rutas) {
		for (const char* ruta : rutas) {
			std::ifstream prueba(ruta, std::ios::binary);
			if (prueba.good()) {
				destino.LoadModel(ruta);
				return destino.cargado();
			}
		}
		printf("No se encontro el modelo requerido: %s\n", *rutas.begin());
		return false;
	}
	glm::mat4 colocar(const Model& m, glm::vec3 posicion, float altura, float giro = 0.f) {
		const auto& a = m.minimo; const auto& b = m.maximo;
		float escala = altura / (std::max)(.001f, b.y - a.y);
		auto matriz = glm::translate(glm::mat4(1), posicion);
		matriz = glm::rotate(matriz, giro, glm::vec3(0, 1, 0));
		matriz = glm::scale(matriz, glm::vec3(escala));
		return glm::translate(matriz, glm::vec3(-(a.x + b.x) * .5f, -a.y, -(a.z + b.z) * .5f));
	}
	bool iniciar() {
		// Antena Mickey: 
		antenaMickey = Model();
		antenaMickey.LoadModel("Models/antenaMickey.obj");

		// Edificios planeta Lol y mickey
		edificioValle1 = Model();
		edificioValle1.LoadModel("Models/EdificioValle1.obj");
		edificioValle2 = Model();
		edificioValle2.LoadModel("Models/EdificioValle2.obj");
		hospitalValle = Model();
		hospitalValle.LoadModel("Models/HospitalValle.obj");


		for (int i = 1; i < CrepperDatos::cantidad; ++i) {
			piezas[i].LoadModel(CrepperDatos::nodos[i].archivo);
			if (!piezas[i].cargado()) return false;
		}

		if (!cargarModelo(aurelion, { "Models/aurelion sol.glb","Models/Equipo/aurelion sol.glb" })) return false;
		if (!cargarModelo(explorador, { "Models/carroexplorador.obj","Models/Equipo/carroexplorador.obj" })) return false;
		const char* rutaPiso = nullptr;
		for (const char* candidata : { "Textures/pisoNieve.jpg","Textures/pisoNieve.jpeg","Textures/Equipo/pisoNieve.jpg" }) {
			std::ifstream prueba(candidata, std::ios::binary);
			if (prueba.good()) { rutaPiso = candidata; break; }
		}
		if (!rutaPiso) {
			printf("No se encontro la textura del piso: Textures/pisoNieve.jpg\n");
			return false;
		}
		piso = Texture(rutaPiso);
		if (!piso.LoadTextureA()) return false;
		piso.UseTexture();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
		if (GLEW_EXT_texture_filter_anisotropic) {
			GLfloat maxAniso = 1.f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
			if (maxAniso > 1.f) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (std::min)(maxAniso, 8.f));
		}
#endif
		glBindTexture(GL_TEXTURE_2D, 0);
		const char* baseCubemap = "Textures/Skybox/valle_helado_cubemap/";
		if (!std::ifstream(std::string(baseCubemap) + "px.png", std::ios::binary).good())
			baseCubemap = "Textures/valle_helado_cubemap/";
		stbi_set_flip_vertically_on_load(false);
		std::vector<std::string> caras;
		for (const char* c : { "px","nx","py","ny","pz","nz" })
			caras.push_back(std::string(baseCubemap) + c + ".png");
		cielo = Skybox(caras);
		stbi_set_flip_vertically_on_load(true);
		auto fragmento = LucesNocturnas::agregarShader(frag);
        fragmento.insert(fragmento.find("void main()"), LucesNocturnas::puntosValle);
		shader.CreateFromString(vert, fragmento.c_str()); shader.UseShader();
		glGetIntegerv(GL_CURRENT_PROGRAM, &programa);
		constexpr float mitadPiso = 1500.f, escalaU = mitadPiso / 40.f, escalaV = mitadPiso / 60.f;
		GLfloat vertices[] = {
			-mitadPiso,0.f,-mitadPiso, -escalaU,-escalaV, 0.f,1.f,0.f,
			 mitadPiso,0.f,-mitadPiso,  escalaU,-escalaV, 0.f,1.f,0.f,
			-mitadPiso,0.f, mitadPiso, -escalaU, escalaV, 0.f,1.f,0.f,
			 mitadPiso,0.f, mitadPiso,  escalaU, escalaV, 0.f,1.f,0.f,
		};
		unsigned indices[] = { 0,2,1, 1,2,3 };
		terreno.CreateMeshModel(vertices, indices, 32, 6);
		return programa != 0;
	}
	void entrar(Camera& camera) {
		camera = Camera({ 50,27,65 }, { 0,1,0 }, -125, -15, 24, .25f);
		glfwSetWindowTitle(glfwGetCurrentContext(), "Valle helado - Crepper NPC | 1 Espacio | 2 Valle | 3 Erid | 4 Libre | 5 Nave");
	}
	void dibujar(Camera& camera, const glm::mat4& projection, double dt, const CicloDia::Estado& ciclo, bool linterna, bool ligadaAvatar) {
		if (!estabaActivo) entrar(camera);
		estabaActivo = true;
		if (std::isfinite(dt) && dt > 0) tiempo += dt;
		auto pos = camera.getCameraPosition(), limitado = CrepperNPC::limitar(pos);
		if (glm::length(pos - limitado) > .0001f) {
			auto f = camera.getCameraDirection();
			camera = Camera(limitado, { 0,1,0 }, glm::degrees(std::atan2(f.z, f.x)), glm::degrees(std::asin(f.y)), 24, .25f);
		}
		glClearColor(.65f, .78f, .87f, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		auto view = camera.calculateViewMatrix(); cielo.DrawSkybox(view, projection, true, ciclo.tinteCielo, ciclo.haciaSol, ciclo.crepusculo);
		shader.UseShader();
        glUniform3fv(glGetUniformLocation(programa, "haciaSol"), 1, glm::value_ptr(ciclo.haciaSol));
        glUniform3fv(glGetUniformLocation(programa, "colorSolar"), 1, glm::value_ptr(ciclo.colorLuz));
        glUniform1f(glGetUniformLocation(programa, "ambienteSolar"), ciclo.ambiente);
        glUniform1f(glGetUniformLocation(programa, "difusaSolar"), ciclo.difusa);
		// Matrices de edificios y antena (calculadas antes de enviar las luces para iluminar tambien el piso)
		glm::mat4 modelAntena = glm::mat4(1.0f);
		modelAntena = glm::translate(modelAntena, glm::vec3(70.0f, 18.545733f, 25.0f));
		modelAntena = glm::rotate(modelAntena, glm::radians(-30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelAntena = glm::scale(modelAntena, glm::vec3(0.05f, 0.05f, 0.05f));

		glm::mat4 modelEdificio1 = glm::mat4(1.0f);
		modelEdificio1 = glm::translate(modelEdificio1, glm::vec3(-90.0f, 0.0f, -105.0f));
		modelEdificio1 = glm::scale(modelEdificio1, glm::vec3(10.0f));
		modelEdificio1 = glm::translate(modelEdificio1, glm::vec3(1.0732105f, 0.007146f, -7.0602285f));

		glm::mat4 modelEdificio2 = glm::mat4(1.0f);
		modelEdificio2 = glm::translate(modelEdificio2, glm::vec3(5.0f, 0.0f, -125.0f));
		modelEdificio2 = glm::scale(modelEdificio2, glm::vec3(10.0f));
		modelEdificio2 = glm::translate(modelEdificio2, glm::vec3(-0.0095675f, 0.004131f, -2.132702f));

		glm::mat4 modelHospital = glm::mat4(1.0f);
		modelHospital = glm::translate(modelHospital, glm::vec3(90.0f, 0.0f, -95.0f));
		modelHospital = glm::scale(modelHospital, glm::vec3(8.0f));
		modelHospital = glm::translate(modelHospital, glm::vec3(1.110245f, 0.029977f, -6.8515425f));

		LucesNocturnas::ActualizarEdificiosValle(modelEdificio1, modelEdificio2, modelHospital, modelAntena);
		LucesNocturnas::configurar(2, ciclo, camera, linterna, ligadaAvatar);
        shader.SetPointLights(LucesNocturnas::lucesFarolasActivas, LucesNocturnas::cantidadFarolasActivas);
		GLint locModel = glGetUniformLocation(programa, "model");
		GLint locTinte = glGetUniformLocation(programa, "tinte");
		GLint locUsarTex = glGetUniformLocation(programa, "usarTextura");
		glUniformMatrix4fv(glGetUniformLocation(programa, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(programa, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniform1i(glGetUniformLocation(programa, "texturaPiso"), 0);


		// Piso nevado
		auto ident = glm::mat4(1);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(ident));
		glUniform3f(locTinte, 1.f, 1.f, 1.f);
		glUniform1i(locUsarTex, 1);
		piso.UseTexture();
		terreno.RenderMeshModel();


		auto mAurelion = colocar(aurelion, { -42.f,24.f,-95.f }, 22.f, static_cast<float>(tiempo * .15));
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(mAurelion));
		aurelion.RenderModel();


		auto mExplorador = colocar(explorador, { 40.f,0.f,-8.f }, 7.5f, -.65f);
		glUniform3f(locTinte, .76f, .82f, .88f);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(mExplorador));
		explorador.RenderModel();


		// Antena valle helado.
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelAntena));
		glUniform3f(locTinte, 1.0f, 1.0f, 1.0f);
		glUniform1i(locUsarTex, 1);
		antenaMickey.RenderModel();

		// Edificio 1
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelEdificio1));
		glUniform3f(locTinte, 1.0f, 1.0f, 1.0f);
		glUniform1i(locUsarTex, 1);
		edificioValle1.RenderModel();

		// Edificio 2:
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelEdificio2));
		glUniform3f(locTinte, 1.0f, 1.0f, 1.0f);
		glUniform1i(locUsarTex, 1);
		edificioValle2.RenderModel();

		// Hospital
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelHospital));
		glUniform3f(locTinte, 1.0f, 1.0f, 1.0f);
		glUniform1i(locUsarTex, 1);
		hospitalValle.RenderModel();

		glUniform1i(locUsarTex, 0);
		LucesNocturnas::DibujarFarolas(2, ciclo, locModel, locTinte);

		// Crepper NPC jerarquizado
		glUniform1i(locUsarTex, 0);
		auto pose = CrepperNPC::evaluar(tiempo);
		for (int i = 1; i < CrepperDatos::cantidad; ++i) {
			const char* nombre = CrepperDatos::nodos[i].nombre;
			glm::vec3 color(.53f, .59f, .58f);
			if (std::strncmp(nombre, "ojo_", 4) == 0) color = { .018f,.025f,.025f };
			else if (std::strcmp(nombre, "cabeza") == 0 || std::strncmp(nombre, "apendice", 8) == 0) color = { .65f,.69f,.67f };
			else if (std::strcmp(nombre, "borde_frontal") == 0) color = { .2f,.28f,.28f };
			glUniform3fv(locTinte, 1, glm::value_ptr(color));
			glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(pose.matrices[i]));
			piezas[i].RenderModel();
		}
		glUseProgram(0);
	}

}


const float toRadians = 3.14159265f / 180.0f;


Window mainWindow;
std::vector<MeshModel*> meshListModel; // recibe xyz uv nx ny nz
std::vector<Shader> shaderList;


Camera camera;


Texture brickTexture;
Texture dirtTexture;
Texture plainTexture;
Texture pisoPlaneta2;
Texture pisoPlaneta1;


// Variables para los planetas 
float rotaPlanetas = 0.0f;
float rotaPlanetasOffset = 0.005f;


float rotaAnillo = 0.0f;
float rotaAnilloOffset = 0.0025f;


float rotaLuna = 0.0f;
float rotaLunaOffset = 0.005f;


// Variables para el satelite
float movSatelite = 0.0f;
float movSateliteOffset = 0.002f;


int estadoSatelite = 0;
float rotacionSatelite = 0.0f;


float rotaAntena = 0.0f;
float rotaAntenaOffset = 0.025f;


float rotaPanelIzq = 0.0f;
float rotaPanelDer = 0.0f;


float rotaPanelIzqOffset = 0.01f;
float rotaPanelDerOffset = 0.01f;


// Variables para rocky
float rotBrazoRocky = 0.0f;
float rotBrazoRockyOffset = 0.1f;
bool saludo = true;

// Variables para el modo de cámara
bool camaraLigadaAvatar = true;
bool camaraEnNave = false;

//animacion plataforma
float anguloCirculoHex = 0.0f;
float anguloSemiAro = 0.0f;

// ----- Lista de Modelos a importar -----


// Planeta 2

Model rocky;
Model brazoRocky;

Model xenonita;

Model rocas1;
Model roca2;
PlanetaryRaceTrack pistaCarreras;

Model Podio_M;
Model Kang_M;
Model Kodos_M;
Model NaveAlien_M;
Texture texEscalones;


Model naveEspacial;
bool focosEncendidos = true;
bool teclaL_presionada = false;

bool rayoEncendido = false;
bool teclaR_presionada = false;
float alturaAbduccion = 0.0f;
float giroAbduccion = 0.0f;

Model Homero_M;


// Configuración de los 4 vehículos (Carril, Velocidad, Inicio, Escala, GiroCorrección, Altura, Color)
PlanetaryRaceTrack::RacerConfig corredores[4] = {
	{ -3.3f, 22.0f,  0.0f, 0.8f, 180.0f, 0.45f, glm::vec3(1.0f, 0.25f, 0.25f) }, // Carril 1 (Izquierda exterior - Rojo)
	{ -1.1f, 25.5f,  8.0f, 0.8f, 180.0f, 0.45f, glm::vec3(0.25f, 0.6f, 1.0f) },  // Carril 2 (Centro izq - Azul)
	{ 1.1f, 23.5f, 16.0f, 0.8f, 180.0f, 0.45f, glm::vec3(0.3f, 1.0f, 0.35f) },  // Carril 3 (Centro der - Verde)
	{ 3.3f, 27.0f, 24.0f, 0.8f, 180.0f, 0.45f, glm::vec3(1.0f, 0.85f, 0.2f) }   // Carril 4 (Derecha exterior - Amarillo)
};


// Modelos del espacio
Model sol;
Model luna;
Model anilloAst;
Model planeta1;
Model planeta2;
Model cuerpoSat;
Model antenaSat;
Model panelDerSat;
Model panelIzqSat;
Model asteroides;


//para la plataforma
Model BasePlataforma_M;
Model NucleoPlataforma_M;
Model CirculoHex_M;
Model TorrePlataforma_M;
Model EsferaPlataforma_M;
Model SemiaroPlataforma_M;
Model AndamioIzqPlataforma_M;
Model AndamioDerPlataforma_M;
//Estacion Espacial
Model EstacionCabina_M;
Model EstacionEstructura_M;
Model EstacionPanelDerecho_M;
Model EstacionPanelIzquierdo_M;
Model EstacionCapsulasDer_M;
Model EstacionCapsulasIzq_M;

Model EstacionPuertaIzq_M;
Model EstacionPuertaDer_M;


// RECORRIDO 4
Model Cohete_M;
float coheteAltura = 0.0f;
float coheteVelocidad = 0.0f;
bool  coheteLanzando = false;
float cicloCohete = 0.0f;
const float CICLO_COHETE = 5.0f;
const float ACELERACION_COHETE = 5.0f;
const float ALTURA_MAX_COHETE = 500.0f;

float tiempoRecorrido4 = 0.0f;
int faseRecorrido4 = 0; // 0 = Valle (cohete), 1 = Espacio (estación)
const float DURACION_FASE1 = 30.0f;
const float DURACION_FASE2 = 25.0f; 
float aperturaPuertas = 0.0f;
float progresoNaveNPC = 0.0f;
const float VELOCIDAD_RECORRIDO4 = 1.0f; // Escala de tiempo en segundos reales.

#include "AnimacionCoheteKF.h"


// Skybox de nuestros escenarios
Skybox skybox1;		// Skybox del espacio
Skybox skybox2;		// Skybox del planeta 1
Skybox skybox3;		// Skybox del planeta 2


int escenarioActual = 1;	// Escenario actual a mostrar (1, 2 o 3)


//materiales
Material Material_brillante;
Material Material_opaco;


GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;
static double limitFPS = 1.0 / 60.0;


// luz direccional
DirectionalLight mainLight;
//para declarar varias luces de tipo pointlight
PointLight pointLights[MAX_POINT_LIGHTS];
SpotLight spotLights[MAX_SPOT_LIGHTS];


// Vertex Shader
static const char* vShader = "shaders/shader_light.vert";


// Fragment Shader
static const char* fShader = "shaders/shader_light.frag";


static const char* TITULO_VENTANA = "Proyecto final | 1 Espacio | 2 Valle helado | 3 Erid | 4 Camara Libre | 5 Nave | 9 Recorrido";


//función de calculo de normales por promedio de vértices 
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
		-10.0f, 0.0f, -10.0f,	0.0f, 0.0f,		0.0f, 1.0f, 0.0f,
		10.0f, 0.0f, -10.0f,	10.0f, 0.0f,	0.0f, 1.0f, 0.0f,
		-10.0f, 0.0f, 10.0f,	0.0f, 10.0f,	0.0f, 1.0f, 0.0f,
		10.0f, 0.0f, 10.0f,		10.0f, 10.0f,	0.0f, 1.0f, 0.0f
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


	calcAverageNormals(indices, 12, vertices, 32, 8, 5);


	calcAverageNormals(vegetacionIndices, 12, vegetacionVertices, 64, 8, 5);


}


void CreateShaders()
{
	Shader* shader1 = new Shader();
	const auto vertices = shader1->ReadFile(vShader);
    auto fragmento = LucesNocturnas::agregarShader(shader1->ReadFile(fShader));
    const std::string direccionAnterior = "FragPos - pLight.position";
    auto direccionPuntual = fragmento.find(direccionAnterior);
    if (direccionPuntual != std::string::npos)
        fragmento.replace(direccionPuntual, direccionAnterior.size(), "pLight.position - FragPos");
    const std::string salida = "color = texture(theTexture, TexCoord)*vColor*finalcolor;";
    const auto posicion = fragmento.find(salida);
    fragmento.replace(posicion, salida.size(),
        "color = texture(theTexture, TexCoord)*vColor*(finalcolor+vec4(luzLocal(FragPos,normalize(Normal)),0)); color.rgb += emisionObjeto;");
    shader1->CreateFromString(vertices.c_str(), fragmento.c_str());
	shaderList.push_back(*shader1);
}


int main()
{
	mainWindow = Window(1366, 768); // 1280, 1024 or 1024, 768
	if (mainWindow.Initialise() != 0) return 1;
	CreateObjects();
	CreateShaders();
	if (!LucesNocturnas::IniciarFarolas()) return 2;
	if (!ValleHelado::iniciar()) return 2;


	camera = Camera(glm::vec3(0.0f, 0.0f, 375.0f), glm::vec3(0.0f, 1.0f, 0.0f), -60.0f, 0.0f, 1.0f, 0.5f);


	brickTexture = Texture("Textures/brick.png");
	brickTexture.LoadTextureA();
	dirtTexture = Texture("Textures/dirt.png");
	dirtTexture.LoadTextureA();
	plainTexture = Texture("Textures/plain.png");
	plainTexture.LoadTextureA();
	pisoPlaneta2 = Texture("Textures/pisoPlaneta2.jpg");
	pisoPlaneta2.LoadTextureA();
	pisoPlaneta1 = Texture("Textures/pisoNieve.jpg");
	pisoPlaneta1.LoadTextureA();
	naveEspacial.LoadModel("Models/naveEspacial.obj");
	Cohete_M.LoadModel("Models/cohete.obj");
	AnimacionCoheteKF::iniciar();
	texEscalones = Texture("Textures/escalones.png");
	texEscalones.LoadTextureA();

	// ----- Carga de modelos -----
	// Rocky
	rocky = Model();
	rocky.LoadModel("Models/rockyPersonaje.obj");


	brazoRocky = Model();
	brazoRocky.LoadModel("Models/rockyBrazo.obj");

	xenonita = Model();
	xenonita.LoadModel("Models/xenonita.obj");

	// Rocas del Planeta 2
	rocas1 = Model();
	rocas1.LoadModel("Models/rocas1.obj");

	roca2 = Model();
	roca2.LoadModel("Models/roca2.obj");

	// Modelos del Debate (ajusta los nombres a tus archivos)
	Podio_M = Model();     Podio_M.LoadModel("Models/podium.obj");
	Kang_M = Model();      Kang_M.LoadModel("Models/alien_1.obj");
	Kodos_M = Model();     Kodos_M.LoadModel("Models/alien_1.obj");
	NaveAlien_M = Model(); NaveAlien_M.LoadModel("Models/UFO.obj");
	Homero_M = Model();
	Homero_M.LoadModel("Models/Milhouse.obj");

	// Nave espacial 1 de la pista de carreras
	// 4 naves de carrera (la 0 es la que conduces con la tecla 5)
	Model navesCarrera[4];
	const char* rutasNaves[4] = {
		"Models/naveEspacial.obj",   // corredor 0 (jugador)
		"Models/Spacechip.obj",          // <-- cambia por tus archivos
		"Models/nave3.obj",
		"Models/nave4.obj"
	};
	// Escala y altura propias de cada modelo (cada uno trae un tamaño distinto)
	float escalaNaves[4] = { 5.0f, 5.0f, 5.0f, 5.0f };
	float alturaNaves[4] = { 0.25f, 0.25f, 0.25f, 0.25f };
	for (int i = 0; i < 4; i++) {
		navesCarrera[i] = Model();
		navesCarrera[i].LoadModel(rutasNaves[i]);
	}

	// Avatar (torso, cola, brazos, cinturón, barras y antena)
	Avatar::iniciar();


	pistaCarreras.invertNormals = false; // false para que las normales apunten hacia +Y y reciban luz solar y nocturna
	pistaCarreras.buildMesh();
	// Modelos del espacio
	sol = Model();
	sol.LoadModel("Models/sol.obj");


	luna = Model();
	luna.LoadModel("Models/luna.obj");


	anilloAst = Model();
	anilloAst.LoadModel("Models/anilloAst.obj");


	planeta1 = Model();
	planeta1.LoadModel("Models/planeta1.obj");


	planeta2 = Model();
	planeta2.LoadModel("Models/planeta2.obj");


	cuerpoSat = Model();
	cuerpoSat.LoadModel("Models/cuerpoSat.obj");


	antenaSat = Model();
	antenaSat.LoadModel("Models/antena.obj");


	panelIzqSat = Model();
	panelIzqSat.LoadModel("Models/panelIzq.obj");


	panelDerSat = Model();
	panelDerSat.LoadModel("Models/panelDer.obj");


	asteroides = Model();
	asteroides.LoadModel("Models/asteroides.obj");


	//PLATAFORMA

	BasePlataforma_M = Model();
	BasePlataforma_M.LoadModel("Models/BasePlataforma.obj");

	TorrePlataforma_M = Model();
	TorrePlataforma_M.LoadModel("Models/TorrePlataforma.obj");

	CirculoHex_M = Model();
	CirculoHex_M.LoadModel("Models/CirculoPlataforma.obj");

	AndamioIzqPlataforma_M = Model();
	AndamioIzqPlataforma_M.LoadModel("Models/AndamioIzqPlataforma.obj");

	AndamioDerPlataforma_M = Model();
	AndamioDerPlataforma_M.LoadModel("Models/AndamioDerPlataforma.obj");


	EsferaPlataforma_M = Model();
	EsferaPlataforma_M.LoadModel("Models/EsferaPlataforma.obj");

	SemiaroPlataforma_M = Model();
	SemiaroPlataforma_M.LoadModel("Models/Semiaro.obj");

	NucleoPlataforma_M = Model();
	NucleoPlataforma_M.LoadModel("Models/NucleoPlataforma.obj");

	//ESTACION ESPACIAL

	EstacionCabina_M = Model();
	EstacionCabina_M.LoadModel("Models/EstacionCabina.obj");

	EstacionEstructura_M = Model();
	EstacionEstructura_M.LoadModel("Models/EstacionEstructura.obj");

	EstacionPanelDerecho_M = Model();
	EstacionPanelDerecho_M.LoadModel("Models/EstacionPanelDerecho.obj");

	EstacionPanelIzquierdo_M = Model();
	EstacionPanelIzquierdo_M.LoadModel("Models/EstacionPanelIzquierdo.obj");

	EstacionCapsulasDer_M = Model();
	EstacionCapsulasDer_M.LoadModel("Models/EstacionCapsulasDer.obj");

	EstacionCapsulasIzq_M = Model();
	EstacionCapsulasIzq_M.LoadModel("Models/EstacionCapsulasIzq.obj");

	EstacionPuertaIzq_M = Model();
	EstacionPuertaIzq_M.LoadModel("Models/EstacionPuertaIzq.obj");

	EstacionPuertaDer_M = Model();
	EstacionPuertaDer_M.LoadModel("Models/EstacionPuertaDer.obj");


	// ----- Configuración Skybox Escenario 1 -----
	std::vector<std::string> skyboxFaces;
	skyboxFaces.push_back("Textures/Skybox/espacio_rt.tga");
	skyboxFaces.push_back("Textures/Skybox/espacio_lf.tga");
	skyboxFaces.push_back("Textures/Skybox/espacio_dn.tga");
	skyboxFaces.push_back("Textures/Skybox/espacio_up.tga");
	skyboxFaces.push_back("Textures/Skybox/espacio_bk.tga");
	skyboxFaces.push_back("Textures/Skybox/espacio_ft.tga");


	skybox1 = Skybox(skyboxFaces);


	// ----- Configuración Skybox Escenario 2 (Planeta 1)-----
	std::vector<std::string> skyboxFaces2;
	skyboxFaces2.push_back("Textures/Skybox/planeta1_rt.tga");
	skyboxFaces2.push_back("Textures/Skybox/planeta1_lf.tga");
	skyboxFaces2.push_back("Textures/Skybox/planeta1_dn.tga");
	skyboxFaces2.push_back("Textures/Skybox/planeta1_up.tga");
	skyboxFaces2.push_back("Textures/Skybox/planeta1_bk.tga");
	skyboxFaces2.push_back("Textures/Skybox/planeta1_ft.tga");
	skybox2 = Skybox(skyboxFaces2);


	// ----- Configuración Skybox Escenario 3 (Planeta 2) -----
	std::vector<std::string> skyboxFaces3;
	skyboxFaces3.push_back("Textures/Skybox/erid_right.jpg");
	skyboxFaces3.push_back("Textures/Skybox/erid_left.jpg");
	skyboxFaces3.push_back("Textures/Skybox/erid_bottom.jpg");
	skyboxFaces3.push_back("Textures/Skybox/erid_top.jpg");
	skyboxFaces3.push_back("Textures/Skybox/erid_back.jpg");
	skyboxFaces3.push_back("Textures/Skybox/erid_front.jpg");
	skybox3 = Skybox(skyboxFaces3);
	Material_brillante = Material(4.0f, 256);
	Material_opaco = Material(0.3f, 4);


	//contador de luces puntuales
	unsigned int pointLightCount = 0;


	//Declaración de primer luz puntual (Modelo del Sol)
	pointLights[0] = PointLight(1.0f, 0.75f, 0.0f,
		5.0f, 5.0f,
		0.0f, 75.0f, -150.0f,
		0.3f, 0.0005f, 0.0007f);
	pointLightCount++;


	//Declaración de segunda luz puntual
	pointLights[1] = PointLight(1.0f, 1.0f, 1.0f,
		0.8f, 0.8f,
		0.0f, 0.0f, 150.0f,
		0.3f, 0.2f, 0.01f);
	pointLightCount++;

	// Luz puntual morada para la nave
	int indicePuntualNave = pointLightCount;
	pointLights[indicePuntualNave] = PointLight(1.0f, 0.0f, 1.0f,
		0.8f, 1.0f,
		0.0f, 0.0f, 0.0f,
		0.3f, 0.2f, 0.1f);
	pointLightCount++;
	unsigned int spotLightCount = 0;


	// Luz puntual del núcleo Hextech de la plataforma (escenario 2)
	int indicePuntualHextech = pointLightCount;
	pointLights[indicePuntualHextech] = PointLight(0.1f, 0.65f, 1.0f,
		0.5f, 1.8f,
		10.5f, 20.5f, -30.0f,
		1.0f, 0.045f, 0.0075f);
	pointLightCount++;

	//linterna (índice 0)
	spotLights[0] = SpotLight(1.0f, 1.0f, 1.0f,
		0.0f, 2.0f,
		0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		5.0f);
	spotLightCount++;


	// Reflectores del debate + rayo abductor (índices 1, 2 y 3) -> spotLightCount = 4
	EscenarioDebate::AgregarSpotLights(spotLights, spotLightCount);

	int indiceFaroNave = spotLightCount;
	spotLights[spotLightCount] = SpotLight(1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,
		0.0f, 2.8f, 0.0f,
		1.0f, 0.0f, 0.0f, // Dirección hacia adelante (+X)
		1.0f, 0.0005f, 0.00019f,
		25.0f); // Apertura del faro
	spotLightCount++;

	GLuint uniformProjection = 0, uniformModel = 0, uniformView = 0, uniformEyePosition = 0,
		uniformSpecularIntensity = 0, uniformShininess = 0;
	GLuint uniformColor = 0;
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (GLfloat)mainWindow.getBufferWidth() / mainWindow.getBufferHeight(), 0.1f, 1000.0f);


	// Matrices y posiciones de las luces para la jerarquia
	glm::mat4 roverMatrix(1.0f);
	glm::mat4 avionMatrix(1.0f);


	glm::vec3 roverPos;
	glm::vec3 roverDir;


	glm::vec3 avionFrentPos;
	glm::vec3 avionFrentDir;


	glm::vec3 avionAbajoPos;
	glm::vec3 avionAbajoDir;


	const double inicioCiclo = glfwGetTime();
	////Loop mientras no se cierra la ventana
	while (!mainWindow.getShouldClose())
	{
		GLfloat now = glfwGetTime();
		const double segundosValle = now - lastTime;
		deltaTime = now - lastTime;
		deltaTime += (now - lastTime) / limitFPS;
		lastTime = now;


		// Tiempo en segundos para las animaciones de la nave y el rayo
		const float tiempoVuelo = (float)glfwGetTime();


		//Recibir eventos del usuario
		glfwPollEvents();
		AnimacionCoheteKF::controlarTeclado(mainWindow.getsKeys());
		AnimacionCoheteKF::animar();

		// --- CONTROL DE TECLADO: LUCES Y ABDUCCIÓN ---
		// Tecla L: Enciende / Apaga los focos del escenario
		if (mainWindow.getsKeys()[GLFW_KEY_L]) {
			if (!teclaL_presionada) {
				focosEncendidos = !focosEncendidos;
				teclaL_presionada = true;
			}
		}
		else {
			teclaL_presionada = false;
		}

		// Tecla R: Activa / Desactiva el rayo abductor
		if (mainWindow.getsKeys()[GLFW_KEY_R]) {
			if (!teclaR_presionada) {
				rayoEncendido = !rayoEncendido;
				teclaR_presionada = true;
			}
		}
		else {
			teclaR_presionada = false;
		}

		// FÍSICA DE ABDUCCIÓN (Sube si el rayo está activo, cae si se apaga)
				// FÍSICA DE ABDUCCIÓN (en segundos reales)
		const float dtSeg = static_cast<float>(segundosValle);
		const float VEL_SUBIDA = 10.0f;    // unidades por segundo (menor = más lento)
		const float VEL_BAJADA = 12.0f;
		const float VEL_GIRO = 1.2f;    // radianes por segundo
		const float ALTURA_TOPE = 65.0f;  // coincide con el tope de EscenarioDebate

		if (rayoEncendido) {
			alturaAbduccion += VEL_SUBIDA * dtSeg;
			giroAbduccion += VEL_GIRO * dtSeg;
			if (alturaAbduccion > ALTURA_TOPE) alturaAbduccion = ALTURA_TOPE;
		}
		else {
			if (alturaAbduccion > 0.0f) {
				alturaAbduccion -= VEL_BAJADA * dtSeg;
				if (alturaAbduccion < 0.0f) alturaAbduccion = 0.0f;
			}
		}

		// Mouse (se lee una sola vez por frame)
		const GLfloat dxMouse = mainWindow.getXChange();
		const GLfloat dyMouse = mainWindow.getYChange();

		// Cámara libre: escenario 1 siempre, y escenarios 2/3 cuando no está ligada al avatar
		const bool camaraLibre = !camaraEnNave && (escenarioActual == 1 || (!camaraLigadaAvatar && escenarioActual != 4));
		if (camaraLibre) {
			camera.keyControl(mainWindow.getsKeys(),
				escenarioActual == 2 ? static_cast<float>(segundosValle) : deltaTime);
			camera.mouseControl(dxMouse, dyMouse);
		}

		// Detección de teclas para cambiar de escenario y reubicar la cámara
		if (mainWindow.getsKeys()[GLFW_KEY_1] && escenarioActual != 1) {
			escenarioActual = 1;
			camaraEnNave = false;
			camera = Camera(glm::vec3(0.0f, 0.0f, 375.0f), glm::vec3(0.0f, 1.0f, 0.0f), -60.0f, 0.0f, 2.5f, 0.5f);
			glfwSetWindowTitle(glfwGetCurrentContext(), TITULO_VENTANA);
		}
		if (mainWindow.getsKeys()[GLFW_KEY_2] && escenarioActual != 2) {
			escenarioActual = 2;
			camaraEnNave = false;
			camaraLigadaAvatar = true;
			camera = Camera(glm::vec3(0.0f, 0.0f, 50.0f), glm::vec3(0.0f, 1.0f, 0.0f), -60.0f, 0.0f, 1.5f, 0.5f);
		}
		if (mainWindow.getsKeys()[GLFW_KEY_3] && (escenarioActual != 3 || camaraEnNave)) {
			escenarioActual = 3;
			camaraEnNave = false;
			camaraLigadaAvatar = true;
			// Yaw -90 mira hacia -Z, donde está el estrado del debate
			camera = Camera(glm::vec3(0.0f, 5.0f, 50.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, 1.5f, 0.5f);
			glfwSetWindowTitle(glfwGetCurrentContext(), TITULO_VENTANA);
		}

		// Tecla 4: Cámara libre
		if (mainWindow.getsKeys()[GLFW_KEY_4]) {
			if (camaraEnNave || camaraLigadaAvatar) {
				glm::vec3 p = camera.getCameraPosition();
				glm::vec3 f = camera.getCameraDirection();
				camera = Camera(p, glm::vec3(0.0f, 1.0f, 0.0f),
					glm::degrees(std::atan2(f.z, f.x)),
					glm::degrees(std::asin(std::clamp(f.y, -0.99f, 0.99f))),
					1.5f, 0.5f);
			}
			camaraLigadaAvatar = false;
			camaraEnNave = false;
			glfwSetWindowTitle(glfwGetCurrentContext(), "Camara Libre | 1 Espacio | 2 Valle | 3 Erid (Avatar) | 4 Libre | 5 Nave");
		}

		// Tecla 5: Cámara en 3ra persona conduciendo la nave espacial en la pista
		if (mainWindow.getsKeys()[GLFW_KEY_5]) {
			escenarioActual = 3;
			camaraLigadaAvatar = false;
			camaraEnNave = true;
			glfwSetWindowTitle(glfwGetCurrentContext(), "Modo Conduccion Nave 3ra Persona (A/D cambiar carril) | 3 Avatar | 4 Libre | 5 Nave");
		}
		//Tecla 9 para Recorrido 4 ----
		if (mainWindow.getsKeys()[GLFW_KEY_9] && escenarioActual != 4) {
			escenarioActual = 4;
            tiempoRecorrido4 = 0.0f; faseRecorrido4 = 0;
            cicloCohete = 0.0f; coheteAltura = 0.0f; coheteVelocidad = 0.0f;
            coheteLanzando = false; aperturaPuertas = 0.0f; progresoNaveNPC = 0.0f;
            AnimacionCoheteKF::iniciarReproduccion();
			camaraLigadaAvatar = false;
			camaraEnNave = false;
			camera = Camera(glm::vec3(250.0f, 120.0f, 250.0f), glm::vec3(0.0f, 1.0f, 0.0f), -135.0f, -20.0f, 1.5f, 0.5f);
			glfwSetWindowTitle(glfwGetCurrentContext(), "Recorrido 4 - Cinemática | 1,2,3 Escenarios | 9 Recorrido");
		}

		// ---- Lógica de tiempo y cámara para el Recorrido 4 ----
		if (escenarioActual == 4) {
			tiempoRecorrido4 += static_cast<float>(segundosValle) * VELOCIDAD_RECORRIDO4;

			if (tiempoRecorrido4 > (DURACION_FASE1 + DURACION_FASE2)) {
				tiempoRecorrido4 = 0.0f;
				cicloCohete = 0.0f;
				coheteAltura = 0.0f;
				coheteVelocidad = 0.0f;
				coheteLanzando = false;
			}

			float anguloOrbita = tiempoRecorrido4 * 0.08f;

			if (tiempoRecorrido4 < DURACION_FASE1) {
				faseRecorrido4 = 0;
				glm::vec3 lookAt(0.0f, 40.0f, -7.0f);
				float radio = 90.0f;
				AnimacionCoheteKF::enfoqueCamara(glm::vec3(0.0f, 8.5f, -7.0f), lookAt, radio);
				glm::vec3 camPos = lookAt + glm::vec3(radio * cos(anguloOrbita), 15.0f * sin(anguloOrbita * 0.7f), radio * sin(anguloOrbita));
				glm::vec3 dir = glm::normalize(lookAt - camPos);
				camera = Camera(camPos, glm::vec3(0.0f, 1.0f, 0.0f), glm::degrees(atan2(dir.z, dir.x)), glm::degrees(asin(dir.y)), 1.0f, 0.5f);
			}
			else {
				faseRecorrido4 = 1;
				float tLocal = tiempoRecorrido4 - DURACION_FASE1;

				// Cámara más cerca de la estación (radio = 60)
				glm::vec3 centro(200.0f, 80.0f, 150.0f);
				float radio = 10.0f;
				glm::vec3 camPos = centro + glm::vec3(radio * cos(anguloOrbita), 5.0f + 1.0f * sin(anguloOrbita * 0.6f), radio * sin(anguloOrbita));
				glm::vec3 lookAt(200.0f, 80.0f, 150.0f);
				glm::vec3 dir = glm::normalize(lookAt - camPos);
				camera = Camera(camPos, glm::vec3(0.0f, 1.0f, 0.0f), glm::degrees(atan2(dir.z, dir.x)), glm::degrees(asin(dir.y)), 1.0f, 0.5f);

				// Tiempos ralentizados
				if (tLocal < 3.0f) {
					aperturaPuertas = tLocal / 3.0f; progresoNaveNPC = 1.0f;
				}
				else if (tLocal < 9.0f) {
					aperturaPuertas = 1.0f; progresoNaveNPC = 1.0f - ((tLocal - 3.0f) / 6.0f); // Nave entra (6s)
				}
				else if (tLocal < 12.0f) {
					aperturaPuertas = 1.0f - ((tLocal - 9.0f) / 3.0f); progresoNaveNPC = 0.0f; // Cierra (3s)
				}
				else if (tLocal < 15.0f) {
					aperturaPuertas = (tLocal - 12.0f) / 3.0f; progresoNaveNPC = 0.0f; // Abre (3s)
				}
				else if (tLocal < 21.0f) {
					aperturaPuertas = 1.0f; progresoNaveNPC = (tLocal - 15.0f) / 6.0f; // Nave sale (6s)
				}
				else {
					aperturaPuertas = 1.0f - ((tLocal - 21.0f) / 4.0f); progresoNaveNPC = 1.0f; // Cierra (4s)
				}

				aperturaPuertas = std::clamp(aperturaPuertas, 0.0f, 1.0f);
				progresoNaveNPC = std::clamp(progresoNaveNPC, 0.0f, 1.0f);
			}
		}
		// -----------------------------------------

        const double tiempoCiclo = glfwGetTime() - inicioCiclo;
        const auto ciclo = CicloDia::evaluar(tiempoCiclo);
        if (escenarioActual == 1 || (escenarioActual == 4 && faseRecorrido4 == 1)) {
            mainLight = DirectionalLight(1, 1, 1, 0.4f, 0.4f, 0, -1, 0);
        } else {
            const auto& c = ciclo.colorLuz;
            const auto d = ciclo.haciaSol; // El shader usa dot(normal, direction).
            mainLight = DirectionalLight(c.x, c.y, c.z, ciclo.ambiente, ciclo.difusa, d.x, d.y, d.z);
        }

		// Avatar en los escenarios 2 (Valle) y 3 (Erid): WASD lo mueve y la cámara lo sigue en tercera persona
		static int escenarioAvatar = 0;
		if (escenarioActual == 2 || escenarioActual == 3) {
			if (escenarioAvatar != escenarioActual) {
				Avatar::entrarEn(escenarioActual);
				escenarioAvatar = escenarioActual;
			}

			// ===== AQUÍ VA EL PASO 6 (reemplaza los dos if anteriores) =====
			if (camaraLigadaAvatar && !camaraEnNave) {
				Avatar::actualizar(mainWindow.getsKeys(), static_cast<float>(segundosValle));
				Avatar::seguirConCamara(camera, dxMouse, dyMouse);
			}
			// ===============================================================
		}
		else {
			escenarioAvatar = 0;
		}
        LucesNocturnas::ActualizarInterruptores(escenarioActual, Avatar::pos,
            mainWindow.getsKeys()[GLFW_KEY_X], camaraEnNave);
        // Si estamos en el Escenario 3, calculamos la matriz de la pista y de la nave desde aquí
		// para que la cámara en 3ra persona (Tecla 5) actualice la vista antes de dibujar el Skybox y enviar uniformView
		glm::mat4 matrizPista = glm::mat4(1.0f);
		glm::mat4 modelVehiculo = glm::mat4(1.0f);
		if (escenarioActual == 3) {
			if (camaraEnNave) {
				float dtSec = static_cast<float>(segundosValle);
				if (mainWindow.getsKeys()[GLFW_KEY_A] || mainWindow.getsKeys()[GLFW_KEY_LEFT])
					corredores[0].laneOffset -= 4.5f * dtSec;
				if (mainWindow.getsKeys()[GLFW_KEY_D] || mainWindow.getsKeys()[GLFW_KEY_RIGHT])
					corredores[0].laneOffset += 4.5f * dtSec;
				corredores[0].laneOffset = std::clamp(corredores[0].laneOffset, -4.0f, 4.0f);
			}

			matrizPista = glm::translate(glm::mat4(1.0f), glm::vec3(-100.0f, 2.0f, 150.0f));
			matrizPista = glm::scale(matrizPista, glm::vec3(2.5f));
			modelVehiculo = pistaCarreras.getVehicleTransform(corredores[0], tiempoVuelo, matrizPista);

			if (camaraEnNave) {
				// Extraemos posición y vectores ortonormales de la nave en el mundo
				glm::vec3 posNave = glm::vec3(modelVehiculo[3]);
				glm::vec3 upNave = glm::normalize(glm::vec3(modelVehiculo[1]));
				// Con yawFixDeg = 180.0f, el eje +Z local de modelVehiculo apunta hacia adelante en la pista
				glm::vec3 fwdNave = glm::normalize(glm::vec3(modelVehiculo[2]));

				// Colocar la cámara detrás y ligeramente arriba de la nave
				glm::vec3 camPos = posNave - fwdNave * 18.0f + upNave * 7.5f;
				glm::vec3 target = posNave + fwdNave * 25.0f + upNave * 2.0f;
				glm::vec3 lookDir = glm::normalize(target - camPos);

				float yawDeg = glm::degrees(std::atan2(lookDir.z, lookDir.x));
				float pitchDeg = glm::degrees(std::asin(std::clamp(lookDir.y, -0.99f, 0.99f)));

				camera = Camera(camPos, upNave, yawDeg, pitchDeg, 0.0f, 0.0f);
			}
		}


		// Escenario 2: Valle helado, Crepper NPC y modelos de Planeta 1
		if (escenarioActual == 2) {
			// Animaciones y matrices de la plataforma/torre Hextech antes de dibujar el valle
			// para que sus luces nocturnas iluminen tambien la nieve y el avatar (como en P08320291197.cpp)
			anguloCirculoHex += 2.0f * deltaTime;
			anguloSemiAro += 1.0f * deltaTime;
			float flotacionEsfera = sin((float)glfwGetTime() * 0.5f) * 0.2f;

			glm::mat4 modelBase = glm::translate(glm::mat4(1.0f), glm::vec3(10.5f, 0.0f, -30.0f));
			modelBase = glm::rotate(modelBase, 180.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			modelBase = glm::scale(modelBase, glm::vec3(10.0f));

			glm::mat4 modelNucleo = glm::translate(modelBase, glm::vec3(0.0f, 0.41f, 0.0f));
			modelNucleo = glm::rotate(modelNucleo, glm::radians(anguloCirculoHex), glm::vec3(0.0f, 1.0f, 0.0f));

			glm::mat4 modelCirculo = glm::translate(modelBase, glm::vec3(0.0f, 0.85f, -0.41f));
			modelCirculo = glm::rotate(modelCirculo, glm::radians(anguloCirculoHex), glm::vec3(0.0f, 1.0f, 0.0f));

			glm::mat4 modelAndamioDer = glm::translate(modelBase, glm::vec3(-2.5f, 0.85f, 0.45f));
			glm::mat4 modelTorre = glm::translate(modelBase, glm::vec3(0.0f, 3.15f, 0.95f));
			glm::mat4 modelEsfera = glm::translate(modelTorre, glm::vec3(0.0f, 3.3f + flotacionEsfera, 0.0f));
			glm::mat4 modelSemiAro = glm::rotate(modelEsfera, glm::radians(anguloSemiAro), glm::vec3(0.0f, 1.0f, 1.0f));

			LucesNocturnas::ActualizarTorreValle(modelNucleo, modelTorre);

			auto proyeccionValle = glm::perspective(glm::radians(45.0f),
				(float)mainWindow.getBufferWidth() / mainWindow.getBufferHeight(), 0.1f, 1200.0f);
			ValleHelado::dibujar(camera, proyeccionValle, segundosValle, ciclo, mainWindow.getLinternaEncendida(), camaraLigadaAvatar);


			// Avatar dibujado con el shader del valle (view y projection ya quedaron puestos en dibujar)
			{
				GLint prog = ValleHelado::programa;
				glUseProgram(prog);
				glUniform1i(glGetUniformLocation(prog, "usarTextura"), 1);
				Avatar::dibujar(glGetUniformLocation(prog, "model"), glGetUniformLocation(prog, "tinte"));
				glUseProgram(0);
			}
			// ===== PLATAFORMA HEXTECH (con shader de iluminación principal) =====
			shaderList[0].UseShader();
			LucesNocturnas::configurar(2, ciclo, camera, mainWindow.getLinternaEncendida(), camaraLigadaAvatar);

			GLint progPrincipal = 0;
			glGetIntegerv(GL_CURRENT_PROGRAM, &progPrincipal);
			GLint uEmision = glGetUniformLocation(progPrincipal, "emisionObjeto");

			GLuint uModel = shaderList[0].GetModelLocation();
			GLuint uProjection = shaderList[0].GetProjectionLocation();
			GLuint uView = shaderList[0].GetViewLocation();
			GLuint uEyePos = shaderList[0].GetEyePositionLocation();
			GLuint uColor = shaderList[0].getColorLocation();
			GLuint uSpecular = shaderList[0].GetSpecularIntensityLocation();
			GLuint uShininess = shaderList[0].GetShininessLocation();

			glUniformMatrix4fv(uProjection, 1, GL_FALSE, glm::value_ptr(proyeccionValle));
			glUniformMatrix4fv(uView, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));
			glUniform3f(uEyePos,
				camera.getCameraPosition().x,
				camera.getCameraPosition().y,
				camera.getCameraPosition().z);

			shaderList[0].SetDirectionalLight(&mainLight);
			shaderList[0].SetPointLights(LucesNocturnas::lucesFarolasActivas, LucesNocturnas::cantidadFarolasActivas);
			shaderList[0].SetSpotLights(spotLights, 0);
			Material_brillante.UseMaterial(uSpecular, uShininess);

			glm::vec3 color = glm::vec3(1.0f);
			const glm::vec3 emisionHextech = glm::vec3(0.08f, 0.36f, 0.62f) * LucesNocturnas::intensidad(ciclo);

			// 1. Base (PADRE)
			glUniform3fv(uColor, 1, glm::value_ptr(color));
			glUniform3f(uEmision, 0.0f, 0.0f, 0.0f);
			glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(modelBase));
			BasePlataforma_M.RenderModel();

			// 2. Núcleo
			glUniform3fv(uEmision, 1, glm::value_ptr(emisionHextech));
			glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(modelNucleo));
			NucleoPlataforma_M.RenderModel();

			// 3. Círculo Hextech
			glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(modelCirculo));
			CirculoHex_M.RenderModel();
			glUniform3f(uEmision, 0.0f, 0.0f, 0.0f);

			// 4. Andamio Izq
			glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(modelBase));
			AndamioIzqPlataforma_M.RenderModel();

			// 5. Andamio Der
			glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(modelAndamioDer));
			AndamioDerPlataforma_M.RenderModel();

			// 6. Torre
			glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(modelTorre));
			TorrePlataforma_M.RenderModel();

			// 7. Esfera flotante
			glUniform3fv(uEmision, 1, glm::value_ptr(emisionHextech));
			glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(modelEsfera));
			EsferaPlataforma_M.RenderModel();

			// 8. Semi aro
			glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(modelSemiAro));
			SemiaroPlataforma_M.RenderModel();
			glUniform3f(uEmision, 0.0f, 0.0f, 0.0f);

			// 9. Cohete de 3 Fases animado por Keyframes (Ejercicio 1)
			plainTexture.UseTexture();
			AnimacionCoheteKF::dibujar(glm::vec3(10.5f, 8.5f, -23.0f), uModel, uColor, Cohete_M);

			glUseProgram(0);

			mainWindow.swapBuffers();
			continue;
		}
		ValleHelado::estabaActivo = false;


		// Clear the window
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// Skybox por escenario
		if (escenarioActual == 1) {
			skybox1.DrawSkybox(camera.calculateViewMatrix(), projection);
		}
		else if (escenarioActual == 3) {
			skybox3.DrawSkybox(camera.calculateViewMatrix(), projection, true, ciclo.tinteCielo, ciclo.haciaSol, ciclo.crepusculo);
		}


        if (escenarioActual == 4) {
            if (faseRecorrido4 == 0)
                skybox2.DrawSkybox(camera.calculateViewMatrix(), projection, true, ciclo.tinteCielo, ciclo.haciaSol, ciclo.crepusculo);
            else skybox1.DrawSkybox(camera.calculateViewMatrix(), projection);
        }

		// Actualizar matrices padre de Erid (pista, nave y podios) antes de enviar las luces y dibujar el piso (como en P08320291197.cpp)
		if (escenarioActual == 3) {
			glm::vec3 naveSpotPos = glm::vec3(modelVehiculo * glm::vec4(0.0f, 0.0f, 5.0f, 1.0f));
			glm::vec3 naveSpotDir = glm::normalize(glm::vec3(modelVehiculo * glm::vec4(0.0f, -0.4f, 1.0f, 0.0f)));
			spotLights[indiceFaroNave].SetFlash(naveSpotPos, naveSpotDir);

			glm::vec3 navePuntualPos = glm::vec3(modelVehiculo * glm::vec4(-0.5f, 2.25f, 0.0f, 1.0f));
			pointLights[indicePuntualNave].SetPos(navePuntualPos);

			const float yEstrado = EscenarioDebate::Y_BASE + (EscenarioDebate::NUM_ESCALONES * EscenarioDebate::ALTO_ESCALON);
			glm::mat4 matrizPodioIzq = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-16.0f, yEstrado, -152.0f)), glm::vec3(3.5f));
			glm::mat4 matrizPodioDer = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 16.0f, yEstrado, -152.0f)), glm::vec3(3.5f));
			LucesNocturnas::ActualizarEscenaErid(matrizPista, matrizPodioIzq, matrizPodioDer);
		}

		// Configuración de los Shaders de Iluminación 
		shaderList[0].UseShader();
		LucesNocturnas::configurar(escenarioActual == 4 ? 1 : escenarioActual, ciclo, camera, mainWindow.getLinternaEncendida(), camaraLigadaAvatar && !camaraEnNave);
		uniformModel = shaderList[0].GetModelLocation();
		uniformProjection = shaderList[0].GetProjectionLocation();
		uniformView = shaderList[0].GetViewLocation();
		uniformEyePosition = shaderList[0].GetEyePositionLocation();
		uniformColor = shaderList[0].getColorLocation();


		//información en el shader de intensidad especular y brillo
		uniformSpecularIntensity = shaderList[0].GetSpecularIntensityLocation();
		uniformShininess = shaderList[0].GetShininessLocation();


		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));
		glUniform3f(uniformEyePosition, camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);


        spotLights[0] = SpotLight(1,1,1, 0,0, 0,0,0, 0,-1,0, 1,0,0, 25);


		if (escenarioActual == 1)
		{
			shaderList[0].SetDirectionalLight(&mainLight);
			shaderList[0].SetPointLights(pointLights, 1); // sólo el Sol
			shaderList[0].SetSpotLights(spotLights, 0);
		}


        else if (escenarioActual == 4) {
            shaderList[0].SetDirectionalLight(&mainLight);
            const float brilloRecorrido = LucesNocturnas::intensidad(ciclo);
            if (faseRecorrido4 == 0 && brilloRecorrido > .001f) {
                PointLight lucesLanzamiento[2];
                lucesLanzamiento[0] = PointLight(0.15f, 0.75f, 1.0f, 0.12f * brilloRecorrido, 2.0f * brilloRecorrido,
                    0.0f, 8.6f, 0.0f, 1.0f, 0.025f, 0.0035f);
                lucesLanzamiento[1] = PointLight(0.28f, 0.80f, 1.0f, 0.14f * brilloRecorrido, 2.2f * brilloRecorrido,
                    0.0f, 49.5f, 3.5f, 1.0f, 0.020f, 0.0025f);
                shaderList[0].SetPointLights(lucesLanzamiento, 2);
            } else {
                shaderList[0].SetPointLights(pointLights, 0);
            }
            shaderList[0].SetSpotLights(spotLights, 0);
        }
		else if (escenarioActual == 3)
		{
			EscenarioDebate::ActualizarLucesDinamicas(spotLights, tiempoVuelo, focosEncendidos && LucesNocturnas::intensidad(ciclo) > .5f, rayoEncendido);

			shaderList[0].SetDirectionalLight(&mainLight);

			PointLight lucesErid[MAX_POINT_LIGHTS];
            unsigned int cantidadLucesErid = 0;
            lucesErid[cantidadLucesErid++] = pointLights[indicePuntualNave];
            for (unsigned int i = 0; i < LucesNocturnas::cantidadFarolasActivas && cantidadLucesErid < MAX_POINT_LIGHTS; ++i)
                lucesErid[cantidadLucesErid++] = LucesNocturnas::lucesFarolasActivas[i];
            shaderList[0].SetPointLights(lucesErid, cantidadLucesErid);
			shaderList[0].SetSpotLights(spotLights, spotLightCount);
		}


		glm::mat4 model(1.0);
		glm::mat4 modelaux(1.0);
		glm::mat4 modelaux2(1.0); // para la jerarquía del satelite
		glm::mat4 modelaux3(1.0); // para la jerarquía del satelite
		glm::mat4 modelaux4(1.0);
		glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);


		// Dibujado de objetos según el escenario
		if (escenarioActual == 1) {

			Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
			// Animación básica de los planetas
			rotaPlanetas += rotaPlanetasOffset * deltaTime;
			if (rotaPlanetas >= 360.0f * toRadians) {
				rotaPlanetas = 0.0f;
			}


			// Animación básica de los anillos de asteroides
			rotaAnillo += rotaAnilloOffset * deltaTime;


			// Animación de la luna orbitando alrededor del planeta 1
			rotaLuna += rotaLunaOffset * deltaTime;


			if (rotaLuna >= 360.0f * toRadians) {
				rotaLuna = 0.0f;
			}


			// Animación del satélite
			rotaAntena += rotaAntenaOffset * deltaTime;
			rotaPanelIzq += rotaPanelIzqOffset * deltaTime;
			rotaPanelDer += rotaPanelDerOffset * deltaTime;
			movSatelite += movSateliteOffset * deltaTime;
			rotacionSatelite += 0.001f * deltaTime;


			// Sol
			model = glm::mat4(1.0);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(0.0f, 75.0f, -150.0f));
			model = glm::rotate(model, rotaPlanetas, glm::vec3(0.0f, -1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(100.0f, 100.0f, 100.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			sol.RenderModel();


			// Planeta 1
			model = glm::mat4(1.0);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(250.0f, 0.0f, 175.0f));
			model = glm::rotate(model, rotaPlanetas, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(40.0f, 40.0f, 40.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			planeta1.RenderModel();


			// Luna
			model = glm::mat4(1.0f);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(250.0f, 0.0f, 175.0f));
			model = glm::rotate(model, rotaLuna, glm::vec3(0.0f, -1.0f, 0.0f));
			model = glm::translate(model, glm::vec3(100.0f, 0.0f, 0.0f));
			model = glm::rotate(model, rotaLuna * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(12.0f, 12.0f, 12.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			luna.RenderModel();


			// Satélite
			// Cuerpo del satelite
			model = modelaux2;
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::mat4(1.0);
			model = glm::translate(model, glm::vec3(250.0f, 0.0f, 175.0f));
			model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::rotate(model, movSatelite, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::translate(model, glm::vec3(60.0f, 0.0f, 0.0f));
			model = glm::rotate(model, rotacionSatelite, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(0.75f, 0.75f, 0.75f));
			modelaux = model;
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			cuerpoSat.RenderModel();


			// Antena
			model = modelaux;						//  z , x, y
			model = glm::translate(model, glm::vec3(0.0f, 0.0f, -0.75f));
			model = glm::rotate(model, rotaAntena, glm::vec3(0.0f, 0.0f, 1.0f));
			modelaux3 = model;
			model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			antenaSat.RenderModel();


			// Panel solar 1
			model = modelaux;
			model = glm::translate(model, glm::vec3(0.0f, 1.7f, 0.0f));
			model = glm::rotate(model, rotaPanelIzq, glm::vec3(0.0f, 1.0f, 0.0f));
			modelaux3 = model;
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			panelIzqSat.RenderModel();


			// Panel solar 2
			model = modelaux;
			model = glm::translate(model, glm::vec3(0.0f, -1.65f, 0.0f));
			model = glm::rotate(model, rotaPanelDer, glm::vec3(0.0f, 1.0f, 0.0f));
			modelaux3 = model;
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			panelDerSat.RenderModel();


			// Planeta 2
			model = glm::mat4(1.0);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(-250.0f, 0.0f, 175.0f));
			model = glm::rotate(model, rotaPlanetas, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(40.0f, 40.0f, 40.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			planeta2.RenderModel();


			// Cinturón de asteroides 1
			model = glm::mat4(1.0);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(-250.0f, 0.0f, 175.0f));
			model = glm::rotate(model, 90.0f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, rotaAnillo, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, glm::vec3(1.55f, 1.55f, 1.55f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			anilloAst.RenderModel();


			// Cinturón de asteroides 2
			model = glm::mat4(1.0);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(-250.0f, 0.0f, 175.0f));
			model = glm::rotate(model, 75.0f * toRadians, glm::vec3(-1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, rotaAnillo, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, glm::vec3(1.7f, 1.7f, 1.7f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			anilloAst.RenderModel();


			// Asteroides
			model = glm::mat4(1.0);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(-100.0f, -50.0f, 750.0f));
			model = glm::rotate(model, 90.0f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, 45.0f * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, glm::vec3(50.0f, 50.0f, 50.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			asteroides.RenderModel();

			//ESTACION 

			// 1. Cabina
			glm::mat4 modelEstacionCabina = glm::mat4(1.0);
			modelEstacionCabina = glm::translate(modelEstacionCabina, glm::vec3(220.0f, 40.0f, 150.0f));
			modelEstacionCabina = glm::rotate(modelEstacionCabina, 90.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			modelEstacionCabina = glm::scale(modelEstacionCabina, glm::vec3(1.0f, 1.0f, 1.0f));

			color = glm::vec3(1.0f, 1.0f, 1.0f);
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelEstacionCabina));
			EstacionCabina_M.RenderModel();

			// 2. Puerta izq
			glm::mat4 modelEstacionPuertaIzq = modelEstacionCabina;
			modelEstacionPuertaIzq = glm::translate(modelEstacionPuertaIzq, glm::vec3(-2.6f, 0.0f, 0.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelEstacionPuertaIzq));
			EstacionPuertaIzq_M.RenderModel();

			// 3. Puerta der
			glm::mat4 modelEstacionPuertaDer = modelEstacionCabina;
			modelEstacionPuertaDer = glm::translate(modelEstacionPuertaDer, glm::vec3(-2.6f, 0.0f, 0.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelEstacionPuertaDer));
			EstacionPuertaDer_M.RenderModel();


			// 4. Estructura
			glm::mat4 modelEstacionEstructura = modelEstacionCabina;
			modelEstacionEstructura = glm::translate(modelEstacionEstructura, glm::vec3(0.0f, 0.0f, 0.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelEstacionEstructura));
			EstacionEstructura_M.RenderModel();

			//  5. EstacionPanelDerecho
			glm::mat4 modelEstacionPanelDerecho = modelEstacionEstructura;
			modelEstacionPanelDerecho = glm::translate(modelEstacionPanelDerecho, glm::vec3(0.0f, 0.0f, 8.5f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelEstacionPanelDerecho));
			EstacionPanelDerecho_M.RenderModel();

			//  6. EstacionPanelIzquierdo
			glm::mat4 modelEstacionPanelIzquierdo = modelEstacionEstructura;
			modelEstacionPanelIzquierdo = glm::translate(modelEstacionPanelIzquierdo, glm::vec3(0.0f, 0.0f, -8.5f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelEstacionPanelIzquierdo));
			EstacionPanelIzquierdo_M.RenderModel();

			// 7. EstacionCapsulaDer
			glm::mat4 modelEstacionCapsulasDer = modelEstacionEstructura;
			modelEstacionCapsulasDer = glm::translate(modelEstacionCapsulasDer, glm::vec3(0.0f, 0.0f, 5.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelEstacionCapsulasDer));
			EstacionCapsulasDer_M.RenderModel();

			// 8. EstacionCapsulaIzq
			glm::mat4 modelEstacionCapsulasIzq = modelEstacionEstructura;
			modelEstacionCapsulasIzq = glm::translate(modelEstacionCapsulasIzq, glm::vec3(0.0f, 0.0f, -5.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelEstacionCapsulasIzq));
			EstacionCapsulasIzq_M.RenderModel();


		}


		else if (escenarioActual == 3) {


			// Animación de Rocky saludando
			if (saludo)
			{
				rotBrazoRocky += rotBrazoRockyOffset;


				if (rotBrazoRocky >= 30.0f)
					saludo = false;
			}
			else
			{
				rotBrazoRocky -= rotBrazoRockyOffset;


				if (rotBrazoRocky <= -30.0f)
					saludo = true;
			}


			// Dibujar Piso 
			model = glm::mat4(1.0);
			model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(100.0f, 1.0f, 100.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));


			pisoPlaneta2.UseTexture();
			Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
			meshListModel[2]->RenderMeshModel();


			// Dibujar Rocas alrededor
			model = glm::mat4(1.0);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(0.0f, -5.0f, 0.0f));
			model = glm::rotate(model, 90.0f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(150.0f, 150.0f, 150.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			rocas1.RenderModel();

			// Roca decoración pista
			model = glm::mat4(1.0);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(-75.0f, -0.5f, 340.0f));
			modelaux = model;
			model = glm::rotate(model, 90.0f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, -30.0f * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, glm::vec3(90.0f, 90.0f, 90.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			roca2.RenderModel();

			// Dibujar Pista de Carreras en Erid
			// (matrizPista y modelVehiculo ya se calcularon antes del Skybox para sincronizar la cámara de la tecla 5)

			// 2. Dibujar Pista
			color = glm::vec3(0.85f, 0.85f, 0.9f);
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(matrizPista));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			dirtTexture.UseTexture();
			Material_brillante.UseMaterial(uniformSpecularIntensity, uniformShininess);
			pistaCarreras.draw();


			// 3. Dibujar las 4 naves corriendo en la pista
			Material_brillante.UseMaterial(uniformSpecularIntensity, uniformShininess);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));

			for (int i = 0; i < 4; i++) {
				// La 0 reutiliza la matriz ya calculada (cuyas luces ya se enviaron antes de dibujar el piso y la pista)
				glm::mat4 mv = (i == 0)
					? modelVehiculo
					: pistaCarreras.getVehicleTransform(corredores[i], tiempoVuelo, matrizPista);

				glm::mat4 modelNave = glm::scale(mv, glm::vec3(escalaNaves[i]));
				modelNave = glm::translate(modelNave, glm::vec3(0.0f, alturaNaves[i], 0.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelNave));
				navesCarrera[i].RenderModel();
			}

			Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);

			// Dibujar a Rocky 1
			model = glm::mat4(1.0);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(300.0f, 7.0f, 65.0f));
			modelaux4 = model;
			model = glm::rotate(model, 90.0f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(35.0f, 35.0f, 35.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			rocky.RenderModel();


			model = modelaux4;
			model = glm::translate(model, glm::vec3(-0.75f, 1.9f, 3.3f));
			model = glm::rotate(model, rotBrazoRocky * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::rotate(model, 180.0f * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::rotate(model, 90.0f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(35.0f, 35.0f, 35.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			brazoRocky.RenderModel();

			// Dibujar a Rocky 2
			model = glm::mat4(1.0);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(300.0f, 6.5f, 85.0f));
			modelaux4 = model;
			model = glm::rotate(model, 180.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, 90.0f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			rocky.RenderModel();


			model = modelaux4;
			model = glm::translate(model, glm::vec3(0.575f, 1.15f, -1.85f));
			model = glm::rotate(model, rotBrazoRocky * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::rotate(model, 180.0f * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::rotate(model, 180.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, 90.0f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			brazoRocky.RenderModel();

			// Xenonita
			model = glm::mat4(1.0);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(300.0f, 1.25f, 0.0f));
			modelaux = model;
			model = glm::rotate(model, 90.0f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(50.0f, 50.0f, 50.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			xenonita.RenderModel();

			model = glm::mat4(1.0);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(375.0f, 4.0f, 75.0f));
			modelaux = model;
			model = glm::rotate(model, 50.0f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, 90.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(50.0f, 50.0f, 50.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			xenonita.RenderModel();

			model = glm::mat4(1.0);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(300.0f, 1.25f, 150.0f));
			modelaux = model;
			model = glm::rotate(model, 20.0f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, 105.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(50.0f, 50.0f, 50.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			xenonita.RenderModel();

			// Roca decoración
			model = glm::mat4(1.0);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			model = glm::translate(model, glm::vec3(300.0f, -1.0f, 75.0f));
			modelaux = model;
			model = glm::rotate(model, 90.0f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, 30.0f * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, glm::vec3(30.0f, 30.0f, 30.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			roca2.RenderModel();

            plainTexture.UseTexture();
            LucesNocturnas::DibujarFarolas(3, ciclo, uniformModel, uniformColor);

			// ----- Escenario del Debate -----
			EscenarioDebate::DibujarEscenario(
				uniformModel, uniformColor, uniformSpecularIntensity, uniformShininess,
				plainTexture, texEscalones, Material_opaco);


			// Avatar (opaco: va antes del rayo translúcido)
			Avatar::dibujar(uniformModel, uniformColor);


			EscenarioDebate::DibujarModelosDebate(
				uniformModel, uniformColor, uniformSpecularIntensity, uniformShininess,
				Podio_M,Kodos_M, Kang_M, NaveAlien_M, Homero_M,
				plainTexture, Material_brillante, tiempoVuelo,
				rayoEncendido, alturaAbduccion, giroAbduccion
			);

			glUseProgram(0);

		}


		else if (escenarioActual == 4) {
			glUniform3f(uniformColor, 1, 1, 1);
            Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
            float dt4 = static_cast<float>(segundosValle) * VELOCIDAD_RECORRIDO4;

			if (faseRecorrido4 == 0) {
				// FASE 1: VALLE (Piso + Plataforma + Cohete)
				model = glm::mat4(1.0);
				model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
				model = glm::scale(model, glm::vec3(200.0f, 1.0f, 200.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				glUniform3fv(uniformColor, 1, glm::value_ptr(glm::vec3(1.0f)));
				pisoPlaneta1.UseTexture();
				Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
				meshListModel[2]->RenderMeshModel();

				cicloCohete += dt4;
				if (!coheteLanzando && cicloCohete >= CICLO_COHETE) { coheteLanzando = true; coheteVelocidad = 0.0f; }
				if (coheteLanzando) {
					coheteVelocidad += ACELERACION_COHETE * 3 * dt4;
					coheteAltura += coheteVelocidad * dt4;
					if (coheteAltura > ALTURA_MAX_COHETE) {
						coheteAltura = 0.0f; coheteVelocidad = 0.0f; coheteLanzando = false; cicloCohete = 0.0f;
					}
				}

				anguloCirculoHex += 16.0f * dt4;
				anguloSemiAro += 13.0f * dt4;
				float flotacionEsfera = sin((float)glfwGetTime() * 0.5f) * 0.2f;

				glm::mat4 modelBase = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
				modelBase = glm::scale(modelBase, glm::vec3(10.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelBase));
				BasePlataforma_M.RenderModel();

				glm::mat4 modelNucleo = glm::translate(modelBase, glm::vec3(0.0f, 0.41f, 0.0f));
				modelNucleo = glm::rotate(modelNucleo, glm::radians(anguloCirculoHex), glm::vec3(0.0f, 1.0f, 0.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelNucleo));
				NucleoPlataforma_M.RenderModel();

				glm::mat4 modelCirculo = glm::translate(modelBase, glm::vec3(0.0f, 0.85f, -0.41f));
				modelCirculo = glm::rotate(modelCirculo, glm::radians(anguloCirculoHex), glm::vec3(0.0f, 1.0f, 0.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelCirculo));
				CirculoHex_M.RenderModel();

				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelBase));
				AndamioIzqPlataforma_M.RenderModel();

				glm::mat4 modelAndamioDer = glm::translate(modelBase, glm::vec3(-2.5f, 0.85f, 0.45f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelAndamioDer));
				AndamioDerPlataforma_M.RenderModel();

				glm::mat4 modelTorre = glm::translate(modelBase, glm::vec3(0.0f, 3.15f, 0.95f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelTorre));
				TorrePlataforma_M.RenderModel();

				glm::mat4 modelEsfera = glm::translate(modelTorre, glm::vec3(0.0f, 3.3f + flotacionEsfera, 0.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelEsfera));
				EsferaPlataforma_M.RenderModel();

				glm::mat4 modelSemiAro = glm::rotate(modelEsfera, glm::radians(anguloSemiAro), glm::vec3(0.0f, 1.0f, 1.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelSemiAro));
				SemiaroPlataforma_M.RenderModel();

				if (!AnimacionCoheteKF::play && tiempoRecorrido4 < 1.0f) AnimacionCoheteKF::iniciarReproduccion();
				plainTexture.UseTexture();
				AnimacionCoheteKF::dibujar(glm::vec3(0.0f, 8.5f, -7.0f), uniformModel, uniformColor, Cohete_M);
			}
			else {
				// FASE 2: ESPACIO (Estación + Naves + Celestes)
				glm::mat4 modelEstacionCabina = glm::translate(glm::mat4(1.0f), glm::vec3(200.0f, 80.0f, 150.0f));
				modelEstacionCabina = glm::rotate(modelEstacionCabina, 90.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelEstacionCabina));
				EstacionCabina_M.RenderModel();

				glm::mat4 modelPuertaIzq = glm::translate(modelEstacionCabina, glm::vec3(-2.6f, 0.0f, -1.0f * aperturaPuertas));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelPuertaIzq));
				EstacionPuertaIzq_M.RenderModel();

				glm::mat4 modelPuertaDer = glm::translate(modelEstacionCabina, glm::vec3(-2.6f, 0.0f, 1.0f * aperturaPuertas));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelPuertaDer));
				EstacionPuertaDer_M.RenderModel();

				glm::mat4 modelEstructura = modelEstacionCabina;
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelEstructura));
				EstacionEstructura_M.RenderModel();

				glm::mat4 modelPanelDer = glm::translate(modelEstructura, glm::vec3(0.0f, 0.0f, 8.5f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelPanelDer));
				EstacionPanelDerecho_M.RenderModel();

				glm::mat4 modelPanelIzq = glm::translate(modelEstructura, glm::vec3(0.0f, 0.0f, -8.5f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelPanelIzq));
				EstacionPanelIzquierdo_M.RenderModel();

				glm::mat4 modelCapsDer = glm::translate(modelEstructura, glm::vec3(0.0f, 0.0f, 5.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelCapsDer));
				EstacionCapsulasDer_M.RenderModel();

				glm::mat4 modelCapsIzq = glm::translate(modelEstructura, glm::vec3(0.0f, 0.0f, -5.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelCapsIzq));
				EstacionCapsulasIzq_M.RenderModel();

				// Nave NPC Entrando/Saliendo
				float distanciaMax = 60.0f;
				float escalaNave = 2.0f;
				glm::vec3 posicionPuerta(-2.6f, 0.0f, 0.0f);
				float distancia = distanciaMax * progresoNaveNPC;
				glm::vec3 offsetNave = posicionPuerta + glm::vec3(-distancia, 0.0f, 0.0f);

				if (progresoNaveNPC > 0.02f) {
					float factorEscala = 1.0f;
					float angulonave = 0.0f;
					if (progresoNaveNPC < 0.15f) factorEscala = progresoNaveNPC / 0.15f;
					if (progresoNaveNPC > 0.85f) factorEscala = (1.0f - progresoNaveNPC) / 0.15f;

					if (tiempoRecorrido4 < 22.0f) angulonave = 90.0f;
					if (tiempoRecorrido4 > 22.0f) angulonave = 270.0f;

					glm::mat4 modelNave = glm::translate(modelEstacionCabina, offsetNave);
					modelNave = glm::rotate(modelNave, glm::radians(angulonave), glm::vec3(0.0f, 1.0f, 0.0f));
					modelNave = glm::scale(modelNave, glm::vec3(escalaNave * factorEscala));
					glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(modelNave));
					naveEspacial.RenderModel();
				}

				// Objetos celestes de fondo
				model = glm::translate(glm::mat4(1.0f), glm::vec3(500.0f, 150.0f, -400.0f));
				model = glm::rotate(model, rotaPlanetas, glm::vec3(0.0f, -1.0f, 0.0f));
				model = glm::scale(model, glm::vec3(80.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				sol.RenderModel();

				model = glm::translate(glm::mat4(1.0f), glm::vec3(-400.0f, -50.0f, -500.0f));
				model = glm::rotate(model, rotaPlanetas, glm::vec3(0.0f, 1.0f, 0.0f));
				model = glm::scale(model, glm::vec3(60.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				planeta1.RenderModel();

				model = glm::translate(glm::mat4(1.0f), glm::vec3(400.0f, -50.0f, -500.0f));
				model = glm::rotate(model, rotaPlanetas, glm::vec3(0.0f, 1.0f, 0.0f));
				model = glm::scale(model, glm::vec3(60.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				planeta2.RenderModel();

				model = glm::translate(glm::mat4(1.0f), glm::vec3(400.0f, -50.0f, -500.0f));
				model = glm::rotate(model, 90.0f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
				model = glm::rotate(model, rotaAnillo, glm::vec3(0.0f, 0.0f, 1.0f));
				model = glm::scale(model, glm::vec3(3.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				anilloAst.RenderModel();

				model = glm::translate(glm::mat4(1.0f), glm::vec3(400.0f, -50.0f, -500.0f));
				model = glm::rotate(model, rotaLuna, glm::vec3(0.0f, -1.0f, 0.0f));
				model = glm::translate(model, glm::vec3(120.0f, 0.0f, 0.0f));
				model = glm::scale(model, glm::vec3(15.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				luna.RenderModel();
			}
		}

		glUseProgram(0);
		mainWindow.swapBuffers();   
	}       // <-- cierra el while

	return 0;
}



