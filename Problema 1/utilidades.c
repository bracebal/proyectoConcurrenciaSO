#include "utilidades.h"
#include "simulacion.h"
#include <ctype.h>
#include "central.h"


// Accede a la bandera
extern bool g_verbose;

// --- Implementación de funciones ---

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
    // Implementación para sistemas Linux/POSIX.
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

static int validar_entrada_entero(const char* mensaje, int valor_actual) {
    char buffer[10];
    int valor_ingresado;

    do {
        printf("%s (Actual: %d): ", mensaje, valor_actual);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL || buffer[0] == '\n') {
            printf("Error: Entrada vacía. Por favor, ingrese un número.\n");
            continue;
        }

        buffer[strcspn(buffer, "\n")] = 0;
        bool es_valido = true;
        for (int i = 0; buffer[i] != '\0'; i++) {
            if (i == 0 && buffer[i] == '-') continue;
            if (!isdigit((unsigned char)buffer[i])) {
                es_valido = false;
                break;
            }
        }

        if (!es_valido) {
            printf("Error: Por favor, ingrese solo caracteres numéricos.\n");
            continue;
        }

        valor_ingresado = atoi(buffer);
        
        if (valor_ingresado > 0) {
            return valor_ingresado;
        } else {
            printf("Error: El valor debe ser un número entero positivo (mayor que 0).\n");
        }
    } while (true);
}


void modificar_parametros(ParametrosSimulacion* params) {
    printf("\n========= MODIFICAR PARÁMETROS =========\n");
    int nuevo_num_telefonos;
    do {
        nuevo_num_telefonos = validar_entrada_entero(
            "Ingrese nuevo número de teléfonos (entre 2 y 500)",
            params->num_telefonos
        );
        if (nuevo_num_telefonos < 2 || nuevo_num_telefonos > 500) {
            printf("Error: El número de teléfonos debe estar entre 2 y 500.\n");
        }
    } while (nuevo_num_telefonos < 2 || nuevo_num_telefonos > 500);
    params->num_telefonos = nuevo_num_telefonos;

    params->timeout1_sin_marcar = validar_entrada_entero(
        "Ingrese nuevo Timeout 1 (segundos para marcar después de descolgar, T1)", 
        params->timeout1_sin_marcar
    );
    
    params->timeout2_sin_respuesta = validar_entrada_entero(
        "Ingrese nuevo Timeout 2 (segundos para contestar después de llamar, T2)", 
        params->timeout2_sin_respuesta
    );

    params->duracion_simulacion = SIMULATION_SPEEDUP_FACTOR * validar_entrada_entero(
        "Ingrese nueva duración de la simulación (horas)", 
        params->duracion_simulacion/3600
    );

    char buffer[10];
    int opcion_verbose;
    do {
        printf("Modo Detallado (Verbose) (Actual: %s). 1=Activar, 2=Desactivar: ", g_verbose ? "Activado" : "Desactivado");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            opcion_verbose = 0;
        } else {
            opcion_verbose = atoi(buffer);
        }

        if (opcion_verbose == 1) {
            g_verbose = true;
            break;
        } else if (opcion_verbose == 2) {
            g_verbose = false;
            break;
        } else {
            printf("Error: Opción no válida. Por favor, ingrese 1 o 2.\n");
        }
    } while (true);
    
    printf("\nParámetros actualizados.\n");
}

void mostrar_menu(ParametrosSimulacion* params) {
    // Declaración de la función que inicia la simulación (previamente definida en simulacion.c)
    
    int opcion;
    char buffer[10];
    do {
        printf("\n========= MENÚ CENTRAL TELEFÓNICA =========\n");
        printf("1. Iniciar simulacion\n");
        printf("2. Modificar parametros\n");
        printf("3. Ver consumo de recursos (Linux)\n");
        printf("4. Salir\n");
        printf("-------------------------------------------\n");
        printf("Parametros: %d Tel, %dH Sim, T1=%ds, T2=%ds | Modo Detallado: %s\n", 
               params->num_telefonos, params->duracion_simulacion/3600, 
               params->timeout1_sin_marcar, params->timeout2_sin_respuesta,
               g_verbose ? "Activado" : "Desactivado");
        printf("Seleccione una opcion: ");

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
            default: printf("Opcion no valida.\n"); break;
        }
    } while (opcion != 4);
}