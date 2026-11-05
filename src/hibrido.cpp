#include <SFML/Graphics.hpp>
#include <mpi.h>
#include <omp.h>
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

// Convierte un argumento de terminal a entero y rechaza texto no numérico
bool convertirArgumentoAEntero(
    const char* argumento,
    int& valorConvertido
) {
    istringstream flujoArgumento(argumento);
    char caracterExtra;

    if (!(flujoArgumento >> valorConvertido))
        return false;

    if (flujoArgumento >> caracterExtra)
        return false;

    return true;
}

// Evita que un color salga del rango válido 0..255
int limitarColor(int valorColor) {
    if (valorColor < 0) return 0;
    if (valorColor > 255) return 255;
    return valorColor;
}

// Devuelve la posición de un píxel dentro del vector
int posicionPixel(
    int columna,
    int fila,
    int anchoImagen
) {
    return (fila * anchoImagen + columna) * 4;
}

// Calcula el promedio de los vecinos de un píxel
sf::Color pixelFiltrado(
    const vector<unsigned char>& imagenOriginal,
    int columna,
    int fila,
    int anchoImagen,
    int altoImagen,
    int tamanioFiltro
) {
    int radioFiltro = tamanioFiltro / 2;

    int sumaRojo = 0;
    int sumaVerde = 0;
    int sumaAzul = 0;
    int cantidadPixeles = 0;

    for (int desplazamientoFila = -radioFiltro;
         desplazamientoFila <= radioFiltro;
         desplazamientoFila++) {

        for (int desplazamientoColumna = -radioFiltro;
             desplazamientoColumna <= radioFiltro;
             desplazamientoColumna++) {

            int filaVecina =
                fila + desplazamientoFila;

            int columnaVecina =
                columna + desplazamientoColumna;

            if (filaVecina >= 0 &&
                filaVecina < altoImagen &&
                columnaVecina >= 0 &&
                columnaVecina < anchoImagen) {

                int posicionVecino =
                    posicionPixel(
                        columnaVecina,
                        filaVecina,
                        anchoImagen
                    );

                sumaRojo +=
                    imagenOriginal[posicionVecino];

                sumaVerde +=
                    imagenOriginal[posicionVecino + 1];

                sumaAzul +=
                    imagenOriginal[posicionVecino + 2];

                cantidadPixeles++;
            }
        }
    }

    return sf::Color(
        sumaRojo / cantidadPixeles,
        sumaVerde / cantidadPixeles,
        sumaAzul / cantidadPixeles
    );
}

// Calcula si un píxel pertenece a un borde.
// CORRECCIÓN: ya no recalcula el filtro para cada vecino.
// Lee los valores filtrados que el proceso calculó y guardó una sola vez
// en imagenFiltradaLocal (sus filas más una fila vecina arriba y abajo).
int calcularBorde(
    const vector<unsigned char>& imagenFiltradaLocal,
    int filaInicialFiltrado,
    int columna,
    int fila,
    int anchoImagen,
    int altoImagen,
    int umbral
) {
    if (fila == 0 ||
        fila == altoImagen - 1 ||
        columna == 0 ||
        columna == anchoImagen - 1) {
        return 0;
    }

    int mascaraSobelX[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };

    int mascaraSobelY[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    int gradienteX = 0;
    int gradienteY = 0;

    for (int desplazamientoFila = -1;
         desplazamientoFila <= 1;
         desplazamientoFila++) {

        for (int desplazamientoColumna = -1;
             desplazamientoColumna <= 1;
             desplazamientoColumna++) {

            int columnaVecina =
                columna + desplazamientoColumna;

            int filaVecina =
                fila + desplazamientoFila;

            // La fila vecina se ubica dentro del bloque filtrado local
            int posicionFiltrada =
                posicionPixel(
                    columnaVecina,
                    filaVecina - filaInicialFiltrado,
                    anchoImagen
                );

            int valorGris = (int)(
                0.30 * imagenFiltradaLocal[posicionFiltrada] +
                0.59 * imagenFiltradaLocal[posicionFiltrada + 1] +
                0.11 * imagenFiltradaLocal[posicionFiltrada + 2]
            );

            gradienteX +=
                valorGris *
                mascaraSobelX
                [desplazamientoFila + 1]
                [desplazamientoColumna + 1];

            gradienteY +=
                valorGris *
                mascaraSobelY
                [desplazamientoFila + 1]
                [desplazamientoColumna + 1];
        }
    }

    int magnitudBorde = (int)sqrt(
        gradienteX * gradienteX +
        gradienteY * gradienteY
    );

    if (magnitudBorde > umbral)
        return 255;

    return 0;
}

// Posteriza un único valor de color
int posterizarValor(
    int valorColor,
    int cantidadNiveles
) {
    double pasoEntreNiveles =
        255.0 / (cantidadNiveles - 1);

    return limitarColor(
        (int)round(
            round(valorColor / pasoEntreNiveles) *
            pasoEntreNiveles
        )
    );
}

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);

    int numeroProceso;
    int cantidadProcesos;

    MPI_Comm_rank(
        MPI_COMM_WORLD,
        &numeroProceso
    );

    MPI_Comm_size(
        MPI_COMM_WORLD,
        &cantidadProcesos
    );

    int anchoImagen = 0;
    int altoImagen = 0;

    vector<unsigned char> imagenOriginal;

    // Solo el proceso 0 carga la imagen
    if (numeroProceso == 0) {

        sf::Image imagen;

        const char* rutaImagenEntrada =
            argc > 1
            ? argv[1]
            : "../imagenes/imagen.png";

        if (!imagen.loadFromFile(rutaImagenEntrada)) {
            cout << "No se pudo abrir " << rutaImagenEntrada << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        anchoImagen = imagen.getSize().x;
        altoImagen = imagen.getSize().y;

        const unsigned char* pixeles =
            imagen.getPixelsPtr();

        imagenOriginal.assign(
            pixeles,
            pixeles +
            anchoImagen * altoImagen * 4
        );
    }

    // MPI envía el tamaño de la imagen a todos
    MPI_Bcast(
        &anchoImagen,
        1,
        MPI_INT,
        0,
        MPI_COMM_WORLD
    );

    MPI_Bcast(
        &altoImagen,
        1,
        MPI_INT,
        0,
        MPI_COMM_WORLD
    );

    if (numeroProceso != 0) {
        imagenOriginal.resize(
            anchoImagen * altoImagen * 4
        );
    }

    // MPI envía la imagen completa a todos los procesos
    MPI_Bcast(
        imagenOriginal.data(),
        anchoImagen * altoImagen * 4,
        MPI_UNSIGNED_CHAR,
        0,
        MPI_COMM_WORLD
    );

    int tamanioFiltro = 3;
    int cantidadNivelesPosterizado = 3;
    int umbral = 128;

    int cantidadThreads = 4;

    if (argc > 2 &&
        (!convertirArgumentoAEntero(argv[2], tamanioFiltro) ||
         (tamanioFiltro != 3 && tamanioFiltro != 5))) {
        if (numeroProceso == 0) {
            cout << "El tamaño de filtro debe ser 3 o 5" << endl;
        }
        MPI_Finalize();
        return 1;
    }

    if (argc > 3 &&
        (!convertirArgumentoAEntero(
            argv[3],
            cantidadNivelesPosterizado
        ) ||
         (cantidadNivelesPosterizado != 3 &&
          cantidadNivelesPosterizado != 9))) {
        if (numeroProceso == 0) {
            cout << "Los niveles de posterizado deben ser 3 o 9" << endl;
        }
        MPI_Finalize();
        return 1;
    }

    if (argc > 4 &&
        (!convertirArgumentoAEntero(argv[4], cantidadThreads) ||
         cantidadThreads <= 0)) {
        if (numeroProceso == 0) {
            cout << "La cantidad de threads debe ser mayor que 0" << endl;
        }
        MPI_Finalize();
        return 1;
    }

    omp_set_num_threads(cantidadThreads);

    // MPI reparte las filas entre procesos
    int filasBase =
        altoImagen / cantidadProcesos;

    int filasSobrantes =
        altoImagen % cantidadProcesos;

    int cantidadFilasProceso =
        filasBase +
        (numeroProceso < filasSobrantes ? 1 : 0);

    int filaInicial =
        numeroProceso * filasBase +
        (numeroProceso < filasSobrantes
            ? numeroProceso
            : filasSobrantes);

    int filaFinal =
        filaInicial + cantidadFilasProceso;

    vector<unsigned char> resultadoLocal(
        cantidadFilasProceso *
        anchoImagen *
        4
    );

    // CORRECCIÓN: bloque filtrado local.
    // Sobel necesita una fila vecina arriba y otra abajo, así que el
    // proceso filtra sus filas más esas filas de borde (si existen).
    int filaInicialFiltrado =
        filaInicial > 0
            ? filaInicial - 1
            : 0;

    int filaFinalFiltrado =
        filaFinal < altoImagen
            ? filaFinal + 1
            : altoImagen;

    int cantidadFilasFiltrado =
        filaFinalFiltrado - filaInicialFiltrado;

    vector<unsigned char> imagenFiltradaLocal(
        cantidadFilasFiltrado *
        anchoImagen *
        4
    );

    MPI_Barrier(MPI_COMM_WORLD);

    double tiempoInicio =
        MPI_Wtime();

    // CORRECCIÓN: 1) filtrar una sola vez el bloque y guardarlo
    #pragma omp parallel for schedule(static)
    for (int fila = filaInicialFiltrado;
         fila < filaFinalFiltrado;
         fila++) {

        for (int columna = 0;
             columna < anchoImagen;
             columna++) {

            sf::Color filtrado =
                pixelFiltrado(
                    imagenOriginal,
                    columna,
                    fila,
                    anchoImagen,
                    altoImagen,
                    tamanioFiltro
                );

            int posicionFiltrada =
                posicionPixel(
                    columna,
                    fila - filaInicialFiltrado,
                    anchoImagen
                );

            imagenFiltradaLocal[posicionFiltrada] = filtrado.r;
            imagenFiltradaLocal[posicionFiltrada + 1] = filtrado.g;
            imagenFiltradaLocal[posicionFiltrada + 2] = filtrado.b;
            imagenFiltradaLocal[posicionFiltrada + 3] = 255;
        }
    }

    // 2) Sobel, posterizado y combinación sobre las filas propias
    // OpenMP reparte entre threads las filas
    // que MPI asignó a este proceso
    #pragma omp parallel for schedule(static)
    for (int fila = filaInicial;
         fila < filaFinal;
         fila++) {

        for (int columna = 0;
             columna < anchoImagen;
             columna++) {

            int posicionOriginal =
                posicionPixel(
                    columna,
                    fila,
                    anchoImagen
                );

            int rojoPosterizado =
                posterizarValor(
                    imagenOriginal[posicionOriginal],
                    cantidadNivelesPosterizado
                );

            int verdePosterizado =
                posterizarValor(
                    imagenOriginal[posicionOriginal + 1],
                    cantidadNivelesPosterizado
                );

            int azulPosterizado =
                posterizarValor(
                    imagenOriginal[posicionOriginal + 2],
                    cantidadNivelesPosterizado
                );

            int valorBorde =
                calcularBorde(
                    imagenFiltradaLocal,
                    filaInicialFiltrado,
                    columna,
                    fila,
                    anchoImagen,
                    altoImagen,
                    umbral
                );

            int filaLocal =
                fila - filaInicial;

            int posicionLocal =
                (filaLocal * anchoImagen + columna) * 4;

            resultadoLocal[posicionLocal] =
                limitarColor(
                    rojoPosterizado + valorBorde
                );

            resultadoLocal[posicionLocal + 1] =
                limitarColor(
                    verdePosterizado + valorBorde
                );

            resultadoLocal[posicionLocal + 2] =
                limitarColor(
                    azulPosterizado + valorBorde
                );

            resultadoLocal[posicionLocal + 3] = 255;
        }
    }

    vector<int> cantidades;
    vector<int> desplazamientos;
    vector<unsigned char> imagenCartoon;

    if (numeroProceso == 0) {

        cantidades.resize(cantidadProcesos);
        desplazamientos.resize(cantidadProcesos);

        int desplazamientoActual = 0;

        for (int proceso = 0;
             proceso < cantidadProcesos;
             proceso++) {

            int filasProceso =
                filasBase +
                (proceso < filasSobrantes ? 1 : 0);

            cantidades[proceso] =
                filasProceso *
                anchoImagen *
                4;

            desplazamientos[proceso] =
                desplazamientoActual;

            desplazamientoActual +=
                cantidades[proceso];
        }

        imagenCartoon.resize(
            anchoImagen * altoImagen * 4
        );
    }

    // MPI reúne los resultados calculados
    // por todos los procesos
    MPI_Gatherv(
        resultadoLocal.data(),
        (int)resultadoLocal.size(),
        MPI_UNSIGNED_CHAR,

        numeroProceso == 0
            ? imagenCartoon.data()
            : nullptr,

        numeroProceso == 0
            ? cantidades.data()
            : nullptr,

        numeroProceso == 0
            ? desplazamientos.data()
            : nullptr,

        MPI_UNSIGNED_CHAR,
        0,
        MPI_COMM_WORLD
    );

    MPI_Barrier(MPI_COMM_WORLD);

    double tiempoFin =
        MPI_Wtime();

    // Solo el proceso 0 guarda el resultado
    if (numeroProceso == 0) {

        sf::Image resultado;

        resultado.create(
            anchoImagen,
            altoImagen,
            imagenCartoon.data()
        );

        if (!resultado.saveToFile(
                "../resultados/cartoon_hibrido.png"
            )) {
            cout << "No se pudo guardar la imagen" << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        cout
            << "Imagen híbrida generada correctamente"
            << endl;

        cout
            << "Procesos MPI: "
            << cantidadProcesos
            << endl;

        cout
            << "Threads por proceso: "
            << cantidadThreads
            << endl;

        cout
            << "Tiempo: "
            << tiempoFin - tiempoInicio
            << " segundos"
            << endl;
    }

    MPI_Finalize();

    return 0;
}
