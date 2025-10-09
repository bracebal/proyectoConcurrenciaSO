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

#define SIMULATION_SPEEDUP_FACTOR 3600

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
    COMUNICANDO, // El otro extremo esta ocupado
    EN_LLAMADA,
    TIMEOUT
} EstadoTelefono;

// --- Estructura para los parametros de la simulacion ---
typedef struct {
    int num_telefonos;
    int timeout1_sin_marcar;
    int timeout2_sin_respuesta;
    int duracion_simulacion;
} ParametrosSimulacion;

// --- Estructura para las estadisticas (Recurso Critico) ---
typedef struct {
    int llamadas_efectivas;
    int llamadas_perdidas_timeout;
    int llamadas_ocupado;
    
    // Metricas existentes
    int llamadas_intentadas_total; 
    int llamadas_atendidas;        
    int descolgados_total;         
    double tiempo_total_conversacion;
    double tiempo_max_espera_conexion;

    // NUEVAS ESTADISTICAS ADICIONADAS
    double total_tiempo_espera_conexion; // Suma de los tiempos de espera de todas las llamadas efectivas
    double max_duracion_conversacion;    // La duración mós larga de una llamada EN_LLAMADA
    
    sem_t lock; // Semaforo para proteger esta estructura
} Estadisticas;

// --- Estructura para representar un Teléfono (Recurso Crítico) ---
typedef struct {
    int id;
    EstadoTelefono estado;
    int numero_marcado;
    int conectado_con;
    struct timespec tiempo_ultimo_evento;
    sem_t lock; // Semaforo para proteger el estado de este telefono
} Telefono;

// --- Estructura para la Central Telefonica (agrupa recursos) ---
typedef struct {
    Telefono** telefonos;
    int num_telefonos; // Para facil acceso
} CentralTelefonica;

// --- Argumentos para los hilos (telefono y central) ---
typedef struct {
    int id; // ID del telefono, o -1 para la central
    CentralTelefonica* central;
    Estadisticas* estadisticas;
    ParametrosSimulacion* params;
    volatile bool* simulacion_activa;
} ArgsHilo;

#endif // CENTRAL_H
