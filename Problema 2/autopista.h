#ifndef AUTOPISTA_H
#define AUTOPISTA_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

// ====================================================================
// DEFINICIONES Y ESTRUCTURAS DE DATOS
// ====================================================================

// --- Constantes de la simulación ---
#define NUM_SUBTRAMOS 4

// --- Enumeraciones ---
typedef enum { AUTO, CAMION } TipoVehiculo;
typedef enum { HACIA_VICTORIA, HACIA_MARACAY } Direccion; // 1->4, 4->1

// --- Estructura para los parámetros de la simulación ---
typedef struct {
    int vehiculos_por_hora;
    int porcentaje_camiones;
    int duracion_simulacion_horas;
} ParametrosSimulacion;

// --- Estructura para las estadísticas (Recurso Crítico) ---
typedef struct {
    long long vehiculos_por_subtramo_total[NUM_SUBTRAMOS][2];
    long long vehiculos_por_subtramo_hora[NUM_SUBTRAMOS][2];
    int max_vehiculos_esperando;
    double max_tiempo_espera;
    int vehiculos_esperando_actual;
    pthread_mutex_t lock; // Se usa un mutex para proteger las estadísticas
} Estadisticas;

// --- Estructura para un Subtramo (Recurso Crítico) ---
typedef struct {
    int id;

    // --- Mecanismos de Sincronización con Semáforos ---
    sem_t sem_capacidad;  // Semáforo contador para la capacidad (subtramos 1, 2, 4)
    sem_t mutex;          // Semáforo binario para exclusión mutua (especialmente para tramo 3)

    // Estado específico del subtramo 3 (un solo carril)
    int vehiculos_dentro;
    Direccion direccion_actual;
    int esperando_victoria;
    int esperando_maracay;
    sem_t puerta_victoria;
    sem_t puerta_maracay;

} Subtramo;

// --- Estructura para un Vehículo ---
typedef struct {
    int id;
    TipoVehiculo tipo;
    Direccion direccion;
    pthread_t hilo_id;

    // Punteros a los recursos compartidos
    Subtramo* subtramos;
    Estadisticas* estadisticas;
    volatile bool* simulacion_activa;

} Vehiculo;

#endif // AUTOPISTA_H
