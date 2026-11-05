#!/bin/bash
# Mide las cuatro versiones (y la versión inicial de MPI e híbrido, antes de la
# corrección del filtrado) y calcula Speedup y Eficiencia contra el secuencial.
#
# Uso, desde la carpeta build:
#   ../medir.sh [filtro] [niveles] [repeticiones]
# Ejemplo:
#   ../medir.sh 3 3 3
#
# Cada configuración se ejecuta varias veces y se usa la mediana.
# Los resultados también se guardan en ../resultados/tiempos.csv

FILTRO=${1:-3}
NIVELES=${2:-3}
REPETICIONES=${3:-3}

PROCESOS_MPI=4
PROCESOS_HIBRIDO=2
THREADS=4

INICIAL=.
CSV=../resultados/tiempos.csv

# Ejecuta un programa y se queda con el número que imprime después de "Tiempo:"
tiempo() {
    "$@" 2>/dev/null | awk '/Tiempo:/ {print $2}'
}

# Mediana de una lista de números (uno por línea)
mediana() {
    sort -n | awk '{v[NR]=$1} END {
        if (NR == 0) { print ""; exit }
        if (NR % 2) print v[(NR + 1) / 2];
        else print (v[NR / 2] + v[NR / 2 + 1]) / 2
    }'
}

medir() {
    for i in $(seq "$REPETICIONES"); do
        tiempo "$@"
    done | mediana
}

# Imprime una fila de la tabla y la agrega al CSV
fila() {
    local nombre="$1" unidades="$2" tiempoParalelo="$3"
    if [ -z "$tiempoParalelo" ]; then
        printf "  %-26s %8s   no se pudo medir\n" "$nombre" "$unidades"
        return
    fi
    awk -v tam="$TAM" -v n="$nombre" -v u="$unidades" -v tp="$tiempoParalelo" -v ts="$TIEMPO_SECUENCIAL" -v csv="$CSV" 'BEGIN {
        s = ts / tp
        printf "  %-26s %8d %10.3f s %9.2f %10.0f%%\n", n, u, tp, s, 100 * s / u
        printf "%sx%s,%s,%d,%.4f,%.3f,%.3f\n", tam, tam, n, u, tp, s, s / u >> csv
    }'
}

echo "tamaño,version,unidades,tiempo_s,speedup,eficiencia" > "$CSV"

echo ""
echo "Filtro ${FILTRO}x${FILTRO} · ${NIVELES} niveles · mediana de ${REPETICIONES} ejecuciones"

for TAM in 800 2000 5000; do
    IMAGEN=../imagenes/imagen_${TAM}.png

    echo ""
    echo "== Imagen ${TAM}x${TAM} =="
    printf "  %-26s %8s %12s %9s %11s\n" "version" "unidades" "tiempo" "speedup" "eficiencia"

    TIEMPO_SECUENCIAL=$(medir ./secuencial "$IMAGEN" "$FILTRO" "$NIVELES")
    printf "  %-26s %8d %10.3f s %9s %11s\n" "Secuencial" 1 "$TIEMPO_SECUENCIAL" "1.00" "-"
    echo "${TAM}x${TAM},Secuencial,1,${TIEMPO_SECUENCIAL},1,1" >> "$CSV"

    fila "OpenMP" $THREADS \
        "$(medir ./openmp "$IMAGEN" "$FILTRO" "$NIVELES" $THREADS)"

    if [ -x "$INICIAL/mpi_inicial" ]; then
        fila "MPI inicial" $PROCESOS_MPI \
            "$(medir mpirun -np $PROCESOS_MPI "$INICIAL/mpi_inicial" "$IMAGEN" "$FILTRO" "$NIVELES")"
    fi

    fila "MPI corregido" $PROCESOS_MPI \
        "$(medir mpirun -np $PROCESOS_MPI ./mpi "$IMAGEN" "$FILTRO" "$NIVELES")"

    if [ -x "$INICIAL/hibrido_inicial" ]; then
        fila "Hibrido inicial" $((PROCESOS_HIBRIDO * THREADS)) \
            "$(medir mpirun -np $PROCESOS_HIBRIDO "$INICIAL/hibrido_inicial" "$IMAGEN" "$FILTRO" "$NIVELES" $THREADS)"
    fi

    fila "Hibrido corregido" $((PROCESOS_HIBRIDO * THREADS)) \
        "$(medir mpirun -np $PROCESOS_HIBRIDO ./hibrido "$IMAGEN" "$FILTRO" "$NIVELES" $THREADS)"
done

echo ""
echo "Resultados guardados en resultados/tiempos.csv"
