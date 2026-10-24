#include <SFML/Graphics.hpp>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>

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

// 1) FILTRADO: promedio de los píxeles vecinos
sf::Image borrarRuido(const sf::Image& imagen, int tamanioFiltro) {
    sf::Vector2u tamanioImagen = imagen.getSize();

    sf::Image resultado;
    resultado.create(tamanioImagen.x, tamanioImagen.y);

    int radioFiltro = tamanioFiltro / 2;

    for (int fila = 0; fila < (int)tamanioImagen.y; fila++) {
        for (int columna = 0; columna < (int)tamanioImagen.x; columna++) {

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

                    int filaVecina = fila + desplazamientoFila;
                    int columnaVecina = columna + desplazamientoColumna;

                    if (filaVecina >= 0 &&
                        filaVecina < (int)tamanioImagen.y &&
                        columnaVecina >= 0 &&
                        columnaVecina < (int)tamanioImagen.x) {

                        sf::Color pixel =
                            imagen.getPixel(columnaVecina, filaVecina);

                        sumaRojo += pixel.r;
                        sumaVerde += pixel.g;
                        sumaAzul += pixel.b;
                        cantidadPixeles++;
                    }
                }
            }

            resultado.setPixel(
                columna,
                fila,
                sf::Color(
                    sumaRojo / cantidadPixeles,
                    sumaVerde / cantidadPixeles,
                    sumaAzul / cantidadPixeles
                )
            );
        }
    }

    return resultado;
}

// Convierte un píxel RGB a un único valor gris
int convertirAGris(sf::Color pixel) {
    return (int)(
        0.30 * pixel.r +
        0.59 * pixel.g +
        0.11 * pixel.b
    );
}

// 2 y 3) RESALTADO + UMBRAL usando Sobel
sf::Image detectarBordes(const sf::Image& imagen, int umbral) {
    sf::Vector2u tamanioImagen = imagen.getSize();

    sf::Image bordes;
    bordes.create(
        tamanioImagen.x,
        tamanioImagen.y,
        sf::Color::Black
    );

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

    for (int fila = 1; fila < (int)tamanioImagen.y - 1; fila++) {
        for (int columna = 1;
             columna < (int)tamanioImagen.x - 1;
             columna++) {

            int gradienteX = 0;
            int gradienteY = 0;

            for (int desplazamientoFila = -1;
                 desplazamientoFila <= 1;
                 desplazamientoFila++) {

                for (int desplazamientoColumna = -1;
                     desplazamientoColumna <= 1;
                     desplazamientoColumna++) {

                    int valorGris = convertirAGris(
                        imagen.getPixel(
                            columna + desplazamientoColumna,
                            fila + desplazamientoFila
                        )
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

            if (magnitudBorde > umbral) {
                bordes.setPixel(
                    columna,
                    fila,
                    sf::Color::White
                );
            }
        }
    }

    return bordes;
}

// POSTERIZADO: reduce los valores posibles de R, G y B
int posterizarValor(int valorColor, int cantidadNiveles) {
    double pasoEntreNiveles =
        255.0 / (cantidadNiveles - 1);

    return limitarColor(
        (int)round(
            round(valorColor / pasoEntreNiveles) *
            pasoEntreNiveles
        )
    );
}

sf::Image posterizar(
    const sf::Image& imagen,
    int cantidadNiveles
) {
    sf::Vector2u tamanioImagen = imagen.getSize();

    sf::Image resultado;
    resultado.create(tamanioImagen.x, tamanioImagen.y);

    for (int fila = 0; fila < (int)tamanioImagen.y; fila++) {
        for (int columna = 0;
             columna < (int)tamanioImagen.x;
             columna++) {

            sf::Color pixel =
                imagen.getPixel(columna, fila);

            resultado.setPixel(
                columna,
                fila,
                sf::Color(
                    posterizarValor(
                        pixel.r,
                        cantidadNiveles
                    ),
                    posterizarValor(
                        pixel.g,
                        cantidadNiveles
                    ),
                    posterizarValor(
                        pixel.b,
                        cantidadNiveles
                    )
                )
            );
        }
    }

    return resultado;
}

// COMBINACIÓN FINAL
sf::Image combinar(
    const sf::Image& imagenPosterizada,
    const sf::Image& imagenBordes
) {
    sf::Vector2u tamanioImagen =
        imagenPosterizada.getSize();

    sf::Image imagenCartoon;
    imagenCartoon.create(
        tamanioImagen.x,
        tamanioImagen.y
    );

    for (int fila = 0; fila < (int)tamanioImagen.y; fila++) {
        for (int columna = 0;
             columna < (int)tamanioImagen.x;
             columna++) {

            sf::Color pixelPosterizado =
                imagenPosterizada.getPixel(columna, fila);

            int valorBorde =
                imagenBordes.getPixel(columna, fila).r;

            imagenCartoon.setPixel(
                columna,
                fila,
                sf::Color(
                    limitarColor(
                        pixelPosterizado.r + valorBorde
                    ),
                    limitarColor(
                        pixelPosterizado.g + valorBorde
                    ),
                    limitarColor(
                        pixelPosterizado.b + valorBorde
                    )
                )
            );
        }
    }

    return imagenCartoon;
}

int main(int argc, char** argv) {

    sf::Image imagenOriginal;

    const char* rutaImagenEntrada =
        argc > 1
        ? argv[1]
        : "../imagenes/imagen.png";

    if (!imagenOriginal.loadFromFile(rutaImagenEntrada)) {
        cout << "No se pudo abrir " << rutaImagenEntrada << endl;
        return 1;
    }

    int tamanioFiltro = 3;
    int cantidadNivelesPosterizado = 3;
    int umbral = 128;

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

    chrono::steady_clock::time_point tiempoInicio =
        chrono::steady_clock::now();

    sf::Image imagenFiltrada =
        borrarRuido(imagenOriginal, tamanioFiltro);

    sf::Image imagenBordes =
        detectarBordes(imagenFiltrada, umbral);

    sf::Image imagenPosterizada =
        posterizar(
            imagenOriginal,
            cantidadNivelesPosterizado
        );

    sf::Image imagenCartoon =
        combinar(imagenPosterizada, imagenBordes);

    chrono::steady_clock::time_point tiempoFin =
        chrono::steady_clock::now();

    if (!imagenCartoon.saveToFile("../resultados/cartoon_secuencial.png")) {
        cout << "No se pudo guardar la imagen" << endl;
        return 1;
    }

    chrono::duration<double> tiempoTranscurrido =
        tiempoFin - tiempoInicio;

    cout << "Imagen secuencial generada correctamente" << endl;
    cout << "Tiempo: " << tiempoTranscurrido.count()
         << " segundos" << endl;

    return 0;
}
