
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <vector>
#include <glew.h>
#include <glfw3.h>

// glm
#include <glm.hpp>
#include <gtc\matrix_transform.hpp>
#include <gtc\type_ptr.hpp>

// clases para dar orden y limpieza al código
#include "Mesh.h"
#include "Shader.h"
#include "Window.h"

// Dimensiones de la ventana
const float toRadians = 3.14159265f / 180.0; // grados a radianes
Window mainWindow;
std::vector<Mesh*> meshList;
std::vector<Shader> shaderList;

// Variables y constantes globales
float angulo = 0.0f;
const int PIRAMIDE = 0;
const int CUBO = 1;

// Índices de Shaders por color
const int SHADER_AMARILLO = 0;
const int SHADER_VERDE = 1;
const int SHADER_ROJO = 2;
const int SHADER_MAGENTA = 3;
const int SHADER_CAFE = 4;
const int SHADER_AZUL = 5;
const int SHADER_NEGRO = 6;

// Pirámide triangular regular
void CreaPiramide() {
    unsigned int indices[] = {
        0,1,2,
        1,3,2,
        3,0,2,
        1,0,3
    };
    GLfloat vertices[] = {
        -0.5f, -0.5f,  0.0f,    //0
         0.5f, -0.5f,  0.0f,    //1
         0.0f,  0.5f, -0.25f,   //2
         0.0f, -0.5f, -0.5f,    //3
    };
    Mesh* obj1 = new Mesh();
    obj1->CreateMesh(vertices, indices, 12, 12);
    meshList.push_back(obj1);
}

// Vértices de un cubo
void CrearCubo() {
    unsigned int cubo_indices[] = {
        0, 1, 2, 2, 3, 0,       // front
        1, 5, 6, 6, 2, 1,       // right
        7, 6, 5, 5, 4, 7,       // back
        4, 0, 3, 3, 7, 4,       // left
        4, 5, 1, 1, 0, 4,       // bottom
        3, 2, 6, 6, 7, 3        // top
    };
    GLfloat cubo_vertices[] = {
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f
    };
    Mesh* cubo = new Mesh();
    cubo->CreateMesh(cubo_vertices, cubo_indices, 24, 36);
    meshList.push_back(cubo);
}

// Carga los múltiples shaders de color
void CreateShaders() {
    auto cargarShader = [](const char* vert, const char* frag) {
        Shader* s = new Shader();
        s->CreateFromFiles(vert, frag);
        shaderList.push_back(*s);
        };

    // Usaremos un único Vertex Shader base, pero Fragment Shaders distintos para cada color
    cargarShader("shaders/shader_base.vert", "shaders/amarillo.frag"); // 0
    cargarShader("shaders/shader_base.vert", "shaders/verde.frag");    // 1
    cargarShader("shaders/shader_base.vert", "shaders/rojo.frag");     // 2
    cargarShader("shaders/shader_base.vert", "shaders/magenta.frag");  // 3
    cargarShader("shaders/shader_base.vert", "shaders/cafe.frag");     // 4
    cargarShader("shaders/shader_base.vert", "shaders/azul.frag");     // 5
    cargarShader("shaders/shader_base.vert", "shaders/negro.frag");    // 6
}

int main() {
    mainWindow = Window(800, 600);
    mainWindow.Initialise();
    CreaPiramide(); // Índice 0
    CrearCubo();    // Índice 1
    CreateShaders();

    // Proyección ortogonal para la composición 2D utilizando objetos 3D
    glm::mat4 projection = glm::ortho(-1.5f, 1.5f, -1.0f, 1.0f, 0.1f, 100.0f);
    glm::mat4 model(1.0);

    // Función auxiliar: Configura el shader correspondiente al color y dibuja la geometría
    auto dibujarFigura = [&](int tipoGeometria, int idShader, glm::vec3 pos, glm::vec3 scale, float rotZ = 0.0f) {
        // 1. Activar el shader del color solicitado
        shaderList[idShader].useShader();
        GLuint uniformModel = shaderList[idShader].getModelLocation();
        GLuint uniformProjection = shaderList[idShader].getProjectLocation();

        // 2. Calcular matriz del modelo
        model = glm::mat4(1.0);
        model = glm::translate(model, pos);
        if (rotZ != 0.0f) {
            model = glm::rotate(model, rotZ * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
        }
        model = glm::scale(model, scale);

        // 3. Enviar matrices al shader
        glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));

        // 4. Renderizar geometría (0 = pirámide, 1 = cubo)
        meshList[tipoGeometria]->RenderMesh();
        };

    while (!mainWindow.getShouldClose()) {
        glfwPollEvents();

        glClearColor(0.85f, 0.85f, 0.85f, 1.0f); // Fondo Gris
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST); // Vital para que las figuras 3D no se solapen incorrectamente

        // ==========================================
        // DIBUJO DE LA ESCENA (Pirámides y Cubos)
        // ==========================================

        // 1. PISO (Cubo Negro aplanado)
        dibujarFigura(CUBO, SHADER_NEGRO, glm::vec3(0.0f, -0.8f, -1.0f), glm::vec3(3.0f, 0.1f, 0.5f));

        // 2. ESTRUCTURA IZQUIERDA
        // Postes usando cubos estirados
        dibujarFigura(CUBO, SHADER_CAFE, glm::vec3(-1.0f, -0.3f, -1.0f), glm::vec3(0.05f, 0.9f, 0.1f));
        dibujarFigura(CUBO, SHADER_CAFE, glm::vec3(-0.7f, -0.3f, -1.0f), glm::vec3(0.05f, 0.9f, 0.1f));

        // Triángulos invertidos usando pirámides
        dibujarFigura(PIRAMIDE, SHADER_AMARILLO, glm::vec3(-0.85f, -0.125f, -1.0f), glm::vec3(0.25f, 0.25f, 0.25f), 180.0f);
        dibujarFigura(PIRAMIDE, SHADER_ROJO, glm::vec3(-0.85f, -0.375f, -1.0f), glm::vec3(0.25f, 0.25f, 0.25f), 180.0f);
        dibujarFigura(PIRAMIDE, SHADER_VERDE, glm::vec3(-0.85f, -0.625f, -1.0f), glm::vec3(0.25f, 0.25f, 0.25f), 180.0f);

        // 3. ESTRUCTURA CENTRAL (Cuadrados formados por cubos)
        dibujarFigura(CUBO, SHADER_AMARILLO, glm::vec3(-0.15f, -0.3f, -1.0f), glm::vec3(0.3f, 0.3f, 0.1f));
        dibujarFigura(CUBO, SHADER_ROJO, glm::vec3(0.15f, -0.3f, -1.0f), glm::vec3(0.3f, 0.3f, 0.1f));
        dibujarFigura(CUBO, SHADER_MAGENTA, glm::vec3(-0.15f, -0.6f, -1.0f), glm::vec3(0.3f, 0.3f, 0.1f));
        dibujarFigura(CUBO, SHADER_VERDE, glm::vec3(0.15f, -0.6f, -1.0f), glm::vec3(0.3f, 0.3f, 0.1f));

        // Rombos superpuestos (cubos rotados 45° con un ligero salto en Z)
        dibujarFigura(CUBO, SHADER_AZUL, glm::vec3(0.0f, -0.45f, -0.99f), glm::vec3(0.424f, 0.424f, 0.12f), 45.0f);
        dibujarFigura(CUBO, SHADER_CAFE, glm::vec3(0.0f, -0.45f, -0.98f), glm::vec3(0.2f, 0.2f, 0.14f), 45.0f);

        // 4. ESTRUCTURA DERECHA (Trifuerza usando pirámides)
        dibujarFigura(PIRAMIDE, SHADER_VERDE, glm::vec3(0.6f, -0.6f, -1.0f), glm::vec3(0.3f, 0.3f, 0.3f));
        dibujarFigura(PIRAMIDE, SHADER_ROJO, glm::vec3(0.9f, -0.6f, -1.0f), glm::vec3(0.3f, 0.3f, 0.3f));
        dibujarFigura(PIRAMIDE, SHADER_MAGENTA, glm::vec3(0.75f, -0.3f, -1.0f), glm::vec3(0.3f, 0.3f, 0.3f));
        dibujarFigura(PIRAMIDE, SHADER_AMARILLO, glm::vec3(0.75f, -0.6f, -0.99f), glm::vec3(0.3f, 0.3f, 0.3f), 180.0f);

        glUseProgram(0);
        mainWindow.swapBuffers();
    }
    return 0;
}