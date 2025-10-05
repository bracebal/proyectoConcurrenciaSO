#include "utilidades.h"

// Accede a la bandera global definida en main.c
extern bool g_verbose;

// --- Implementación de funciones auxiliares ---

const char* estado_a_texto(EstadoTelefono estado) {
    switch (estado) {
        case COLGADO:                     return "Colgado";
        case RECIBIENDO_LLAMADA:          return "Ring-Ring";
        case DESCOLGADO_ESPERANDO_MARCAR: return "Piii... (Tono de marcado)";
        case MARCANDO:                    return "Marcando...";
        case LLAMANDO:                    return "Piii-Piii (Llamando)";
        case COMUNICANDO:                 return "Tuu-Tuu-Tuu (Ocupado)";
        case EN_LLAMADA:                  return "En llamada";
        case TIMEOUT:                     return "Tuu-Tuu-Tuu (Timeout)";
        default:                          return "Desconocido";
    }
}

void ver_consumo_recursos() {
    printf("\n--- Consumo de Recursos del Proceso ---\n");
    // Esta implementación es para sistemas Linux/POSIX.
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

// --- Implementación de funciones del menú ---

void modificar_parametros(ParametrosSimulacion* p) {
    printf("\n--- Modificar Parámetros de Simulación ---\n");
    printf("Deje el campo en blanco para mantener el valor actual.\n\n");
    char buffer[100];

    printf("Número de teléfonos (actual: %d): ", p->num_telefonos);
    fgets(buffer, sizeof(buffer), stdin);
    if (atoi(buffer) > 0) p->num_telefonos = atoi(buffer);

    printf("Timeout sin marcar (segundos, actual: %d): ", p->timeout1_sin_marcar);
    fgets(buffer, sizeof(buffer), stdin);
    if (atoi(buffer) > 0) p->timeout1_sin_marcar = atoi(buffer);
    
    printf("Timeout sin respuesta (segundos, actual: %d): ", p->timeout2_sin_respuesta);
    fgets(buffer, sizeof(buffer), stdin);
    if (atoi(buffer) > 0) p->timeout2_sin_respuesta = atoi(buffer);

    printf("Duración de la simulación (segundos, actual: %d): ", p->duracion_simulacion);
    fgets(buffer, sizeof(buffer), stdin);
    if (atoi(buffer) > 0) p->duracion_simulacion = atoi(buffer);
    
    printf("\nParámetros actualizados.\n");
}

void mostrar_menu(ParametrosSimulacion* params) {
    // Declaración de la función que inicia la simulación (definida en simulacion.c)
    void iniciar_simulacion(ParametrosSimulacion* params);
    
    int opcion;
    char buffer[10];
    do {
        printf("\n========= MENÚ CENTRAL TELEFÓNICA =========\n");
        printf("1. Iniciar simulación\n");
        printf("2. Modificar parámetros\n");
        printf("3. Ver consumo de recursos (Linux)\n");
        printf("4. Salir\n");
        printf("-------------------------------------------\n");
        printf("Parámetros: %d Tel, %ds Sim, T1=%ds, T2=%ds | Modo Detallado: %s\n", 
               params->num_telefonos, params->duracion_simulacion, 
               params->timeout1_sin_marcar, params->timeout2_sin_respuesta,
               g_verbose ? "Activado" : "Desactivado");
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

