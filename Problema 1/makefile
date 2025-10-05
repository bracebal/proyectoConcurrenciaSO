# Makefile para el Proyecto de Simulación de Central Telefónica
# Estructura modular para facilitar la compilación y el mantenimiento.

# --- Variables ---
CC = gcc
CFLAGS = -Wall -Wextra -g -O2
LDFLAGS = -pthread

# --- Archivos ---
TARGET = central_sim
SOURCES = main.c simulacion.c utilidades.c
OBJECTS = $(SOURCES:.c=.o)

# --- Reglas ---

# Regla por defecto
all: $(TARGET)

# Regla para enlazar y crear el ejecutable final
$(TARGET): $(OBJECTS)
	@echo "Enlazando para crear el ejecutable: $@"
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo ">>> Ejecutable '$(TARGET)' creado exitosamente. <<<"
	@echo ">>> Ejecutar con: ./$(TARGET) [-v] <<<"

# Regla genérica para compilar .c a .o
%.o: %.c
	@echo "Compilando: $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para limpiar el directorio
clean:
	@echo "Limpiando archivos compilados..."
	rm -f $(OBJECTS) $(TARGET)
	@echo "Limpieza completada."

.PHONY: all clean

