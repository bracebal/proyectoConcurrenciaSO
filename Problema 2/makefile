# --- Variables ---
CC = gcc
# CFLAGS: Banderas para el compilador
# -Wall: Activa todas las advertencias
# -Wextra: Activa advertencias adicionales
# -g: Incluye información de depuración
# -O2: Optimización de nivel 2 para rendimiento
CFLAGS = -Wall -Wextra -g -O2
# LDFLAGS: Banderas para el enlazador (linker)
# -pthread: Enlaza la librería POSIX Threads (necesaria para hilos y semáforos)
LDFLAGS = -pthread

# --- Archivos ---
# El nombre del programa ejecutable final
TARGET = autopista_sim
# Lista de todos los archivos fuente (.c)
SOURCES = main.c simulacion.c utilidades.c
# Genera automáticamente la lista de archivos objeto (.o) a partir de los fuentes
OBJECTS = $(SOURCES:.c=.o)

# --- Reglas ---

# Regla por defecto: se ejecuta al escribir "make" o "make all"
# Compila el programa completo.
all: $(TARGET)

# Regla para enlazar los archivos objeto y crear el ejecutable final
# Depende de todos los archivos objeto.
$(TARGET): $(OBJECTS)
	@echo "Enlazando para crear el ejecutable: $@"
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo ">>> Ejecutable '$(TARGET)' creado exitosamente. <<<"
	@echo ">>> Ejecutar con: ./$(TARGET) [-v] <<<"

# Regla genérica para compilar cualquier archivo .c en su .o correspondiente
# %.o: %.c significa "para crear un archivo que termina en .o, busca uno con el mismo nombre que termina en .c"
%.o: %.c
	@echo "Compilando: $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para limpiar el directorio
# Elimina los archivos objeto y el ejecutable.
clean:
	@echo "Limpiando archivos compilados..."
	rm -f $(OBJECTS) $(TARGET)
	@echo "Limpieza completada."

# .PHONY: Le dice a make que "all" y "clean" no son nombres de archivos reales,
# sino nombres de recetas que siempre deben ejecutarse cuando se les llama.
.PHONY: all clean