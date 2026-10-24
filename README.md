# Computación Avanzada: efecto cartoon en C++

Este proyecto genera una imagen estilo cartoon a partir de una imagen RGB y permite comparar cuatro formas de ejecutar el mismo algoritmo:

- secuencial;
- MPI (memoria distribuida);
- OpenMP (memoria compartida);
- híbrida MPI + OpenMP.

El procesamiento conserva el algoritmo definido para el trabajo: primero reduce ruido con un filtro promedio, después detecta bordes con Sobel y un umbral, posteriza los colores y finalmente combina los bordes con la imagen posterizada.

## Estructura

```text
computacion-avanzada-cartoon/
├── src/
│   ├── secuencial.cpp
│   ├── mpi.cpp
│   ├── openmp.cpp
│   └── hibrido.cpp
├── imagenes/
│   └── imagen.png
├── resultados/
├── CMakeLists.txt
├── README.md
└── .gitignore
```

`imagenes/imagen.png` es una imagen RGB de prueba de 128 x 128 píxeles. Puede reemplazarse por la imagen que se quiera procesar, conservando ese nombre.

## Diferencias entre las cuatro versiones

- **Secuencial:** usa un único flujo de ejecución. Completa cada etapa antes de comenzar la siguiente.
- **MPI:** usa varios procesos con memoria separada. El proceso 0 carga y distribuye la imagen; cada proceso calcula un grupo de filas y al final el proceso 0 reúne los resultados.
- **OpenMP:** usa un proceso con varios threads que comparten memoria. Los `#pragma omp parallel for` reparten las filas entre threads.
- **MPI + OpenMP:** usa varios procesos y varios threads dentro de cada proceso. MPI reparte grupos de filas entre procesos y OpenMP reparte las filas de cada grupo entre threads.

Las versiones MPI e híbrida calculan el píxel filtrado cuando Sobel lo necesita, tal como estaba planteado en los códigos originales. Las versiones secuencial y OpenMP almacenan primero toda la imagen filtrada. No se cambió esta organización para mantener las implementaciones de referencia.

## Requisitos

- compilador de C++ con soporte para C++17;
- CMake 3.16 o superior;
- SFML 2.5 o 2.6, módulo Graphics;
- MPI con soporte para C++;
- OpenMP.

### Importante sobre SFML

Los cuatro códigos originales usan la API de SFML 2.x: `Image::create`, `getPixel(x, y)` y `setPixel(x, y, color)`. SFML 3 cambió esas firmas. Por eso el proyecto solicita explícitamente SFML 2.5 o superior dentro de la serie 2.x y no modifica el algoritmo para adaptarlo a SFML 3.

En macOS, la fórmula normal `sfml` instala SFML 3. Se debe instalar `sfml@2`.

### Instalación en macOS con Homebrew

```bash
brew install cmake sfml@2 open-mpi libomp
```

El `CMakeLists.txt` ya incluye las ubicaciones habituales de Homebrew en Apple Silicon (`/opt/homebrew`) e Intel (`/usr/local`) para encontrar `sfml@2` y `libomp`.

### Instalación en Ubuntu o Debian

```bash
sudo apt update
sudo apt install build-essential cmake libsfml-dev openmpi-bin libopenmpi-dev libomp-dev
```

Antes de compilar conviene confirmar que `libsfml-dev` pertenece a la serie 2.x:

```bash
pkg-config --modversion sfml-graphics
```

## Compilación

Desde la carpeta raíz del proyecto:

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j
```

Se generan cuatro ejecutables dentro de `build`:

```text
secuencial
mpi
openmp
hibrido
```

CMake enlaza únicamente las dependencias necesarias:

- `secuencial`: SFML;
- `mpi`: SFML y MPI;
- `openmp`: SFML y OpenMP;
- `hibrido`: SFML, MPI y OpenMP.

Si se cambia una variable del código, alcanza con repetir:

```bash
cmake --build . -j
```

## Ejecución

Los siguientes comandos se ejecutan desde la carpeta `build`. Esto es importante porque las rutas de entrada y salida son relativas a esa carpeta.

### Secuencial

```bash
./secuencial ../imagenes/imagen_800.png
```

Resultado: `resultados/cartoon_secuencial.png`.

### MPI

Ejemplo con cuatro procesos:

```bash
mpirun -np 4 ./mpi ../imagenes/imagen_800.png
```

Resultado: `resultados/cartoon_mpi.png`.

### OpenMP

```bash
./openmp ../imagenes/imagen_800.png
```

Resultado: `resultados/cartoon_openmp.png`.

### Híbrida MPI + OpenMP

Ejemplo con dos procesos MPI y cuatro threads dentro de cada proceso:

```bash
mpirun -np 2 ./hibrido ../imagenes/imagen_800.png
```

Resultado: `resultados/cartoon_hibrido.png`.

En algunos equipos Open MPI puede advertir que no hay suficientes *slots* para la cantidad de procesos solicitada. Para una prueba local se puede agregar `--oversubscribe`, aunque para las mediciones finales conviene usar una cantidad de procesos acorde al hardware:

```bash
mpirun --oversubscribe -np 4 ./mpi ../imagenes/imagen_800.png
```

## Imagen de entrada y resultados

La ruta de entrada se puede indicar como primer argumento. Si no se pasa
ningún argumento, las cuatro versiones usan por defecto:

```text
imagenes/imagen.png
```

Por ejemplo, para seleccionar otra imagen:

```bash
./secuencial ../imagenes/imagen_2000.png
```

Cada ejecución reemplaza solamente el archivo correspondiente dentro de `resultados`. Los programas comprueban tanto la lectura como el guardado y terminan con error si alguna operación falla.

Para las pruebas del informe están preparadas versiones de:

- 800 x 800;
- 2000 x 2000;
- 5000 x 5000.

Para comparar tiempos correctamente, todas las versiones de una misma prueba deben recibir exactamente la misma imagen y los mismos parámetros.

## Cómo cambiar los parámetros

Los parámetros se pasan desde la terminal, después de la ruta de la imagen.
Si falta alguno, se usa su valor predeterminado.

### Tamaño del filtro promedio

Es el segundo argumento. Solo se aceptan `3` para un filtro 3 x 3 y `5`
para un filtro 5 x 5. El valor predeterminado es `3`.

### Niveles de posterización

Es el tercer argumento. Solo se aceptan `3` o `9`. El valor predeterminado
es `3`.

### Umbral de bordes

```cpp
int umbral = 128;
```

Se debe mantener el mismo umbral en las cuatro versiones cuando se comparan resultados.

### Cantidad de threads OpenMP

Es el cuarto argumento de OpenMP y de la versión híbrida. Debe ser mayor
que cero y su valor predeterminado es `4`. Para las pruebas previstas se
usan `1`, `2`, `4` u `8`. En la versión híbrida es la cantidad de threads
**por cada proceso MPI**.

### Sintaxis completa

```bash
./secuencial RUTA_IMAGEN TAMANIO_FILTRO NIVELES_POSTERIZADO
mpirun -np CANTIDAD_PROCESOS ./mpi RUTA_IMAGEN TAMANIO_FILTRO NIVELES_POSTERIZADO
./openmp RUTA_IMAGEN TAMANIO_FILTRO NIVELES_POSTERIZADO CANTIDAD_THREADS
mpirun -np CANTIDAD_PROCESOS ./hibrido RUTA_IMAGEN TAMANIO_FILTRO NIVELES_POSTERIZADO CANTIDAD_THREADS
```

Por ejemplo:

```bash
./secuencial ../imagenes/imagen_800.png 3 3
mpirun -np 4 ./mpi ../imagenes/imagen_800.png 3 3
./openmp ../imagenes/imagen_800.png 3 3 4
mpirun -np 2 ./hibrido ../imagenes/imagen_800.png 3 3 4
```

Los valores predeterminados son imagen `../imagenes/imagen.png`, filtro `3`,
posterizado `3` y threads `4`.

### Cantidad de procesos MPI

No se cambia en el código. Se indica con `-np` al ejecutar:

```bash
mpirun -np 1 ./mpi
mpirun -np 2 ./mpi
mpirun -np 4 ./mpi
```

El mismo criterio se aplica a `hibrido`.

## Medición de tiempos

Las cuatro versiones imprimen el tiempo en segundos al finalizar:

- secuencial usa `std::chrono::steady_clock`;
- MPI usa `MPI_Wtime()`;
- OpenMP usa `omp_get_wtime()`;
- híbrida usa `MPI_Wtime()`.

La carga y el guardado del PNG quedan fuera de la medición porque dependen del disco. En MPI e híbrido también queda fuera la distribución inicial de la imagen, mientras que la reunión final de filas sí queda incluida. Las barreras MPI sincronizan los procesos antes de iniciar y antes de finalizar la medición.

Para una comparación seria:

1. usar la misma imagen y los mismos parámetros;
2. compilar una vez antes de medir;
3. ejecutar cada configuración varias veces;
4. registrar los tiempos reales impresos por los programas;
5. calcular el promedio o la mediana y explicar el criterio elegido.

Las fórmulas son:

```text
Speedup = TiempoSecuencial / TiempoParalelo
Eficiencia = Speedup / cantidadUnidadesEjecucion
```

Para MPI, las unidades de ejecución son los procesos. Para OpenMP, son los threads. Para la versión híbrida se puede usar `cantidadProcesos * cantidadThreads`, dejando este criterio aclarado en el informe.

El proyecto no incluye resultados inventados ni tablas precargadas: los valores deben salir de ejecuciones reales en el equipo utilizado para las pruebas.

## Cambios técnicos respecto de los códigos originales

No se cambió el algoritmo, no se acortaron los nombres descriptivos y no se agregaron optimizaciones difíciles de explicar. Solo se hicieron estos cambios:

1. la entrada se recibe como primer argumento y, si se omite, se lee desde `../imagenes/imagen.png` al ejecutar dentro de `build`;
2. cada versión guarda su salida dentro de `../resultados/` con el nombre pedido;
3. se comprueba si `saveToFile` pudo guardar la imagen;
4. la versión secuencial mide el procesamiento con `std::chrono::steady_clock`;
5. la versión MPI mide con `MPI_Wtime()` y usa barreras, siguiendo el criterio de la versión híbrida;
6. MPI e híbrido convierten explícitamente `resultadoLocal.size()` a `int`, porque `MPI_Gatherv` recibe un contador de tipo `int`;
7. se agregaron mensajes que informan procesos, threads y tiempos;
8. CMake exige SFML 2.x para conservar la API original.

## Observación sobre imágenes muy grandes

Cada píxel se almacena con cuatro componentes RGBA. Las versiones MPI e híbrida envían una copia completa de la imagen a cada proceso, tal como indican las implementaciones de referencia. Por eso, una imagen de 5000 x 5000 ocupa aproximadamente 100 MB por copia y el consumo total aumenta con la cantidad de procesos. Esto debe tenerse en cuenta al elegir cuántos procesos ejecutar, pero no se cambió el esquema porque forma parte del código definido para el informe.
