# Makefile para el Proyecto de Sincronización de Sistemas Operativos (Versión Mejorada)
#
# Comandos:
#   make all                - Compila ambos programas. Es el comando por defecto.
#   make problema1_solucion - Compila solo la solución del problema 1.
#   make problema2_solucion - Compila solo la solución del problema 2.
#   make clean              - Elimina los archivos ejecutables generados.

# Compilador a utilizar
CC = gcc

# Banderas de compilación:
# -Wall: Muestra todas las advertencias (warnings).
# -Wextra: Muestra advertencias adicionales.
# -pthread: Enlaza la librería de Pthreads, necesaria para hilos.
# -O3: Optimización de nivel 3 para mejor rendimiento.
# -o $@: Indica que el archivo de salida ($@) tendrá el mismo nombre que el target.
# $<: Representa el primer prerrequisito (el archivo .c).
CFLAGS = -Wall -Wextra -pthread -O3
LDFLAGS = -pthread

# Target por defecto, que se ejecuta si solo escribes "make"
all: problema1_solucion problema2_solucion

# Regla para compilar la solución del problema 1
problema1_solucion: problema1_solucion.c
	$(CC) $(CFLAGS) -o $@ $< -lm
	@echo "Programa 'problema1_solucion' compilado exitosamente."

# Regla para compilar la solución del problema 2
problema2_solucion: problema2_solucion.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Programa 'problema2_solucion' compilado exitosamente."

# Regla para limpiar los archivos compilados
clean:
	rm -f problema1_solucion problema2_solucion
	@echo "Archivos compilados eliminados."
