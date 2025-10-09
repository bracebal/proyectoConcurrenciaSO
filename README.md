# Proyecto de Sincronización: Concurrencia con POSIX Threads y Semáforos

**Universidad de Carabobo (UC)- Facultad Experimental de Ciencias y Tecnología (FACyT)**
**Sistemas Operativos**
**Profesora: PhD Mirella Herrera**

## 👥 Autores

| Nombre | Cédula de Identidad (C.I.) |
|--------|----------------------------|
| ADRIÁN RAMÍREZ | 30.871.139 |
| BRAYAN CEBALLOS | 29.569.937 |
| JOSHTIN MEJÍAS | 31.071.016 |
| ULISES RONDÓN | 26.011.965 |

## 💡 Introducción: Justificación de la Sincronización

Ambos problemas fueron resueltos utilizando la biblioteca POSIX Threads (pthreads) para modelar la concurrencia, permitiendo que múltiples entidades (teléfonos, vehículos) operen simultáneamente.

La sincronización es implementada mediante Semáforos POSIX (`sem_t`) y Mutex POSIX (`pthread_mutex_t`) para garantizar:

- **Exclusión Mutua:** Proteger los recursos compartidos (estructuras de estadísticas, estado de recursos).
- **Limitación de Capacidad:** Controlar el aforo o la disponibilidad de recursos (tramos viales, la propia central).
- **Flujo Controlado:** Coordinar las acciones entre hilos que dependen de otros.

---

# PROBLEMA 1: Simulación de una Central Telefónica

## 📌 Descripción del Proyecto

Simulación de una central telefónica con N teléfonos. Cada teléfono es un hilo independiente que intenta realizar y recibir llamadas. El sistema utiliza mecanismos de sincronización para gestionar los estados de las llamadas, manejar tiempos de espera (timeouts) y evitar condiciones de carrera al actualizar las estadísticas globales.

La simulación se centra en la lógica de estados de un sistema de comunicaciones donde un hilo actúa como "Usuario" (cambiando el estado del teléfono) y otro hilo actúa como "Central" (procesando y conectando las llamadas).

## 🎯 Objetivos Específicos

- Modelar la transición de estados de un teléfono (Colgado, Marcando, Llamando, En Llamada, Timeout, Ocupado).
- Implementar timeouts de espera (T1 y T2) para llamadas sin marcar y sin respuesta.
- Utilizar Semáforos binarios para proteger el estado de cada teléfono y las estadísticas globales.
- Generar un reporte estadístico final que incluya métricas de éxito, fallos por timeout, llamadas ocupadas, y tiempos promedio de conexión/conversación.

## 📦 Estructura del Directorio `Problema 1`

| Archivo | Tipo | Descripción |
|---------|------|-------------|
| `main.c` | Fuente | Inicializa parámetros, estructuras, crea todos los hilos (`hilo_telefono` y `hilo_central`), y maneja el ciclo de vida de la simulación. |
| `central.h` | Cabecera | Define las estructuras `Telefono`, `Estadisticas` y `CentralTelefonica`, y las enumeraciones de estados. |
| `simulacion.c` | Fuente | Contiene las funciones clave de la lógica: `hilo_telefono` (simulación de la acción del usuario) y `hilo_central` (lógica de conexión). También incluye la función de reporte final. |
| `utilidades.c` / `utilidades.h` | Varios | Funciones de menú, manejo de parámetros y utilidades auxiliares. |
| `makefile` | Configuración | Script de compilación para generar el ejecutable `central_sim`. |

---

# PROBLEMA 2: Simulación de Tráfico Vehicular (Autopista)

## 📌 Descripción del Proyecto

Simulación de una autopista de cuatro tramos de circulación bidireccional, donde cada vehículo es un hilo que recorre la vía. El sistema utiliza Semáforos Contadores para modelar el aforo limitado de cada tramo y presenta un caso especial de restricción por peso en uno de ellos.

## 🎯 Objetivos Específicos

- Simular la lógica de tránsito de vehículos (Carros y Camiones) en ambos sentidos (1-4 y 4-1).
- Implementar la limitación de capacidad general en los tramos usando `sem_wait()` y `sem_post()`.
- Implementar la **Restricción de Peso en el Tramo 2**:
  - Carro: Consume 1 unidad de aforo.
  - Camión: Consume 2 unidades de aforo.
- Utilizar Mutex para garantizar la integridad de las estadísticas de espera y tránsito.
- Generar un Reporte Horario y Diario con métricas de congestión, incluyendo el máximo de vehículos en espera y el tiempo de espera promedio/máximo.

## 📦 Estructura del Directorio `Problema 2`

| Archivo | Tipo | Descripción |
|---------|------|-------------|
| `main.c` | Fuente | Inicializa los semáforos contadores y mutex globales (incluyendo el semáforo especial `sem_peso_tramo2`), y gestiona la creación de los hilos de vehículos por hora simulada. |
| `vehiculo.c` | Fuente | Contiene la función `logica_vehiculo` (la rutina principal del hilo). Implementa la lógica de entrada/salida de tramos, incluyendo la lógica de consumo de peso para el Tramo 2. |
| `estructuras.h` | Cabecera | Define `Vehiculo`, `SimParams`, `Estadisticas` y las declaraciones extern de los semáforos y mutex. |
| `makefile` | Configuración | Script de compilación para generar el ejecutable `autopista_sim`. |

---

# ⚙️ Instalación y Ejecución

Los Makefile de cada problema (`Problema 1/makefile` y `Problema 2/makefile`) simplifican la compilación y la ejecución.

## Requisitos

- Compilador de C (gcc).
- Librería de hilos POSIX (pthread).
- Herramienta GNU Make.

## 🛠 Compilación y Ejecución: Problema 1 (Central Telefónica)

```bash
# Navegar al directorio del problema
cd "Problema 1"

# Compilar (crea el ejecutable central_sim)
make

# Ejecutar simulación
./central_sim

# Limpiar archivos objeto y el ejecutable
make clean
```

## 🛠 Compilación y Ejecución: Problema 2 (Autopista)

```bash

# Navegar al directorio del problema
cd "Problema 2"

# Compilar (crea el ejecutable autopista_sim)
make

# Ejecutar la simulación
./autopista_sim

# Limpiar archivos objeto y el ejecutable
make clean