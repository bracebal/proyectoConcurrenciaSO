#ifndef CENTRAL_H
#define CENTRAL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

// ====================================================================
// DEFINICIONES Y ESTRUCTURAS DE DATOS
// ====================================================================

// --- Enumeraciones para estados y eventos ---
typedef enum {
    COLGADO,
    RECIBIENDO_LLAMADA,
    DESCOLGADO_ESPERANDO_MARCAR,
    MARCANDO,
    LLAMANDO,
    COMUNICANDO, // El otro extremo está ocupado
    EN_LLAMADA,
    TIMEOUT
} EstadoTelefono;

// --- Estructura para los parámetros de la simulación ---
typedef struct {
    int num_telefonos;
    int timeout1_sin_marcar;
    int timeout2_sin_respuesta;
    int duracion_simulacion;
} ParametrosSimulacion;

// --- Estructura para las estadísticas (Recurso Crítico) ---
typedef struct {
    int llamadas_efectivas;
    int llamadas_perdidas_timeout;
    int llamadas_ocupado;
    double tiempo_max_espera_conexion;
    sem_t lock; // Semáforo para proteger esta estructura
} Estadisticas;

// --- Estructura para representar un Teléfono (Recurso Crítico) ---
typedef struct {
    int id;
    EstadoTelefono estado;
    int numero_marcado;
    int conectado_con;
    time_t tiempo_ultimo_evento;
    sem_t lock; // Semáforo para proteger el estado de este teléfono
} Telefono;

// --- Estructura para la Central Telefónica (agrupa recursos) ---
typedef struct {
    Telefono** telefonos;
    int num_telefonos; // Para fácil acceso
} CentralTelefonica;

// --- Argumentos para los hilos (teléfono y central) ---
typedef struct {
    int id; // ID del teléfono, o -1 para la central
    CentralTelefonica* central;
    Estadisticas* estadisticas;
    ParametrosSimulacion* params;
    volatile bool* simulacion_activa;
} ArgsHilo;

#endif // CENTRAL_H
