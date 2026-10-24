#include <SFML/Graphics.hpp>
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

// 1) FILTRADO: promedio de los píxeles vecinos
void borrarRuido(
    const vector<unsigned char>& imagenOriginal,
    vector<unsigned char>& imagenFiltrada,
    int anchoImagen,
    int altoImagen,
    int tamanioFiltro
) {
    int radioFiltro = tamanioFiltro / 2;

    #pragma omp parallel for schedule(static)
    for (int fila = 0; fila < altoImagen; fila++) {

        for (int columna = 0;
             columna < anchoImagen;
             columna++) {

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

            int posicionActual =
                posicionPixel(
                    columna,
                    fila,
                    anchoImagen
                );

            imagenFiltrada[posicionActual] =
                sumaRojo / cantidadPixeles;

            imagenFiltrada[posicionActual + 1] =
                sumaVerde / cantidadPixeles;

            imagenFiltrada[posicionActual + 2] =
                sumaAzul / cantidadPixeles;

            imagenFiltrada[posicionActual + 3] = 255;
        }
    }
}

// Convierte un píxel RGB a gris
int convertirAGris(
    const vector<unsigned char>& imagen,
    int columna,
    int fila,
    int anchoImagen
) {
    int posicion =
        posicionPixel(
            columna,
            fila,
            anchoImagen
        );

    int rojo = imagen[posicion];
    int verde = imagen[posicion + 1];
    int azul = imagen[posicion + 2];

    return (int)(
        0.30 * rojo +
        0.59 * verde +
        0.11 * azul
    );
}

// 2 y 3) RESALTADO + UMBRAL usando Sobel
void detectarBordes(
    const vector<unsigned char>& imagenFiltrada,
    vector<unsigned char>& imagenBordes,
    int anchoImagen,
    int altoImagen,
    int umbral
) {
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

    #pragma omp parallel for schedule(static)
    for (int fila = 1;
         fila < altoImagen - 1;
         fila++) {

        for (int columna = 1;
             columna < anchoImagen - 1;
             columna++) {

            int gradienteX = 0;
            int gradienteY = 0;

            for (int desplazamientoFila = -1;
                 desplazamientoFila <= 1;
                 desplazamientoFila++) {

                for (int desplazamientoColumna = -1;
                     desplazamientoColumna <= 1;
                     desplazamientoColumna++) {

                    int valorGris =
                        convertirAGris(
                            imagenFiltrada,
                            columna + desplazamientoColumna,
                            fila + desplazamientoFila,
                            anchoImagen
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

            int posicionActual =
                posicionPixel(
                    columna,
                    fila,
                    anchoImagen
                );

            int valorBorde =
                magnitudBorde > umbral
                ? 255
                : 0;

            imagenBordes[posicionActual] = valorBorde;
            imagenBordes[posicionActual + 1] = valorBorde;
            imagenBordes[posicionActual + 2] = valorBorde;
            imagenBordes[posicionActual + 3] = 255;
        }
    }
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

// POSTERIZADO
void posterizar(
    const vector<unsigned char>& imagenOriginal,
    vector<unsigned char>& imagenPosterizada,
    int anchoImagen,
    int altoImagen,
    int cantidadNivelesPosterizado
) {
    #pragma omp parallel for schedule(static)
    for (int fila = 0; fila < altoImagen; fila++) {

        for (int columna = 0;
             columna < anchoImagen;
             columna++) {

            int posicionActual =
                posicionPixel(
                    columna,
                    fila,
                    anchoImagen
                );

            imagenPosterizada[posicionActual] =
                posterizarValor(
                    imagenOriginal[posicionActual],
                    cantidadNivelesPosterizado
                );

            imagenPosterizada[posicionActual + 1] =
                posterizarValor(
                    imagenOriginal[posicionActual + 1],
                    cantidadNivelesPosterizado
                );

            imagenPosterizada[posicionActual + 2] =
                posterizarValor(
                    imagenOriginal[posicionActual + 2],
                    cantidadNivelesPosterizado
                );

            imagenPosterizada[posicionActual + 3] = 255;
        }
    }
}

// COMBINACIÓN FINAL
void combinar(
    const vector<unsigned char>& imagenPosterizada,
    const vector<unsigned char>& imagenBordes,
    vector<unsigned char>& imagenCartoon,
    int anchoImagen,
    int altoImagen
) {
    #pragma omp parallel for schedule(static)
    for (int fila = 0; fila < altoImagen; fila++) {

        for (int columna = 0;
             columna < anchoImagen;
             columna++) {

            int posicionActual =
                posicionPixel(
                    columna,
                    fila,
                    anchoImagen
                );

            int valorBorde =
                imagenBordes[posicionActual];

            imagenCartoon[posicionActual] =
                limitarColor(
                    imagenPosterizada[posicionActual] +
                    valorBorde
                );

            imagenCartoon[posicionActual + 1] =
                limitarColor(
                    imagenPosterizada[posicionActual + 1] +
                    valorBorde
                );

            imagenCartoon[posicionActual + 2] =
                limitarColor(
                    imagenPosterizada[posicionActual + 2] +
                    valorBorde
                );

            imagenCartoon[posicionActual + 3] = 255;
        }
    }
}

int main(int argc, char** argv) {

    sf::Image imagen;

    const char* rutaImagenEntrada =
        argc > 1
        ? argv[1]
        : "../imagenes/imagen.png";

    if (!imagen.loadFromFile(rutaImagenEntrada)) {
        cout << "No se pudo abrir " << rutaImagenEntrada << endl;
        return 1;
    }

    int anchoImagen = imagen.getSize().x;
    int altoImagen = imagen.getSize().y;

    const unsigned char* pixeles =
        imagen.getPixelsPtr();

    vector<unsigned char> imagenOriginal(
        pixeles,
        pixeles + anchoImagen * altoImagen * 4
    );

    vector<unsigned char> imagenFiltrada(
        anchoImagen * altoImagen * 4,
        0
    );

    vector<unsigned char> imagenBordes(
        anchoImagen * altoImagen * 4,
        0
    );

    vector<unsigned char> imagenPosterizada(
        anchoImagen * altoImagen * 4,
        0
    );

    vector<unsigned char> imagenCartoon(
        anchoImagen * altoImagen * 4,
        0
    );

    int tamanioFiltro = 3;
    int cantidadNivelesPosterizado = 3;
    int umbral = 128;

    int cantidadThreads = 4;

    if (argc > 2 &&
        (!convertirArgumentoAEntero(argv[2], tamanioFiltro) ||
         (tamanioFiltro != 3 && tamanioFiltro != 5))) {
        cout << "El tamaño de filtro debe ser 3 o 5" << endl;
        return 1;
    }

    if (argc > 3 &&
        (!convertirArgumentoAEntero(
            argv[3],
            cantidadNivelesPosterizado
        ) ||
         (cantidadNivelesPosterizado != 3 &&
          cantidadNivelesPosterizado != 9))) {
        cout << "Los niveles de posterizado deben ser 3 o 9" << endl;
        return 1;
    }

    if (argc > 4 &&
        (!convertirArgumentoAEntero(argv[4], cantidadThreads) ||
         cantidadThreads <= 0)) {
        cout << "La cantidad de threads debe ser mayor que 0" << endl;
        return 1;
    }

    omp_set_num_threads(cantidadThreads);

    double tiempoInicio =
        omp_get_wtime();

    borrarRuido(
        imagenOriginal,
        imagenFiltrada,
        anchoImagen,
        altoImagen,
        tamanioFiltro
    );

    detectarBordes(
        imagenFiltrada,
        imagenBordes,
        anchoImagen,
        altoImagen,
        umbral
    );

    posterizar(
        imagenOriginal,
        imagenPosterizada,
        anchoImagen,
        altoImagen,
        cantidadNivelesPosterizado
    );

    combinar(
        imagenPosterizada,
        imagenBordes,
        imagenCartoon,
        anchoImagen,
        altoImagen
    );

    double tiempoFin =
        omp_get_wtime();

    sf::Image resultado;

    resultado.create(
        anchoImagen,
        altoImagen,
        imagenCartoon.data()
    );

    if (!resultado.saveToFile(
            "../resultados/cartoon_openmp.png"
        )) {
        cout << "No se pudo guardar la imagen" << endl;
        return 1;
    }

    cout
        << "Imagen OpenMP generada correctamente"
        << endl;

    cout
        << "Threads utilizados: "
        << cantidadThreads
        << endl;

    cout
        << "Tiempo: "
        << tiempoFin - tiempoInicio
        << " segundos"
        << endl;

    return 0;
}
