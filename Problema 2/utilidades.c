#include "utilidades.h"

extern bool g_verbose; // Accede a la bandera definida en main.c

// --- Implementación de funciones auxiliares ---

const char* tipo_a_texto(TipoVehiculo t) { return t == AUTO ? "Auto" : "Camión"; }
const char* dir_a_texto(Direccion d) { return d == HACIA_VICTORIA ? "1->4" : "4->1"; }

double tiempo_actual_preciso() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
}

void ver_consumo_recursos() {
    printf("\n--- Consumo de Recursos del Proceso ---\n");
    // Implementación es para sistemas Linux/POSIX.
    FILE* fp = fopen("/proc/self/status", "r");
    if (fp == NULL) {
        perror("No se pudo abrir /proc/self/status. Función compatible con Linux.");
        return;
    }
    char linea[256];
    while (fgets(linea, sizeof(linea), fp)) {
        if (strncmp(linea, "VmSize:", 7) == 0 || strncmp(linea, "VmRSS:", 6) == 0 || strncmp(linea, "Threads:", 8) == 0) {
            printf("%s", linea);
        }
    }
    fclose(fp);
    printf("----------------------------------------\n\n");
}

// --- Implementación de funciones para el menú ---

void modificar_parametros(ParametrosSimulacion* p) {
    printf("\n--- Modificar Parámetros de Simulación ---\n");
    printf("Deje el campo en blanco para mantener el valor actual.\n\n");
    char buffer[100];

    printf("Vehículos por hora (actual: %d): ", p->vehiculos_por_hora);
    fgets(buffer, sizeof(buffer), stdin);
    if (atoi(buffer) > 0) p->vehiculos_por_hora = atoi(buffer);

    printf("Porcentaje de camiones (%%, actual: %d): ", p->porcentaje_camiones);
    fgets(buffer, sizeof(buffer), stdin);
    if (atoi(buffer) >= 0 && atoi(buffer) <= 100) p->porcentaje_camiones = atoi(buffer);

    printf("Duración simulación (horas, actual: %d): ", p->duracion_simulacion_horas);
    fgets(buffer, sizeof(buffer), stdin);
    if (atoi(buffer) > 0) p->duracion_simulacion_horas = atoi(buffer);

    printf("\nParámetros actualizados.\n");
}

void mostrar_menu(ParametrosSimulacion* params) {
    // Declaración de la función que inicia la simulación (definida previamente en simulacion.c)
    void iniciar_simulacion(ParametrosSimulacion* params);
    
    int opcion;
    char buffer[10];
    do {
        printf("\n============ MENÚ SIMULACIÓN AUTOPISTA ============\n");
        printf("1. Iniciar simulación\n");
        printf("2. Modificar parámetros\n");
        printf("3. Ver consumo de recursos (Linux)\n");
        printf("4. Salir\n");
        printf("---------------------------------------------------\n");
        printf("Parámetros: %d veh/h, %d%% camiones, %d horas | Modo Detallado: %s\n",
               params->vehiculos_por_hora, params->porcentaje_camiones,
               params->duracion_simulacion_horas, g_verbose ? "Activado" : "Desactivado");
        printf("Seleccione una opción: ");

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            opcion = 4;
        } else {
            opcion = atoi(buffer);
        }

        switch (opcion) {
            case 1: iniciar_simulacion(params); break;
            case 2: modificar_parametros(params); break;
            case 3: ver_consumo_recursos(); break;
            case 4: printf("Saliendo del programa.\n"); break;
            default: printf("Opción no válida.\n"); break;
        }
    } while (opcion != 4);
}
