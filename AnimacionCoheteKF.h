#pragma once
#include <cstdio>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <initializer_list>
#include <glew.h>
#include <glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "Model.h"

namespace AnimacionCoheteKF {

	// ------------------------------------------------------------------ modelos
	Model Fase1_M, Fase2_M, Capsula_M;
	struct Pieza { Model* modelo; glm::vec3 centro; float radio; float desfase; };
	Pieza piezas[3];                   // 0 = base (fase 1), 1 = media (fase 2), 2 = punta (capsula)
	bool piezasListas = false;
	bool origenCompartido = true;      // true si las piezas conservan su posicion en el cohete armado
	const float ESCALA = 5.0f;

	// ------------------------------------------------------------------ keyframes
	enum Canal { F1Y, F2Y, CAPY, F1X, F1ROTZ, F2X, F2ROTZ, CAPX, CAPZ, CAPROTY, CAPROTZ, NUM_CANALES };
	struct Frame { float v[NUM_CANALES]; };

	constexpr int MAX_FRAMES = 40;
	Frame KeyFrame[MAX_FRAMES];
	int FrameIndex = 0;
	int playIndex = 0;
	float tSeg = 0.0f;                 // segundos transcurridos dentro del segmento actual
	bool play = false;
	int idxOrbita = 0;                 // primer keyframe donde la capsula ya esta orbitando

	const float DUR_SEG = 2.2f;        // segundos entre keyframes de ascenso/separacion
	const float DUR_ORBITA = 1.6f;     // segundos por cada tramo de orbita
	const float RADIO_ORBITA = 18.0f;

	// Estado actual de las 3 fases
	float f1Y = 0.0f, f2Y = 0.0f, capY = 0.0f;
	float f1X = 0.0f, f1RotZ = 0.0f;
	float f2X = 0.0f, f2RotZ = 0.0f;
	float capX = 0.0f, capZ = 0.0f, capRotY = 0.0f, capRotZ = 0.0f;

	bool orbitaFinal = false;          // despues del ultimo keyframe la capsula sigue orbitando
	float orbAng = 0.0f;               // angulo de la orbita en grados

	bool tEspacio = false, tG = false, tC = false, tK = false, tP = false;
	const char* ARCHIVO_KF = "keyframes_cohete.txt";

	// ------------------------------------------------------------------ utilidades
	void asignar(const float* r)
	{
		f1Y = (std::max)(r[F1Y], 0.0f);          // las fases no atraviesan el piso
		f2Y = (std::max)(r[F2Y], 0.0f);
		capY = r[CAPY];
		f1X = r[F1X];  f1RotZ = r[F1ROTZ];
		f2X = r[F2X];  f2RotZ = r[F2ROTZ];
		capX = r[CAPX];  capZ = r[CAPZ];  capRotY = r[CAPROTY];  capRotZ = r[CAPROTZ];
	}

	float catmull(float p0, float p1, float p2, float p3, float t)
	{
		return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t * t
			+ (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t * t * t);
	}

	void poseEn(int i, float t)
	{
		const Frame& a = KeyFrame[(std::max)(i - 1, 0)];
		const Frame& b = KeyFrame[i];
		const Frame& c = KeyFrame[(std::min)(i + 1, FrameIndex - 1)];
		const Frame& d = KeyFrame[(std::min)(i + 2, FrameIndex - 1)];
		float r[NUM_CANALES];
		for (int k = 0; k < NUM_CANALES; ++k) r[k] = catmull(a.v[k], b.v[k], c.v[k], d.v[k], t);
		asignar(r);
	}

	float duracionSegmento(int i) { return (i >= idxOrbita) ? DUR_ORBITA : DUR_SEG; }

	void imprimirFrameConsola(int i, const char* accion)
	{
		const float* v = KeyFrame[i].v;
		printf("[%s] KeyFrame[%d] -> f1Y:%.2f f2Y:%.2f capY:%.2f | f1X:%.2f f1RotZ:%.2f | f2X:%.2f f2RotZ:%.2f | capX:%.2f capZ:%.2f capRotY:%.2f capRotZ:%.2f\n",
			accion, i, v[F1Y], v[F2Y], v[CAPY], v[F1X], v[F1ROTZ], v[F2X], v[F2ROTZ], v[CAPX], v[CAPZ], v[CAPROTY], v[CAPROTZ]);
	}

	// ------------------------------------------------------------------ archivo txt
	void guardarArchivoTXT()
	{
		std::ofstream out(ARCHIVO_KF);                 // sobreescribe
		if (!out.is_open()) { printf("[ERROR] No se pudo abrir %s para guardar.\n", ARCHIVO_KF); return; }
		printf("\n========== GUARDANDO KEYFRAMES EN %s ==========\n", ARCHIVO_KF);
		for (int i = 0; i < FrameIndex; ++i)
		{
			out << i;
			for (int c = 0; c < NUM_CANALES; ++c) out << " " << KeyFrame[i].v[c];
			out << "\n";
			imprimirFrameConsola(i, "GUARDADO TXT");
		}
		out.close();
		printf("========== TOTAL GUARDADOS: %d KEYFRAMES ==========\n\n", FrameIndex);
	}

	bool cargarArchivoTXT()
	{
		std::ifstream in(ARCHIVO_KF);
		if (!in.is_open()) return false;
		FrameIndex = 0;
		int idx = 0;
		printf("\n========== CARGANDO KEYFRAMES DESDE %s ==========\n", ARCHIVO_KF);
		while (FrameIndex < MAX_FRAMES && (in >> idx))
		{
			Frame& k = KeyFrame[FrameIndex];
			bool ok = true;
			for (int c = 0; c < NUM_CANALES; ++c)
				if (!(in >> k.v[c])) { ok = false; break; }
			if (!ok) break;
			imprimirFrameConsola(FrameIndex, "CARGADO TXT");
			FrameIndex++;
		}
		in.close();
		if (FrameIndex > 0) asignar(KeyFrame[0].v);
		printf("========== TOTAL CARGADOS: %d KEYFRAMES ==========\n\n", FrameIndex);
		return FrameIndex > 1;
	}

	void cargarFramesPorDefecto()
	{
		// Alturas calibradas (max 46 en Y) para que la separacion de Fase 1, Fase 2 y la orbita
		// de la capsula ocurran completamente dentro del encuadre de la camara.
		const float datos[10][NUM_CANALES] = {
			// f1Y,   f2Y,   capY,    f1X,  f1RotZ,  f2X,  f2RotZ,   capX,   capZ, capRotY, capRotZ
			{ 0.0f,   0.0f,   0.0f,   0.0f,   0.0f,  0.0f,   0.0f,   0.0f,   0.0f,    0.0f,   0.0f }, // 0: En plataforma
			{ 12.0f,  12.0f,  12.0f,   0.0f,   0.0f,  0.0f,   0.0f,   0.0f,   0.0f,    0.0f,   0.0f }, // 1: Despegue conjunto
			{ 18.0f,  24.0f,  24.0f,  -8.0f,  35.0f,  0.0f,   0.0f,   0.0f,   0.0f,    0.0f,   0.0f }, // 2: SE SEPARA FASE 1 a baja altura
			{ 6.0f,  32.0f,  36.0f, -18.0f,  75.0f,  4.0f, -15.0f,   0.0f,   6.0f,    0.0f, -15.0f }, // 3: Fase 1 cae a la izq, SE SEPARA FASE 2
			{ 0.0f,  22.0f,  44.0f, -26.0f,  90.0f, 14.0f, -50.0f,   0.0f,  18.0f,    0.0f, -45.0f }, // 4: F1 en el suelo, F2 cayendo a la der, Capsula sube
			{ 0.0f,   5.0f,  46.0f, -28.0f,  90.0f, 24.0f, -85.0f,  18.0f,   0.0f,   90.0f, -90.0f }, // 5: F2 casi en suelo, Capsula entra en orbita (90)
			{ 0.0f,   0.0f,  46.0f, -28.0f,  90.0f, 28.0f, -90.0f,   0.0f, -18.0f,  180.0f, -90.0f }, // 6: F2 en el suelo, Capsula orbitando (180)
			{ 0.0f,   0.0f,  46.0f, -28.0f,  90.0f, 28.0f, -90.0f, -18.0f,   0.0f,  270.0f, -90.0f }, // 7: Capsula orbitando (270)
			{ 0.0f,   0.0f,  46.0f, -28.0f,  90.0f, 28.0f, -90.0f,   0.0f,  18.0f,  360.0f, -90.0f }, // 8: Capsula completa 1ra vuelta (360)
			{ 0.0f,   0.0f,  46.0f, -28.0f,  90.0f, 28.0f, -90.0f,  18.0f,   0.0f,  450.0f, -90.0f }  // 9: Capsula sigue orbitando (450)
		};
		FrameIndex = 10;
		for (int i = 0; i < FrameIndex; ++i)
			for (int c = 0; c < NUM_CANALES; ++c)
				KeyFrame[i].v[c] = datos[i][c];
		asignar(KeyFrame[0].v);
		guardarArchivoTXT();
	}

	bool intentarCargar(Model& m, std::initializer_list<const char*> rutas)
	{
		for (const char* r : rutas)
		{
			std::ifstream f(r, std::ios::binary);
			if (f.good())
			{
				m.LoadModel(r);
				if (m.cargado())
				{
					printf("[COHETE] Modelo cargado exitosamente: %s (Y min:%.2f, max:%.2f)\n",
						r, static_cast<float>(m.minimo.y), static_cast<float>(m.maximo.y));
					return true;
				}
			}
		}
		return false;
	}

	Pieza construirPieza(Model* m, float desfaseY)
	{
		glm::vec3 minV(static_cast<float>(m->minimo.x), static_cast<float>(m->minimo.y), static_cast<float>(m->minimo.z));
		glm::vec3 maxV(static_cast<float>(m->maximo.x), static_cast<float>(m->maximo.y), static_cast<float>(m->maximo.z));
		glm::vec3 centro = (minV + maxV) * 0.5f;
		float radio = 0.25f * ((maxV.x - minV.x) + (maxV.z - minV.z));
		return { m, centro, radio, desfaseY };
	}

	void iniciar()
	{
		for (int i = 0; i < MAX_FRAMES; ++i) KeyFrame[i] = Frame{};

		bool c1 = intentarCargar(Fase1_M, {
			"Models/saturne5P3.obj"
			});
		bool c2 = intentarCargar(Fase2_M, {
			"Models/saturne5P2.obj"
			});
		bool c3 = intentarCargar(Capsula_M, {
			"Models/saturne5P1.obj"
			});

		if (c1 && c2 && c3)
		{
			float cy1 = 0.5f * (static_cast<float>(Fase1_M.minimo.y) + static_cast<float>(Fase1_M.maximo.y));
			float cy3 = 0.5f * (static_cast<float>(Capsula_M.minimo.y) + static_cast<float>(Capsula_M.maximo.y));
			if (cy1 > cy3 + 0.1f)
			{
				std::swap(Fase1_M, Capsula_M);
				std::swap(cy1, cy3);
				printf("[COHETE] Auto-ordenado por altura: saturne5P3 = Base (Fase 1), saturne5P2 = Media (Fase 2), saturne5P1 = Capsula (Punta)\n");
			}
			else
			{
				printf("[COHETE] Orden detectado: saturne5P1 = Base (Fase 1), saturne5P2 = Media (Fase 2), saturne5P3 = Capsula (Punta)\n");
			}

			origenCompartido = (std::abs(cy1 - cy3) > 0.2f);
			float h1 = static_cast<float>(Fase1_M.maximo.y) - static_cast<float>(Fase1_M.minimo.y);
			float h2 = static_cast<float>(Fase2_M.maximo.y) - static_cast<float>(Fase2_M.minimo.y);

			piezas[0] = construirPieza(&Fase1_M, 0.0f);
			piezas[1] = construirPieza(&Fase2_M, origenCompartido ? 0.0f : h1);
			piezas[2] = construirPieza(&Capsula_M, origenCompartido ? 0.0f : (h1 + h2));
			piezasListas = true;
			printf("[COHETE] 3 piezas Saturn V listas (piezasListas=1, origenCompartido=%d)\n", origenCompartido ? 1 : 0);
		}
		else
		{
			piezasListas = false;
			printf("[AVISO COHETE] No se encontraron los 3 OBJs en Models/ (P1=%d, P2=%d, P3=%d). Se dividira coheteCompleto en 3 fases.\n", c1, c2, c3);
		}

		// Si no existe el archivo TXT o si tiene los valores antiguos fuera de camara (capY > 100),
		// regenera automaticamente el TXT con las alturas visibles en pantalla.
		if (!cargarArchivoTXT() || KeyFrame[FrameIndex - 1].v[CAPY] > 100.0f)
		{
			cargarFramesPorDefecto();
		}
		printf("Controles Keyframes Cohete: [ESPACIO] Reproducir | [K] Capturar Frame | [G] Guardar TXT | [C] Cargar TXT | [P] Imprimir poses\n");
	}

	// ------------------------------------------------------------------ edicion / reproduccion
	void capturarFrameActual()
	{
		if (FrameIndex >= MAX_FRAMES) { printf("[AVISO] Limite de %d KeyFrames alcanzado.\n", MAX_FRAMES); return; }
		float* v = KeyFrame[FrameIndex].v;
		v[F1Y] = f1Y;  v[F2Y] = f2Y;  v[CAPY] = capY;
		v[F1X] = f1X;  v[F1ROTZ] = f1RotZ;
		v[F2X] = f2X;  v[F2ROTZ] = f2RotZ;
		v[CAPX] = capX;  v[CAPZ] = capZ;  v[CAPROTY] = capRotY;  v[CAPROTZ] = capRotZ;
		imprimirFrameConsola(FrameIndex, "CAPTURADO EN MEMORIA");
		FrameIndex++;
	}

	// Para que la camara del recorrido se reubique suave al iniciar
	bool camaraReinicia = true;

	void iniciarReproduccion()
	{
		if (FrameIndex <= 1) return;
		idxOrbita = FrameIndex;
		for (int i = 0; i < FrameIndex; ++i)
			if (KeyFrame[i].v[CAPROTZ] <= -89.0f) { idxOrbita = i; break; }
		asignar(KeyFrame[0].v);
		playIndex = 0;
		tSeg = 0.0f;
		orbitaFinal = false;
		camaraReinicia = true;
		play = true;
		printf("\n>>> INICIANDO REPRODUCCION DE ANIMACION POR KEYFRAMES (Total Frames: %d) <<<\n", FrameIndex);
		imprimirFrameConsola(0, "REPRODUCIENDO FRAME INICIAL");
	}

	void controlarTeclado(bool* keys)
	{
		if (keys[GLFW_KEY_SPACE]) { if (!tEspacio) { iniciarReproduccion(); tEspacio = true; } }
		else tEspacio = false;
		if (keys[GLFW_KEY_K]) { if (!tK) { capturarFrameActual(); tK = true; } }
		else tK = false;
		if (keys[GLFW_KEY_G]) { if (!tG) { guardarArchivoTXT(); tG = true; } }
		else tG = false;
		if (keys[GLFW_KEY_C]) { if (!tC) { cargarArchivoTXT(); tC = true; } }
		else tC = false;
		if (keys[GLFW_KEY_P])
		{
			if (!tP)
			{
				printf("\n========== IMPRESION DE KEYFRAMES EN CONSOLA ==========\n");
				for (int i = 0; i < FrameIndex; ++i) imprimirFrameConsola(i, "CONSOLA");
				printf("=======================================================\n\n");
				tP = true;
			}
		}
		else tP = false;
	}

	// Se llama una vez por cuadro (no hace falta pasarle nada: usa su propio reloj en segundos)
	void animar()
	{
		static double ultimo = glfwGetTime();
		double ahora = glfwGetTime();
		float dt = static_cast<float>(ahora - ultimo);
		ultimo = ahora;
		if (dt > 0.1f) dt = 0.1f;
		if (dt < 0.0f) dt = 0.0f;

		if (play)
		{
			tSeg += dt;
			while (tSeg >= duracionSegmento(playIndex))
			{
				tSeg -= duracionSegmento(playIndex);
				playIndex++;
				if (playIndex > FrameIndex - 2)
				{
					asignar(KeyFrame[FrameIndex - 1].v);
					play = false;
					orbitaFinal = (capY > 25.0f);
					orbAng = std::atan2(capZ, capX) * 180.0f / 3.14159265f;
					printf(">>> FIN DE LA ANIMACION (la capsula queda orbitando, las fases 1 y 2 en el suelo) <<<\n\n");
					return;
				}
				imprimirFrameConsola(playIndex, "REPRODUCIENDO SIGUIENTE KEYFRAME");
			}
			poseEn(playIndex, tSeg / duracionSegmento(playIndex));
		}
		else if (orbitaFinal)
		{
			// La capsula sigue en orbita, continuando justo donde termino el ultimo keyframe
			orbAng -= (45.0f / DUR_ORBITA) * dt;
			float rad = orbAng * 3.14159265f / 180.0f;
			capX = RADIO_ORBITA * std::cos(rad);
			capZ = RADIO_ORBITA * std::sin(rad);
			capRotY = 90.0f - orbAng;
		}
	}

	// ------------------------------------------------------------------ camara del recorrido
	// Sigue la accion: se aleja un poco cuando las fases se separan para que se vean todas.
	void enfoqueCamara(const glm::vec3& posBase, glm::vec3& lookAt, float& radio)
	{
		static float ySuave = 0.0f, rSuave = 75.0f;
		static double ultimo = glfwGetTime();
		double ahora = glfwGetTime();
		float dt = static_cast<float>(ahora - ultimo);
		ultimo = ahora;
		if (dt > 0.1f) dt = 0.1f;
		if (dt < 0.0f) dt = 0.0f;

		float arriba = capY, abajo = capY;                    // fases que siguen en el aire
		if (f1Y > 3.0f) abajo = (std::min)(abajo, f1Y);
		if (f2Y > 3.0f) abajo = (std::min)(abajo, f2Y);
		float yObj = posBase.y + 0.5f * (arriba + abajo) + 12.0f;
		float rObj = 75.0f + 0.9f * (arriba - abajo);

		if (camaraReinicia) { ySuave = yObj; rSuave = rObj; camaraReinicia = false; }
		float k = (std::min)(1.0f, 2.5f * dt);
		ySuave += (yObj - ySuave) * k;
		rSuave += (rObj - rSuave) * k;

		lookAt = glm::vec3(posBase.x, ySuave, posBase.z);
		radio = rSuave;
	}

	// ------------------------------------------------------------------ dibujo
	void dibujarPieza(const Pieza& p, const glm::vec3& pos, float giroY, float rotZ, float ajusteSuelo, GLuint uModel)
	{
		glm::mat4 m = glm::translate(glm::mat4(1.0f), pos + glm::vec3(0.0f, ajusteSuelo, 0.0f));
		m = glm::scale(m, glm::vec3(ESCALA));
		m = glm::translate(m, glm::vec3(0.0f, p.desfase, 0.0f));
		m = glm::translate(m, p.centro);                    // gira sobre su propio centro
		m = glm::rotate(m, glm::radians(giroY), glm::vec3(0.0f, 1.0f, 0.0f));
		m = glm::rotate(m, glm::radians(rotZ), glm::vec3(0.0f, 0.0f, 1.0f));
		m = glm::translate(m, -p.centro);
		glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(m));
		p.modelo->RenderModel();
	}

	// Si una fase cae y se acuesta, baja su centro para quedar apoyada en el piso
	float ajusteSueloPieza(const Pieza& p, float rotZ)
	{
		float t = (std::min)(1.0f, std::abs(rotZ) / 90.0f);
		return -((p.centro.y + p.desfase) - p.radio) * ESCALA * t;
	}

	void dibujar(glm::vec3 posBase, GLuint uModel, GLuint uColor, Model& coheteCompleto)
	{
		glUniform3f(uColor, 1.0f, 1.0f, 1.0f);

		if (!piezasListas)
		{
			// Si por alguna razon no cargaron los 3 OBJs por separado, dibuja 3 secciones independientes
			// usando coheteCompleto para que SIEMPRE se vea la separacion de fases y la orbita
			if (coheteCompleto.cargado())
			{
				// Fase 1
				glm::mat4 m1 = glm::translate(glm::mat4(1.0f), posBase + glm::vec3(f1X, f1Y, 0.0f));
				m1 = glm::rotate(m1, glm::radians(f1RotZ), glm::vec3(0.0f, 0.0f, 1.0f));
				m1 = glm::scale(m1, glm::vec3(ESCALA * 0.85f, ESCALA * 0.45f, ESCALA * 0.85f));
				glUniform3f(uColor, 0.85f, 0.85f, 0.90f);
				glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(m1));
				coheteCompleto.RenderModel();

				// Fase 2
				glm::mat4 m2 = glm::translate(glm::mat4(1.0f), posBase + glm::vec3(f2X, f2Y + 8.0f, 0.0f));
				m2 = glm::rotate(m2, glm::radians(f2RotZ), glm::vec3(0.0f, 0.0f, 1.0f));
				m2 = glm::scale(m2, glm::vec3(ESCALA * 0.70f, ESCALA * 0.35f, ESCALA * 0.70f));
				glUniform3f(uColor, 0.95f, 0.55f, 0.15f);
				glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(m2));
				coheteCompleto.RenderModel();

				// Capsula
				glm::mat4 m3 = glm::translate(glm::mat4(1.0f), posBase + glm::vec3(capX, capY + 15.0f, capZ));
				m3 = glm::rotate(m3, glm::radians(capRotY), glm::vec3(0.0f, 1.0f, 0.0f));
				m3 = glm::rotate(m3, glm::radians(capRotZ), glm::vec3(0.0f, 0.0f, 1.0f));
				m3 = glm::scale(m3, glm::vec3(ESCALA * 0.55f, ESCALA * 0.30f, ESCALA * 0.55f));
				glUniform3f(uColor, 0.20f, 0.75f, 1.0f);
				glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(m3));
				coheteCompleto.RenderModel();
				glUniform3f(uColor, 1.0f, 1.0f, 1.0f);
			}
			return;
		}

		// 1. Fase 1 (base): despega con todo, se separa y cae a la izquierda
		dibujarPieza(piezas[0], posBase + glm::vec3(f1X, f1Y, 0.0f), 0.0f, f1RotZ, ajusteSueloPieza(piezas[0], f1RotZ), uModel);

		// 2. Fase 2 (media): sigue subiendo, se separa y cae a la derecha
		dibujarPieza(piezas[1], posBase + glm::vec3(f2X, f2Y, 0.0f), 0.0f, f2RotZ, ajusteSueloPieza(piezas[1], f2RotZ), uModel);

		// 3. Capsula (punta): llega arriba y se queda orbitando
		dibujarPieza(piezas[2], posBase + glm::vec3(capX, capY, capZ), capRotY, capRotZ, 0.0f, uModel);

		glUniform3f(uColor, 1.0f, 1.0f, 1.0f);
	}
}