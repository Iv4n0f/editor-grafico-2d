// =====================================================================
// PRACTICA INTEGRADORA - Computacion Grafica
// Rasterizacion (Bresenham) + Transformaciones homogeneas + Relleno ET/EAT
// =====================================================================
// Controles:
//   Click izquierdo  -> agregar vertice
//   Click derecho    -> cerrar poligono
//   1, 2, 3...       -> seleccionar poligono activo
//   D / I            -> rotar 5 grados derecha / izquierda
//   S / s            -> escalar +10% / -10%
//   Flechas          -> trasladar (5 px por pulsacion, 20 con Shift)
//   r / g / b        -> cambiar color de relleno del activo
//   P                -> rellenar poligono activo
//   C                -> limpiar todo
//   ESC              -> salir
// =====================================================================

#include <GL/glut.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace std;

const int ANCHO = 800;
const int ALTO = 600;
const float PI = 3.14159265f;

// =====================================================================
//  ESTRUCTURAS
// =====================================================================
struct Punto {
  float x;
  float y;
};

struct Poligono {
  vector<Punto> P;
  bool cerrado;
  bool relleno;
  float R, G, B; // color de relleno del poligono

  Poligono() : cerrado(false), relleno(false), R(1.0f), G(0.0f), B(0.0f) {}
};

vector<Poligono> poligonos;
int poligonoActual = 0; // indice del poligono en construccion
int poligonoActivo = 0; // indice del poligono seleccionado
Punto Pmouse;

// =====================================================================
//  MATRICES HOMOGENEAS 3x3
// =====================================================================
void identidad(float M[3][3]) {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      M[i][j] = (i == j) ? 1.0f : 0.0f;
}

void multiplicar(float A[3][3], float B[3][3], float C[3][3]) {
  float aux[3][3];
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) {
      aux[i][j] = 0;
      for (int k = 0; k < 3; k++)
        aux[i][j] += A[i][k] * B[k][j];
    }

  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      C[i][j] = aux[i][j];
}

void matrizTraslacion(float tx, float ty, float T[3][3]) {
  identidad(T);
  T[0][2] = tx;
  T[1][2] = ty;
}

void matrizRotacion(float angulo, float R[3][3]) {
  identidad(R);
  float rad = angulo * PI / 180.0f;
  R[0][0] = cos(rad);
  R[0][1] = -sin(rad);
  R[1][0] = sin(rad);
  R[1][1] = cos(rad);
}

void matrizEscala(float sx, float sy, float S[3][3]) {
  identidad(S);
  S[0][0] = sx;
  S[1][1] = sy;
}

Punto transformarPunto(Punto P, float M[3][3]) {
  Punto P2;
  P2.x = M[0][0] * P.x + M[0][1] * P.y + M[0][2];
  P2.y = M[1][0] * P.x + M[1][1] * P.y + M[1][2];
  return P2;
}

void transformar(Poligono &pol, float M[3][3]) {
  for (int i = 0; i < (int)pol.P.size(); i++)
    pol.P[i] = transformarPunto(pol.P[i], M);
}

Punto centroPoligono(Poligono &pol) {
  Punto C = {0, 0};
  if (pol.P.empty())
    return C;

  for (int i = 0; i < (int)pol.P.size(); i++) {
    C.x += pol.P[i].x;
    C.y += pol.P[i].y;
  }
  C.x /= pol.P.size();
  C.y /= pol.P.size();
  return C;
}

// ---------------------------------------------------------------------
//  Rotar respecto al centroide:  M = T(C) * R(?) * T(-C)
// ---------------------------------------------------------------------
void rotar(Poligono &pol, float angulo) {
  Punto C = centroPoligono(pol);

  float T1[3][3], R[3][3], T2[3][3];
  float M1[3][3], M[3][3];

  matrizTraslacion(-C.x, -C.y, T1);
  matrizRotacion(angulo, R);
  matrizTraslacion(C.x, C.y, T2);

  multiplicar(R, T1, M1);
  multiplicar(T2, M1, M);

  transformar(pol, M);
}

// ---------------------------------------------------------------------
//  Escalar respecto al centroide:  M = T(C) * S(sx,sy) * T(-C)
// ---------------------------------------------------------------------
void escalar(Poligono &pol, float factor) {
  Punto C = centroPoligono(pol);

  float T1[3][3], S[3][3], T2[3][3];
  float M1[3][3], M[3][3];

  matrizTraslacion(-C.x, -C.y, T1);
  matrizEscala(factor, factor, S);
  matrizTraslacion(C.x, C.y, T2);

  multiplicar(S, T1, M1);
  multiplicar(T2, M1, M);

  transformar(pol, M);
}

// ---------------------------------------------------------------------
//  TRASLADAR (trabajo del estudiante)
//  M = T(dx, dy)
// ---------------------------------------------------------------------
void trasladar(Poligono &pol, float dx, float dy) {
  float T[3][3];
  matrizTraslacion(dx, dy, T);
  transformar(pol, T);
}

// =====================================================================
//  BRESENHAM (funciona para cualquier pendiente)
// =====================================================================
void Bresenham(int x1, int y1, int x2, int y2) {
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;
  int p = dx - dy;

  while (true) {
    glVertex2i(x1, y1);

    if (x1 == x2 && y1 == y2)
      break;

    int p2 = 2 * p;

    if (p2 > -dy) {
      p -= dy;
      x1 += sx;
    }
    if (p2 < dx) {
      p += dx;
      y1 += sy;
    }
  }
}

// =====================================================================
//  RELLENO SCAN-LINE  (ET + EAT)
//  Adaptado de Relleno.cpp para trabajar con el poligono activo
// =====================================================================
struct Arista {
  int ymin;
  int ymax;
  float x;
  float invM;
};

void rellenarPoligono(Poligono &pol) {
  if (pol.P.size() < 3)
    return;

  // --- Construir ET (Edge Table) indexada por ymin ---
  vector<vector<Arista>> ET(ALTO);

  for (int i = 0; i < (int)pol.P.size(); i++) {
    Punto P1 = pol.P[i];
    Punto P2 = pol.P[(i + 1) % pol.P.size()];

    // Las aristas horizontales no entran al ET
    if ((int)P1.y == (int)P2.y)
      continue;

    Arista A;

    if (P1.y < P2.y) {
      A.ymin = (int)ceil(P1.y);
      A.ymax = (int)ceil(P2.y) - 1;
      A.x = P1.x;
      A.invM = (P2.x - P1.x) / (P2.y - P1.y);
    } else {
      A.ymin = (int)ceil(P2.y);
      A.ymax = (int)ceil(P1.y) - 1;
      A.x = P2.x;
      A.invM = (P1.x - P2.x) / (P1.y - P2.y);
    }

    if (A.ymin >= 0 && A.ymin < ALTO)
      ET[A.ymin].push_back(A);
  }

  // --- Rango vertical ---
  int ymin = ALTO, ymax = 0;
  for (int i = 0; i < (int)pol.P.size(); i++) {
    ymin = min(ymin, (int)pol.P[i].y);
    ymax = max(ymax, (int)pol.P[i].y);
  }
  ymin = max(ymin, 0);
  ymax = min(ymax, ALTO - 1);

  // --- Barrido ---
  vector<Arista> EAT;

  glColor3f(pol.R, pol.G, pol.B);
  glBegin(GL_POINTS);

  for (int y = ymin; y <= ymax; y++) {
    // 1) Trasladar aristas de ET a EAT
    for (int i = 0; i < (int)ET[y].size(); i++)
      EAT.push_back(ET[y][i]);

    // 2) Retirar aristas cuya ymax < y
    EAT.erase(
        remove_if(EAT.begin(), EAT.end(), [y](Arista A) { return y > A.ymax; }),
        EAT.end());

    // 3) Ordenar por x
    sort(EAT.begin(), EAT.end(), [](Arista A, Arista B) { return A.x < B.x; });

    // 4) Pintar entre pares (regla de paridad)
    for (int i = 0; i + 1 < (int)EAT.size(); i += 2) {
      int x1 = (int)ceil(EAT[i].x);
      int x2 = (int)floor(EAT[i + 1].x);

      for (int x = x1; x <= x2; x++)
        glVertex2i(x, y);
    }

    // 5) Actualizar x para la siguiente linea
    for (int i = 0; i < (int)EAT.size(); i++)
      EAT[i].x += EAT[i].invM;
  }

  glEnd();
}

// =====================================================================
//  DIBUJO DEL POLIGONO (contorno con Bresenham)
// =====================================================================
void dibujarPoligono(Poligono &pol, float r, float g, float b) {
  glColor3f(r, g, b);
  glPointSize(2.0f);

  glBegin(GL_POINTS);

  for (int i = 0; i < (int)pol.P.size() - 1; i++) {
    Bresenham((int)round(pol.P[i].x), (int)round(pol.P[i].y),
              (int)round(pol.P[i + 1].x), (int)round(pol.P[i + 1].y));
  }

  if (pol.cerrado && pol.P.size() >= 3) {
    int n = pol.P.size();
    Bresenham((int)round(pol.P[n - 1].x), (int)round(pol.P[n - 1].y),
              (int)round(pol.P[0].x), (int)round(pol.P[0].y));
  }

  glEnd();
}

// =====================================================================
//  DISPLAY
// =====================================================================
void display() {
  glClear(GL_COLOR_BUFFER_BIT);

  // 1) Rellenar todos los poligonos que tengan relleno activo
  for (int i = 0; i < (int)poligonos.size(); i++) {
    if (poligonos[i].cerrado && poligonos[i].relleno)
      rellenarPoligono(poligonos[i]);
  }

  // 2) Dibujar contornos
  for (int i = 0; i < (int)poligonos.size(); i++) {
    if (i == poligonoActivo)
      dibujarPoligono(poligonos[i], 1.0f, 0.0f, 0.0f); // activo = rojo
    else
      dibujarPoligono(poligonos[i], 0.0f, 0.0f, 0.0f); // resto = negro
  }

  // 3) Previsualizar la arista que sigue el mouse (mientras construimos)
  if (poligonoActual < (int)poligonos.size()) {
    Poligono &pol = poligonos[poligonoActual];

    if (!pol.cerrado && pol.P.size() > 0) {
      int n = pol.P.size();
      glColor3f(0.5f, 0.5f, 0.5f);
      glBegin(GL_POINTS);
      Bresenham((int)round(pol.P[n - 1].x), (int)round(pol.P[n - 1].y),
                (int)Pmouse.x, (int)Pmouse.y);
      glEnd();
    }
  }

  glutSwapBuffers();
}

// =====================================================================
//  MOUSE
// =====================================================================
void mouse(int button, int state, int x, int y) {
  if (state != GLUT_DOWN)
    return;

  y = ALTO - y;

  if (button == GLUT_LEFT_BUTTON) {
    if (poligonoActual >= (int)poligonos.size())
      return;

    Poligono &pol = poligonos[poligonoActual];
    if (pol.cerrado)
      return;

    Punto Pi;
    Pi.x = (float)x;
    Pi.y = (float)y;
    pol.P.push_back(Pi);

    cout << "Poligono " << poligonoActual + 1 << " - P" << pol.P.size()
         << " = (" << Pi.x << ", " << Pi.y << ")" << endl;

    glutPostRedisplay();
  }

  if (button == GLUT_RIGHT_BUTTON) {
    if (poligonoActual >= (int)poligonos.size())
      return;

    Poligono &pol = poligonos[poligonoActual];

    if (pol.P.size() < 3) {
      cout << "Se necesitan al menos 3 vertices." << endl;
      return;
    }

    pol.cerrado = true;
    cout << "Poligono " << poligonoActual + 1 << " cerrado." << endl;

    Poligono nuevo;
    poligonos.push_back(nuevo);
    poligonoActual = poligonos.size() - 1;
    poligonoActivo = poligonoActual;

    glutPostRedisplay();
  }
}

void movimiento(int x, int y) {
  Pmouse.x = (float)x;
  Pmouse.y = (float)(ALTO - y);
  glutPostRedisplay();
}

// =====================================================================
//  TECLADO
// =====================================================================
void teclado(unsigned char tecla, int x, int y) {
  // -------- Seleccion de poligono con 1,2,3,...,9 --------
  if (tecla >= '1' && tecla <= '9') {
    int idx = tecla - '1';
    if (idx < (int)poligonos.size() && poligonos[idx].cerrado) {
      poligonoActivo = idx;
      cout << "Poligono " << idx + 1 << " activo." << endl;
    }
  }

  // -------- Rotar --------
  if ((tecla == 'd' || tecla == 'D') && poligonos[poligonoActivo].cerrado)
    rotar(poligonos[poligonoActivo], -5.0f);

  if ((tecla == 'i' || tecla == 'I') && poligonos[poligonoActivo].cerrado)
    rotar(poligonos[poligonoActivo], 5.0f);

  // -------- Escalar --------
  if (tecla == 'S' && poligonos[poligonoActivo].cerrado)
    escalar(poligonos[poligonoActivo], 1.10f);

  if (tecla == 's' && poligonos[poligonoActivo].cerrado)
    escalar(poligonos[poligonoActivo], 0.90f);

  // -------- Color de relleno del activo --------
  if (tecla == 'r') {
    poligonos[poligonoActivo].R = 1;
    poligonos[poligonoActivo].G = 0;
    poligonos[poligonoActivo].B = 0;
  }
  if (tecla == 'g') {
    poligonos[poligonoActivo].R = 0;
    poligonos[poligonoActivo].G = 1;
    poligonos[poligonoActivo].B = 0;
  }
  if (tecla == 'b') {
    poligonos[poligonoActivo].R = 0;
    poligonos[poligonoActivo].G = 0;
    poligonos[poligonoActivo].B = 1;
  }

  // -------- Rellenar el poligono activo --------
  if (tecla == 'p' || tecla == 'P') {
    if (poligonos[poligonoActivo].cerrado) {
      poligonos[poligonoActivo].relleno = true;
      cout << "Poligono " << poligonoActivo + 1 << " rellenado." << endl;
    }
  }

  // -------- Limpiar --------
  if (tecla == 'c' || tecla == 'C') {
    poligonos.clear();
    Poligono nuevo;
    poligonos.push_back(nuevo);
    poligonoActual = 0;
    poligonoActivo = 0;
  }

  if (tecla == 27)
    exit(0);

  glutPostRedisplay();
}

// ---------------------------------------------------------------------
//  Teclas especiales: flechas para TRASLADAR
// ---------------------------------------------------------------------
void tecladoEspecial(int tecla, int x, int y) {
  const float paso = 5.0f;

  if (!poligonos[poligonoActivo].cerrado)
    return;

  switch (tecla) {
  case GLUT_KEY_LEFT:
    trasladar(poligonos[poligonoActivo], -paso, 0);
    break;
  case GLUT_KEY_RIGHT:
    trasladar(poligonos[poligonoActivo], paso, 0);
    break;
  case GLUT_KEY_UP:
    trasladar(poligonos[poligonoActivo], 0, paso);
    break;
  case GLUT_KEY_DOWN:
    trasladar(poligonos[poligonoActivo], 0, -paso);
    break;
  }

  glutPostRedisplay();
}

// =====================================================================
//  INICIALIZACION
// =====================================================================
void inicializar() {
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, ANCHO, 0, ALTO);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  Poligono nuevo;
  poligonos.push_back(nuevo);
}

// =====================================================================
//  MAIN
// =====================================================================
int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(ANCHO, ALTO);
  glutInitWindowPosition(100, 100);
  glutCreateWindow("Practica Integradora - Bresenham + Homogeneas + ET/EAT");

  inicializar();

  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutPassiveMotionFunc(movimiento);
  glutKeyboardFunc(teclado);
  glutSpecialFunc(tecladoEspecial);

  glutMainLoop();
  return 0;
}
