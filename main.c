#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>  // Para medir el tiempo

#define MAX_N    5000     // Número máximo de filas/columnas (5000)
#define MAX_NNZ  50000000  // Número máximo de elementos no cero (50,000,000)
#define MAX_LINE 256      // Tamaño máximo para leer cada línea

// --------------------------------------------------------------------
// Función auxiliar: get_next_data_line
//
// Lee el siguiente renglón del archivo "fp" y lo almacena en "buffer" (tamaño size),
// ignorando líneas vacías o que comiencen con '#' (comentarios).
// Además, si en la línea aparece el carácter '#' en medio, se elimina el comentario.
// Retorna 1 si se leyó una línea con datos, o 0 si se alcanzó el fin del archivo.
// --------------------------------------------------------------------
int get_next_data_line(FILE *fp, char *buffer, int size) {
    while (fgets(buffer, size, fp)) {
        // Eliminar salto de línea
        buffer[strcspn(buffer, "\n")] = '\0';
        
        // Saltar líneas vacías
        char *p = buffer;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0') continue;
        
        // Saltar líneas que comiencen con '#' (comentarios)
        if (*p == '#')
            continue;
        
        // Eliminar comentario inline (si existe)
        char *comentario = strchr(buffer, '#');
        if (comentario)
            *comentario = '\0';
        
        // Eliminar espacios finales
        int len = strlen(buffer);
        while (len > 0 && isspace((unsigned char)buffer[len-1])) {
            buffer[len-1] = '\0';
            len--;
        }
        if (strlen(buffer) == 0)
            continue;
        
        return 1;  // Se obtuvo una línea válida
    }
    return 0;  // Fin del archivo
}

// --------------------------------------------------------------------
// Función para realizar la multiplicación matriz-vector en formato CSR (secuencial)
//
// Notas de paralelización:
// 1. Para paralelizar la multiplicación local (Superstep 1), se podría agregar
//    una directiva OpenMP en el bucle (por ejemplo, #pragma omp parallel for schedule(dynamic))
// 2. En una versión MPI, cada proceso trabajaría sobre su subconjunto de filas,
//    y luego se reunirían las contribuciones parciales.
// --------------------------------------------------------------------
void sparse_matvec_seq(
    int n,             // Número de filas de la matriz
    double *values,    // Array de valores no cero (longitud = nnz)
    int *colIndex,     // Array de índices de columna (longitud = nnz)
    int *rowStart,     // Array de punteros de fila (formato CSR, longitud = n+1)
    double *vector,    // Vector de entrada (longitud = n)
    double *result     // Vector de salida (resultado de A * vector)
) {
    // Bucle de multiplicación local (secuencial)
    // Para OpenMP, descomentar la siguiente línea:
    // #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < n; i++) {
        double suma = 0.0;
        for (int j = rowStart[i]; j < rowStart[i+1]; j++) {
            suma += values[j] * vector[colIndex[j]];
        }
        result[i] = suma;
    }
}

int main(int argc, char *argv[]) {
    int n, nnz, i;
    char line[MAX_LINE];

    // Verificar que se proporcione el nombre del archivo de datos como argumento
    if (argc < 2) {
        printf("Uso: %s <archivo_datos>\n", argv[0]);
        return 1;
    }

    // Abrir el archivo de datos para lectura
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("Error: No se pudo abrir el archivo %s\n", argv[1]);
        return 1;
    }

    // Abrir el archivo de salida "resultados.txt" para escribir el detalle del resultado
    FILE *fout = fopen("results/results.txt", "a");
    if (fout == NULL) {
        printf("Error: No se pudo abrir el archivo resultados.txt para escritura.\n");
        fclose(fp);
        return 1;
    }

    // Abrir el archivo de salida "tiempo.txt" para escribir el tiempo de ejecución
    FILE *ftime = fopen("results/time.txt", "a");
    if (ftime == NULL) {
        printf("Error: No se pudo abrir el archivo tiempo.txt para escritura.\n");
        fclose(fp);
        fclose(fout);
        return 1;
    }

    // Reservar dinámicamente memoria para los arrays grandes
    double *values = malloc(MAX_NNZ * sizeof(double));
    int *colIndex = malloc(MAX_NNZ * sizeof(int));
    int *tempRow = malloc(MAX_NNZ * sizeof(int));
    int *tempCol = malloc(MAX_NNZ * sizeof(int));
    double *tempVal = malloc(MAX_NNZ * sizeof(double));
    if (!values || !colIndex || !tempRow || !tempCol || !tempVal) {
        printf("Error reservando memoria para la matriz.\n");
        fclose(fp);
        fclose(fout);
        fclose(ftime);
        return 1;
    }

    // Reservar memoria para los arrays de tamaño n (más pequeños)
    int *rowStart = malloc((MAX_N + 1) * sizeof(int));
    int *currentRowStart = malloc((MAX_N + 1) * sizeof(int));
    double *vector = malloc(MAX_N * sizeof(double));
    double *result = malloc(MAX_N * sizeof(double));
    if (!rowStart || !currentRowStart || !vector || !result) {
        printf("Error reservando memoria para vectores o índices de filas.\n");
        fclose(fp);
        fclose(fout);
        fclose(ftime);
        return 1;
    }

    // -----------------------------------------------------------
    // Paso 1: Leer la primera línea con dimensiones y nnz
    // -----------------------------------------------------------
    if (!get_next_data_line(fp, line, MAX_LINE)) {
        printf("Error: No se encontró la línea de dimensiones y nnz.\n");
        fclose(fp);
        fclose(fout);
        fclose(ftime);
        return 1;
    }
    if (sscanf(line, "%d %d", &n, &nnz) != 2) {
        printf("Error leyendo n y nnz.\n");
        fclose(fp);
        fclose(fout);
        fclose(ftime);
        return 1;
    }
    if (n > MAX_N || nnz > MAX_NNZ) {
        printf("Error: n o nnz exceden los límites máximos (%d y %d respectivamente).\n", MAX_N, MAX_NNZ);
        fclose(fp);
        fclose(fout);
        fclose(ftime);
        return 1;
    }

    // -----------------------------------------------------------
    // Paso 2: Leer los elementos no cero de la matriz (formato COO)
    // Se esperan nnz líneas de datos
    // -----------------------------------------------------------
    for (i = 0; i < nnz; i++) {
        if (!get_next_data_line(fp, line, MAX_LINE)) {
            printf("Error: No se pudo leer el elemento no cero %d.\n", i);
            fclose(fp);
            fclose(fout);
            fclose(ftime);
            return 1;
        }
        if (sscanf(line, "%d %d %lf", &tempRow[i], &tempCol[i], &tempVal[i]) != 3) {
            printf("Error en el formato del elemento no cero %d.\n", i);
            fclose(fp);
            fclose(fout);
            fclose(ftime);
            return 1;
        }
    }

    // -----------------------------------------------------------
    // Paso 3: Construir la representación CSR de la matriz
    // -----------------------------------------------------------
    for (i = 0; i < n; i++)
        rowStart[i] = 0;
    rowStart[n] = 0;

    // Contar elementos no cero por fila
    for (i = 0; i < nnz; i++) {
        int r = tempRow[i];
        rowStart[r]++;
    }

    // Convertir contadores en índices de inicio (suma acumulativa)
    int suma = 0;
    for (i = 0; i < n; i++) {
        int cuenta = rowStart[i];
        rowStart[i] = suma;
        suma += cuenta;
    }
    rowStart[n] = suma;

    // Copiar rowStart a un array auxiliar
    for (i = 0; i <= n; i++) {
        currentRowStart[i] = rowStart[i];
    }

    // Insertar los elementos en los arrays CSR (values y colIndex)
    for (i = 0; i < nnz; i++) {
        int r = tempRow[i];
        int pos = currentRowStart[r];
        values[pos] = tempVal[i];
        colIndex[pos] = tempCol[i];
        currentRowStart[r]++;
    }

    // -----------------------------------------------------------
    // Paso 4: Leer el vector (se esperan n líneas de datos)
    // -----------------------------------------------------------
    for (i = 0; i < n; i++) {
        if (!get_next_data_line(fp, line, MAX_LINE)) {
            printf("Error: No se pudo leer el elemento %d del vector.\n", i);
            fclose(fp);
            fclose(fout);
            fclose(ftime);
            return 1;
        }
        if (sscanf(line, "%lf", &vector[i]) != 1) {
            printf("Error en el formato del vector en la línea %d.\n", i);
            fclose(fp);
            fclose(fout);
            fclose(ftime);
            return 1;
        }
    }
    fclose(fp);

    // -----------------------------------------------------------
    // Paso 5: Realizar la multiplicación matriz-vector y medir el tiempo
    // -----------------------------------------------------------
    clock_t start = clock();
    sparse_matvec_seq(n, values, colIndex, rowStart, vector, result);
    clock_t end = clock();
    double elapsed_time = ((double)(end - start)) / CLOCKS_PER_SEC;

    // -----------------------------------------------------------
    // Paso 6: Escribir el resultado en "resultados.txt" y el tiempo en "tiempo.txt"
    //         incluyendo el nombre del archivo de prueba
    // -----------------------------------------------------------
    fprintf(fout, "Prueba: %s\n\n", argv[1]);
    fprintf(fout, "Resultado de A * vector:\n");
    for (i = 0; i < n; i++) {
        fprintf(fout, "result[%d] = %f\n", i, result[i]);
    }
    fprintf(fout, "\n-------------------------------------------------\n\n");

    fprintf(ftime, "Prueba: %s\n", argv[1]);
    fprintf(ftime, "Multiplicación completada en %f segundos.\n\n", elapsed_time);

    // También mostrar en consola
    printf("Prueba: %s\n\n", argv[1]);
    printf("Resultado de A * vector:\n");
    for (i = 0; i < n; i++) {
        printf("result[%d] = %f\n", i, result[i]);
    }
    printf("\nMultiplicación completada en %f segundos.\n", elapsed_time);

    // Cerrar archivos de salida
    fclose(fout);
    fclose(ftime);

    // Liberar memoria dinámica
    free(values);
    free(colIndex);
    free(tempRow);
    free(tempCol);
    free(tempVal);
    free(rowStart);
    free(currentRowStart);
    free(vector);
    free(result);

    return 0;
}