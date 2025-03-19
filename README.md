# Multiplicación Matriz Dispersa por Vector (Formato CSR)

Este repositorio implementa la multiplicación secuencial de una matriz dispersa (formato COO convertido a CSR) por un vector. Se mide el tiempo de ejecución y los resultados se guardan en archivos separados.

## Estructura del Repositorio

```
.
├── data
│   ├── test_1.txt
│   ├── test_2.txt
│   └── test_3.txt
├── results
│   ├── results.txt
│   └── time.txt
├── utils
│   └── generar_txt.py
├── .gitignore
├── main
└── main.c
```

- **data/**: Archivos con casos de prueba (matriz en formato COO y vector).
- **results/**: Archivos generados:
  - `results.txt`: Resultados de las multiplicaciones.
  - `time.txt`: Tiempos de ejecución.
- **utils/**:
  - `generar_txt.py`: Script Python para generar casos de prueba personalizados.
- **main.c**: Código fuente principal.
- **main**: Ejecutable compilado.

## Compilación

Compila usando `gcc`:

```bash
gcc -o main main.c -O2
```

Para OpenMP (opcional):

```bash
gcc -o main main.c -O2 -fopenmp
```

## Ejecución

Ejecuta con:

```bash
./main data/test_1.txt
```

El programa hace lo siguiente:
1. Lee matriz dispersa (formato COO) y vector.
2. Convierte la matriz a formato CSR.
3. Realiza multiplicación y mide el tiempo.
4. Guarda resultados en:
   - `results/results.txt`
   - `results/time.txt`

## Ver Resultados

Consulta:

- **Resultados detallados:** `results/results.txt`
- **Tiempo ejecución:** `results/time.txt`

## Generar Nuevas Pruebas

Puedes crear archivos personalizados usando:

```bash
python3 utils/generar_txt.py
```

## Notas Importantes

- Ajusta `MAX_N` y `MAX_NNZ` en `main.c` según el tamaño de tu prueba.
- El tiempo se mide con `clock()` (tiempo CPU). Para tiempo real (wall-clock), utiliza funciones como `gettimeofday()` o `clock_gettime()`.
- Para matrices grandes, verifica la memoria disponible.

¡Explora y experimenta con la multiplicación de matrices dispersas!

