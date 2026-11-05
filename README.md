# Computación Avanzada: efecto cartoon en C++

Este proyecto genera una imagen estilo cartoon a partir de una imagen RGB y permite comparar cuatro formas de ejecutar el mismo algoritmo:

- secuencial;
- MPI (memoria distribuida);
- OpenMP (memoria compartida);
- híbrida MPI + OpenMP.

El procesamiento conserva el algoritmo definido para el trabajo: primero reduce ruido con un filtro promedio, después detecta bordes con Sobel y un umbral, posteriza los colores y finalmente combina los bordes con la imagen posterizada.

## Versión inicial y corrección de MPI

En la primera versión, MPI y el híbrido no guardaban la imagen filtrada: cada vez que Sobel necesitaba uno de sus 9 vecinos, `calcularBorde` llamaba a `pixelFiltrado` y volvía a calcular el promedio. El filtro se calculaba hasta **9 veces por píxel**, mientras que secuencial y OpenMP lo calculan una sola vez. Por eso MPI daba Speedup menor que 1.

En la versión actual, cada proceso:

1. calcula su rango de filas igual que antes (`filasBase`, `filasSobrantes`);
2. **filtra una sola vez** sus filas más una fila vecina arriba y otra abajo (las que Sobel necesita en los límites del bloque) y las guarda en `imagenFiltradaLocal`;
3. aplica Sobel leyendo esos valores guardados, posteriza y combina;
4. reúne el resultado con `MPI_Gatherv`, igual que antes.

En el híbrido, el paso 2 también se reparte entre threads con `#pragma omp parallel for schedule(static)`. El filtrado del bloque queda **dentro** del tiempo medido. Nada más cambia. Las líneas nuevas están marcadas con `// CORRECCIÓN:`.

La versión inicial se conserva sin cambios en `src/version_inicial/` (ejecutables `mpi_inicial` e `hibrido_inicial`) para poder comparar. La versión entregada originalmente también quedó guardada en la etiqueta `v1-entrega-original` del repositorio.

Las tres variantes paralelas (inicial, corregida y OpenMP) generan una imagen **idéntica** a la del secuencial.

### Resultados

Apple M5 (4 núcleos de rendimiento + 6 de eficiencia), filtro 3x3, 3 niveles, mediana de 3 ejecuciones. Salida completa en `resultados/medicion.txt` y `resultados/tiempos.csv`.

| Versión | Unidades | 800x800 | 2000x2000 | 5000x5000 | Tiempo 5000x5000 |
|---|---|---|---|---|---|
| Secuencial | 1 | 1,00 | 1,00 | 1,00 | 2,59 s |
| OpenMP | 4 threads | 2,69 (67 %) | 2,75 (69 %) | 2,70 (68 %) | 0,96 s |
| MPI inicial | 4 procesos | 0,72 (18 %) | 0,74 (18 %) | 0,63 (16 %) | 4,09 s |
| MPI corregido | 4 procesos | 2,25 (56 %) | 2,27 (57 %) | 2,03 (51 %) | 1,27 s |
| Híbrido inicial | 2 × 4 = 8 | 0,94 (12 %) | 1,00 (12 %) | 0,93 (12 %) | 2,77 s |
| Híbrido corregido | 2 × 4 = 8 | 2,67 (33 %) | 2,84 (35 %) | 2,65 (33 %) | 0,98 s |

Speedup (eficiencia). La corrección hace que MPI y el híbrido pasen de no mejorar al secuencial a ser entre 2 y 2,8 veces más rápidos. OpenMP sigue siendo la opción más eficiente. El híbrido logra un tiempo similar al de OpenMP pero con el doble de unidades de ejecución; en este equipo, más allá de 4 unidades el trabajo también cae en los núcleos de eficiencia, que son más lentos.

## Estructura

```text
computacion-avanzada-cartoon/
├── src/
│   ├── secuencial.cpp
│   ├── mpi.cpp              (versión corregida)
│   ├── openmp.cpp
│   ├── hibrido.cpp          (versión corregida)
│   └── version_inicial/
│       ├── mpi.cpp          (versión inicial, sin cambios)
│       └── hibrido.cpp      (versión inicial, sin cambios)
├── imagenes/
│   └── imagen.png
├── resultados/
├── CMakeLists.txt
├── medir.sh
├── README.md
└── .gitignore
```

`imagenes/imagen.png` es una imagen RGB de prueba de 128 x 128 píxeles. Puede reemplazarse por la imagen que se quiera procesar, conservando ese nombre.

## Diferencias entre las cuatro versiones

- **Secuencial:** usa un único flujo de ejecución. Completa cada etapa antes de comenzar la siguiente.
- **MPI:** usa varios procesos con memoria separada. El proceso 0 carga y distribuye la imagen; cada proceso calcula un grupo de filas y al final el proceso 0 reúne los resultados.
- **OpenMP:** usa un proceso con varios threads que comparten memoria. Los `#pragma omp parallel for` reparten las filas entre threads.
- **MPI + OpenMP:** usa varios procesos y varios threads dentro de cada proceso. MPI reparte grupos de filas entre procesos y OpenMP reparte las filas de cada grupo entre threads.

Las versiones secuencial y OpenMP almacenan primero toda la imagen filtrada. Las versiones MPI e híbrida hacen todo en una sola pasada por sus filas, pero primero filtran y guardan su bloque (ver "Versión inicial y corrección de MPI").

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

Se generan seis ejecutables dentro de `build`:

```text
secuencial
mpi
openmp
hibrido
mpi_inicial
hibrido_inicial
```

`mpi_inicial` e `hibrido_inicial` son la versión anterior a la corrección y se usan solo para comparar. Se ejecutan con la misma sintaxis que `mpi` e `hibrido`.

CMake enlaza únicamente las dependencias necesarias:

- `secuencial`: SFML;
- `mpi`: SFML y MPI;
- `openmp`: SFML y OpenMP;
- `hibrido` e `hibrido_inicial`: SFML, MPI y OpenMP;
- `mpi_inicial`: SFML y MPI.

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

Las tablas de este README salen de ejecuciones reales en el equipo utilizado para las pruebas.

### Medir todo de una vez

Desde `build`, después de compilar:

```bash
../medir.sh 3 3 3
```

Los argumentos son filtro, niveles y cantidad de repeticiones. El script ejecuta cada versión (secuencial, OpenMP con 4 threads, MPI con 4 procesos e híbrido con 2 procesos x 4 threads, en sus variantes inicial y corregida) para 800, 2000 y 5000, toma la mediana y calcula Speedup y Eficiencia. Guarda los resultados en `resultados/tiempos.csv`.

## Cambios técnicos respecto de los códigos originales

No se cambió el algoritmo, no se acortaron los nombres descriptivos y no se agregaron optimizaciones difíciles de explicar. Solo se hicieron estos cambios:

1. la entrada se recibe como primer argumento y, si se omite, se lee desde `../imagenes/imagen.png` al ejecutar dentro de `build`;
2. cada versión guarda su salida dentro de `../resultados/` con el nombre pedido;
3. se comprueba si `saveToFile` pudo guardar la imagen;
4. la versión secuencial mide el procesamiento con `std::chrono::steady_clock`;
5. la versión MPI mide con `MPI_Wtime()` y usa barreras, siguiendo el criterio de la versión híbrida;
6. MPI e híbrido convierten explícitamente `resultadoLocal.size()` a `int`, porque `MPI_Gatherv` recibe un contador de tipo `int`;
7. se agregaron mensajes que informan procesos, threads y tiempos;
8. CMake exige SFML 2.x para conservar la API original;
9. corrección posterior: MPI e híbrido filtran y guardan su bloque una sola vez antes de aplicar Sobel, en lugar de recalcular el filtro para cada vecino. La versión anterior quedó en `src/version_inicial/`.

## Observación sobre imágenes muy grandes

Cada píxel se almacena con cuatro componentes RGBA. Las versiones MPI e híbrida envían una copia completa de la imagen a cada proceso, tal como indican las implementaciones de referencia. Por eso, una imagen de 5000 x 5000 ocupa aproximadamente 100 MB por copia y el consumo total aumenta con la cantidad de procesos. Esto debe tenerse en cuenta al elegir cuántos procesos ejecutar, pero no se cambió el esquema porque forma parte del código definido para el informe.
