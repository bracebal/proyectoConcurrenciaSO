// Nombre del archivo: main.c
#include "autopista.h"
#include "utilidades.h"
#include "simulacion.h"

// ====================================================================
// VARIABLES GLOBALES
// ====================================================================
ParametrosSimulacion g_params;
Estadisticas g_estadisticas;
Subtramo g_subtramos[NUM_SUBTRAMOS];
volatile bool g_simulacion_activa = false;
bool g_verbose = false; // Bandera para modo detallado/depuración

// ====================================================================
// INICIALIZACIÓN Y LIMPIEZA DEL SISTEMA
// ====================================================================

void inicializar_sistema() {
    // Parámetros por defecto
    g_params.vehiculos_por_hora = 500;
    g_params.porcentaje_camiones = 20;
    g_params.duracion_simulacion_horas = 24;

    // Inicializar estadísticas y su mutex
    memset(&g_estadisticas, 0, sizeof(Estadisticas));
    pthread_mutex_init(&g_estadisticas.lock, NULL);

    // Inicializar semáforos de cada subtramo
    sem_init(&g_subtramos[0].sem_capacidad, 0, 4); // Subtramo 1: 4 vehículos
    sem_init(&g_subtramos[1].sem_capacidad, 0, 2); // Subtramo 2: 2 "pesos" (2 autos o 1 camión)
    sem_init(&g_subtramos[3].sem_capacidad, 0, 3); // Subtramo 4: 3 vehículos

    // Inicialización del subtramo 3 (el más complejo)
    g_subtramos[2].id = 2;
    sem_init(&g_subtramos[2].mutex, 0, 1);
    sem_init(&g_subtramos[2].puerta_victoria, 0, 0); // Puertas cerradas inicialmente
    sem_init(&g_subtramos[2].puerta_maracay, 0, 0);
    g_subtramos[2].vehiculos_dentro = 0;
    g_subtramos[2].esperando_victoria = 0;
    g_subtramos[2].esperando_maracay = 0;

    srand(time(NULL));
}

void limpiar_recursos() {
    pthread_mutex_destroy(&g_estadisticas.lock);
    for (int i = 0; i < NUM_SUBTRAMOS; i++) {
        if (i == 2) {
            sem_destroy(&g_subtramos[i].mutex);
            sem_destroy(&g_subtramos[i].puerta_victoria);
            sem_destroy(&g_subtramos[i].puerta_maracay);
        } else {
            sem_destroy(&g_subtramos[i].sem_capacidad);
        }
    }
}

// ====================================================================
// FUNCIÓN PRINCIPAL
// ====================================================================

int main(int argc, char *argv[]) {
    // Procesar banderas de la línea de comandos
    if (argc > 1 && strcmp(argv[1], "-v") == 0) {
        g_verbose = true;
        printf("========================================\n");
        printf("||    MODO DETALLADO DE PRUEBA ACTIVO   ||\n");
        printf("========================================\n");
    }

    inicializar_sistema();
    mostrar_menu(&g_params);
    limpiar_recursos();

    return 0;
}

