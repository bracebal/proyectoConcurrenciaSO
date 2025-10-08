# Nombre del ejecutable
TARGET = autopista_sim

# Compilador de C
CC = gcc

# Banderas de compilación
# -g:          Agrega información de depuración
# -Wall:       Habilita todas las advertencias del compilador
# -pthread:    Enlaza con la librería de Pthreads
CFLAGS = -g -Wall -pthread

# Archivos fuente
SOURCES = main.c vehiculo.c

# Archivos objeto
OBJECTS = $(SOURCES:.c=.o)

# Regla por defecto: compila todo el proyecto
all: $(TARGET)

# Regla para enlazar los archivos objeto y crear el ejecutable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Regla para compilar los archivos .c en archivos .o
%.o: %.c estructuras.h
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para limpiar los archivos generados
clean:
	rm -f $(OBJECTS) $(TARGET)

# Phony targets: le dice a make que 'all' y 'clean' no son nombres de archivos
.PHONY: all clean
