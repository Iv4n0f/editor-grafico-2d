# Práctica Integradora - Computación Gráfica

Aplicación interactiva en C++ y OpenGL/GLUT que integra los tres pilares fundamentales de la computación gráfica 2D por software: **rasterizado manual de aristas (Bresenham)**, **transformaciones geométricas homogéneas ($3 \times 3$)** y **rasterizado manual de relleno (Scan-Line con ET y EAT)**.

---

## 🚀 Compilación y Ejecución

### Requisitos
* Compilador de C++ con soporte para C++17 (`g++`, `clang++` o MinGW en Windows).
* Bibliotecas de desarrollo de OpenGL y GLUT (`freeglut3-dev` en Linux / GLUT en Windows).

### Compilación (Linux)
```bash
g++ -std=c++17 main.cpp -o main -lglut -lGLU -lGL
```

### Ejecución
```bash
./main
```

---

## 🎮 Controles de Uso

| Acción / Control | Entrada | Descripción |
| :--- | :--- | :--- |
| **[Modo Dibujo] Agregar vértice** | `Clic Izquierdo` | Agrega un nuevo vértice al polígono en construcción. |
| **[Modo Dibujo] Cerrar polígono** | `Clic Derecho` | Cierra el polígono en construcción (mínimo 3 vértices) y lo deja activo. |
| **[Modo Selección] Seleccionar polígono** | `Clic Derecho` | Cuando no hay dibujo activo, selecciona el polígono bajo el cursor (prioriza el más frontal). |
| **Rotación** | Teclas `d` / `D` | Rota el polígono activo $-5^\circ$ (sentido horario) sobre su centroide. |
| | Teclas `i` / `I` | Rota el polígono activo $+5^\circ$ (sentido antihorario) sobre su centroide. |
| **Escalamiento** | Tecla `S` | Incrementa el tamaño del polígono activo en un $+10\%$ respecto a su centroide. |
| | Tecla `s` | Reduce el tamaño del polígono activo en un $-10\%$ respecto a su centroide. |
| **Traslación** | `Flechas` (←, →, ↑, ↓) | Desplaza el polígono activo $5\text{ px}$ en la dirección indicada. |
| **Canal Rojo (R)** | Tecla `R` (+0.2) / `r` (-0.2) | Ajusta el canal rojo del polígono activo y activa el relleno automáticamente. |
| **Canal Verde (G)** | Tecla `G` (+0.2) / `g` (-0.2) | Ajusta el canal verde del polígono activo y activa el relleno automáticamente. |
| **Canal Azul (B)** | Tecla `B` (+0.2) / `b` (-0.2) | Ajusta el canal azul del polígono activo y activa el relleno automáticamente. |
| **Vaciar relleno** | Tecla `v` / `V` | Desactiva el relleno del polígono activo (vuelve a modo alámbrico). |
| **Limpiar escena** | Tecla `c` / `C` | Elimina todos los polígonos y reinicia el lienzo. |
| **Salir** | Tecla `ESC` | Finaliza la aplicación. |

---

## 🧠 Algoritmos Implementados

Todo el renderizado se realiza mediante **rasterizado por software (manual)**: ningún segmento ni polígono utiliza primitivas de dibujo nativas de OpenGL; cada punto se calcula en la CPU y se dibuja individualmente con `glBegin(GL_POINTS)`.

### 1. Algoritmo de Bresenham (Rasterizado de líneas)
* **Función:** `Bresenham(x1, y1, x2, y2)`
* **Objetivo:** Trazar el contorno del polígono y la arista guía elástica del cursor.
* **Técnica:** Calcula los píxeles óptimos más cercanos a la recta matemática utilizando únicamente operaciones de suma y resta con variables de decisión enteras (`p = dx - dy`). Está generalizado para funcionar en cualquier octante (pendientes positivas, negativas, verticales y horizontales).

### 2. Transformaciones Geométricas 2D en Coordenadas Homogéneas
* **Estructura matemática:** Matrices de transformación $3 \times 3$.
* **Centroide ($C$):** Se calcula el punto medio promediando los vértices:
  $$C_x = \frac{1}{n}\sum_{i=1}^n x_i, \quad C_y = \frac{1}{n}\sum_{i=1}^n y_i$$
* **Rotación y Escala centradas:** Para no alterar la posición del polígono en la pantalla, las transformaciones se componen alrededor de su centroide:
  $$M = T(C) \cdot R(\theta) \cdot T(-C)$$
  $$M = T(C) \cdot S(s_x, s_y) \cdot T(-C)$$
* **Traslación:** Multiplicación directa por la matriz homogénea de traslación $T(\Delta x, \Delta y)$ aplicada a cada vértice mediante `transformarPunto`.

### 3. Relleno por Scan-Line con ET y EAT
* **Función:** `rellenarPoligono(Poligono &pol)`
* **Técnica:** Algoritmo clásico de escaneo horizontal por líneas:
  1. **Edge Table (ET):** Almacena las aristas no horizontales indexadas por su $y_{\min}$, guardando $y_{\max}$, la coordenada $x$ inicial y la pendiente inversa $\text{invM} = \frac{\Delta x}{\Delta y}$.
  2. **Active Edge Table (EAT):** Mantiene las aristas que intersectan con la línea de barrido actual $y$. En cada fila se retiran las aristas cuyo $y_{\max} < y$, se agregan las nuevas desde la ET, se ordenan de menor a mayor en $x$ y se colorean los píxeles entre pares consecutivos de intersecciones $[x_1, x_2]$ con `glVertex2i(x, y)`.
  3. **Actualización incremental:** Se suma $\text{invM}$ a cada arista activa para el siguiente renglón $y + 1$.

### 4. Detección de Punto en Polígono (Ray Casting / Teorema de la Curva de Jordan)
* **Función:** `puntoEnPoligono(float x, float y, const Poligono &pol)`
* **Objetivo:** Identificar si el clic del mouse cayó dentro de un polígono cerrado para seleccionarlo interactivamente.
* **Técnica:** Comparte el mismo principio de paridad del algoritmo Scan-Line. Lanza un rayo horizontal imaginario desde $(x, y)$ hacia el infinito en $+X$ y cuenta las intersecciones con las aristas del polígono: un número impar de cruces indica que el punto se encuentra en el interior, mientras que un número par indica que está en el exterior.

---

## 🖥️ Uso de OpenGL y GLUT

* **OpenGL:** Se utiliza como interfaz gráfica de bajo nivel exclusivamente para configurar el área de proyección ortográfica 2D bidimensional (`gluOrtho2D(0, 800, 0, 600)`) y como búfer de dibujo de píxeles individuales (`glBegin(GL_POINTS)`). No se emplean funciones de rasterizado nativo como `GL_LINES` ni `GL_POLYGON`.
* **GLUT (OpenGL Utility Toolkit):** Se encarga de la gestión de la ventana del sistema operativo, el doble búfer (`GLUT_DOUBLE | GLUT_RGB`) para evitar parpadeos y la arquitectura dirigida por eventos mediante funciones de callback (`glutDisplayFunc`, `glutMouseFunc`, `glutPassiveMotionFunc`, `glutKeyboardFunc` y `glutSpecialFunc`).
