#include <stdio.h>
#include <glew.h>
#include <glfw3.h>

// Dimensiones de la ventana
const int WIDTH = 800, HEIGHT = 800;

// Shaders básicos integrados para dibujar figuras sólidas
const char* vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos, 1.0);\n"
"}\0";

const char* fragmentShaderSource =
"#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f); // Color blanco para las figuras\n"
"}\n\0";

int main()
{
    // 1. Inicialización de GLFW
    if (!glfwInit())
    {
        printf("Falló inicializar GLFW\n");
        glfwTerminate();
        return 1;
    }

    // Configuración de versión y perfil
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Crear ventana
    GLFWwindow* mainWindow = glfwCreateWindow(WIDTH, HEIGHT, "Practica OpenGL", NULL, NULL);
    if (!mainWindow)
    {
        printf("Fallo en crearse la ventana con GLFW\n");
        glfwTerminate();
        return 1;
    }

    // Obtener tamaño de Buffer
    int BufferWidth, BufferHeight;
    glfwGetFramebufferSize(mainWindow, &BufferWidth, &BufferHeight);
    glfwMakeContextCurrent(mainWindow);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        printf("Falló inicialización de GLEW\n");
        glfwDestroyWindow(mainWindow);
        glfwTerminate();
        return 1;
    }

    glViewport(0, 0, BufferWidth, BufferHeight);
    printf("Version de Opengl: %s \n", glGetString(GL_VERSION));
    printf("Marca: %s \n", glGetString(GL_VENDOR));
    printf("Renderer: %s \n", glGetString(GL_RENDERER));
    printf("Shaders: %s \n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    // -------------------------------------------------------------------------
    // 2. Compilación de Shaders
    // -------------------------------------------------------------------------
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // -------------------------------------------------------------------------
    // 3. Definición de Geometría (Cuadrado y Rombo)
    // -------------------------------------------------------------------------

    // CUADRADO (Izquierda: X de -0.8 a -0.2)
    GLfloat verticesCuadrado[] = {
        -0.8f, -0.3f, 0.0f, // 0: Abajo-Izq
        -0.2f, -0.3f, 0.0f, // 1: Abajo-Der
        -0.2f,  0.3f, 0.0f, // 2: Arriba-Der
        -0.8f,  0.3f, 0.0f  // 3: Arriba-Izq
    };
    unsigned int indicesCuadrado[] = {
        0, 1, 2,
        2, 3, 0
    };

    unsigned int VAO_Cuadrado, VBO_Cuadrado, EBO_Cuadrado;
    glGenVertexArrays(1, &VAO_Cuadrado);
    glGenBuffers(1, &VBO_Cuadrado);
    glGenBuffers(1, &EBO_Cuadrado);

    glBindVertexArray(VAO_Cuadrado);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Cuadrado);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCuadrado), verticesCuadrado, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Cuadrado);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesCuadrado), indicesCuadrado, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // ROMBO (Derecha: Centro en X = 0.5)
    GLfloat verticesRombo[] = {
         0.2f,  0.0f, 0.0f, // 0: Izquierda
         0.5f,  0.4f, 0.0f, // 1: Arriba
         0.8f,  0.0f, 0.0f, // 2: Derecha
         0.5f, -0.4f, 0.0f  // 3: Abajo
    };
    unsigned int indicesRombo[] = {
        0, 1, 2,
        2, 3, 0
    };

    unsigned int VAO_Rombo, VBO_Rombo, EBO_Rombo;
    glGenVertexArrays(1, &VAO_Rombo);
    glGenBuffers(1, &VBO_Rombo);
    glGenBuffers(1, &EBO_Rombo);

    glBindVertexArray(VAO_Rombo);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Rombo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesRombo), verticesRombo, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Rombo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesRombo), indicesRombo, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // -------------------------------------------------------------------------
    // 4. Bucle Principal
    // -------------------------------------------------------------------------
    float intervaloColor = 1.0f; // Cambia cada 1.0 segundo

    while (!glfwWindowShouldClose(mainWindow))
    {
        glfwPollEvents();

        // Control cíclico del fondo: Rojo -> Verde -> Azul
        double tiempo = glfwGetTime();
        int estado = (int)(tiempo / intervaloColor) % 3;

        if (estado == 0)
        {
            glClearColor(1.0f, 0.0f, 0.0f, 1.0f); // Rojo
        }
        else if (estado == 1)
        {
            glClearColor(0.0f, 1.0f, 0.0f, 1.0f); // Verde
        }
        else
        {
            glClearColor(0.0f, 0.0f, 1.0f, 1.0f); // Azul
        }

        glClear(GL_COLOR_BUFFER_BIT);

        // Usar programa de shaders
        glUseProgram(shaderProgram);

        // Dibujar Cuadrado
        glBindVertexArray(VAO_Cuadrado);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Dibujar Rombo
        glBindVertexArray(VAO_Rombo);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);

        glfwSwapBuffers(mainWindow);
    }

    // Liberar recursos
    glDeleteVertexArrays(1, &VAO_Cuadrado);
    glDeleteBuffers(1, &VBO_Cuadrado);
    glDeleteBuffers(1, &EBO_Cuadrado);

    glDeleteVertexArrays(1, &VAO_Rombo);
    glDeleteBuffers(1, &VBO_Rombo);
    glDeleteBuffers(1, &EBO_Rombo);

    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(mainWindow);
    glfwTerminate();
    return 0;
}