#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

// --- Valores de caso base para la Simulación ---

// Tiempos (en microsegundos)
#define TIEMPO_SIMULACION_HORA 1000000 // 1 segundo real = 1 hora simulada
#define TIEMPO_EN_TRAMO_MIN 3000       // Tiempo mínimo que un vehículo pasa en un tramo
#define TIEMPO_EN_TRAMO_MAX 6000      // Tiempo máximo que un vehículo pasa en un tramo

// Tipos de Vehículo
#define TIPO_CARRO 0
#define TIPO_CAMION 1

// Sentidos de Circulación
#define SENTIDO_1_A_4 0
#define SENTIDO_4_A_1 1

// --- Estructuras de Datos ---

// Estructura para almacenar los parámetros de la simulación
typedef struct {
    int vehiculos_por_hora;
    int porcentaje_camiones;
    int duracion_horas;
} SimParams;

// Estructura para representar un vehículo
typedef struct {
    int id;
    int tipo;      // TIPO_CARRO o TIPO_CAMION
    int sentido;   // SENTIDO_1_A_4 o SENTIDO_4_A_1
    struct timespec tiempo_inicio_espera; // Usamos timespec para aumentar la precisión
} Vehiculo;

// Estructura para almacenar los datos del reporte horario (por hora)
typedef struct {
    int vehiculos_por_tramo[4][2]; // [tramo][sentido]
} ReporteHora;


// Estructura para las estadísticas globales
typedef struct {
    // Contadores por hora (se resetean cada hora)
    int vehiculos_por_tramo_hora[4][2];

    // Contadores totales del día
    int vehiculos_por_tramo_dia[4][2];

    // Métricas de espera
    int max_vehiculos_esperando[4][2];
    double sum_tiempo[4][2];
    double max_tiempo_espera[4][2];
    int vehiculos_esperando_actual[4][2];
    int vehiculo_max_espera;

    // Mutex para proteger el acceso a las estadísticas
    pthread_mutex_t mutex_stats;
} Estadisticas;


// --- Variables y Semáforos (declarados como extern) ---

extern sem_t sem_tramo[4];
extern sem_t sem_peso_tramo2;
extern pthread_mutex_t mutex_print;
extern pthread_mutex_t mutex_tramo2_entry;
extern Estadisticas stats;

// --- Prototipos de funciones ---
void* logica_vehiculo(void* arg);
void inicializar_recursos();
void destruir_recursos();
void imprimir_reporte_final();
void resetear_stats_hora();
void imprimir_recursos_sistema();
int mostrar_menu(SimParams *params);

#endif // ESTRUCTURAS_H