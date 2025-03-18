# Script: generar_prueba2.py
n = 5000
nnz = 50000000


with open("test_3.txt", "w") as f:
    # Escribir dimensiones
    f.write(f"{n} {nnz}   # n y nnz\n\n")
    
    # Escribir datos de la matriz
    for i in range(nnz):
        fila = i % n
        columna = (i * 7) % n
        valor = (i % 100) + 1.0
        f.write(f"{fila} {columna} {valor}   # elemento {i}\n")
    
    f.write("\n")
    
    # Escribir el vector (todos con valor 1)
    for i in range(n):
        f.write(f"1   # vector[{i}] = 1\n")
