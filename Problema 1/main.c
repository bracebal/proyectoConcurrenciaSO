#include "central.h"
#include "utilidades.h"
#include "simulacion.h"

// ====================================================================
// VARIABLES
// ====================================================================
ParametrosSimulacion g_params;
Estadisticas g_estadisticas;
CentralTelefonica g_central;
volatile bool g_simulacion_activa = false;
bool g_verbose = false; // Bandera

// ====================================================================
// INICIALIZACIÓN Y LIMPIEZA DEL SISTEMA
// ====================================================================

void inicializar_sistema() {
    // Parámetros por defecto
    g_params.num_telefonos = 100;
    g_params.timeout1_sin_marcar = 10;
    g_params.timeout2_sin_respuesta = 15;
    g_params.duracion_simulacion = 60; // 60 segundo = 1 minuto

    // Inicializar estadísticas y su semáforo
    memset(&g_estadisticas, 0, sizeof(Estadisticas));
    sem_init(&g_estadisticas.lock, 0, 1); // Semáforo binario (mutex)

    // Inicializar la central y los teléfonos
    g_central.telefonos = malloc(sizeof(Telefono*) * g_params.num_telefonos);
    g_central.num_telefonos = g_params.num_telefonos;
    for (int i = 0; i < g_params.num_telefonos; ++i) {
        g_central.telefonos[i] = malloc(sizeof(Telefono));
        g_central.telefonos[i]->id = i;
        g_central.telefonos[i]->estado = COLGADO;
        g_central.telefonos[i]->conectado_con = -1;
        g_central.telefonos[i]->numero_marcado = -1;
        g_central.telefonos[i]->tiempo_ultimo_evento = time(NULL);
        sem_init(&g_central.telefonos[i]->lock, 0, 1); // Semáforo binario para cada teléfono
    }
    
    srand(time(NULL));
}

void limpiar_recursos() {
    sem_destroy(&g_estadisticas.lock);
    for (int i = 0; i < g_params.num_telefonos; ++i) {
        sem_destroy(&g_central.telefonos[i]->lock);
        free(g_central.telefonos[i]);
    }
    free(g_central.telefonos);
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
