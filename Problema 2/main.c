#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "estructuras.h"

// --- Definición de Variables ---
sem_t sem_tramo[4];
sem_t sem_peso_tramo2;
pthread_mutex_t mutex_print;
pthread_mutex_t mutex_tramo2_entry;
Estadisticas stats;

// --- Implementación de Funciones ---

// Inicialización de recursos a utilizar
void inicializar_recursos() {
    sem_init(&sem_tramo[0], 0, 4);
    sem_init(&sem_tramo[1], 0, 2);
    sem_init(&sem_tramo[2], 0, 1);
    sem_init(&sem_tramo[3], 0, 3);
    sem_init(&sem_peso_tramo2, 0, 2);

    pthread_mutex_init(&mutex_print, NULL);
    pthread_mutex_init(&stats.mutex_stats, NULL);
    pthread_mutex_init(&mutex_tramo2_entry, NULL);

    srand(time(NULL));
    resetear_stats_hora();
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 2; j++) {
            stats.vehiculos_por_tramo_dia[i][j] = 0;
            stats.max_vehiculos_esperando[i][j] = 0;
            stats.vehiculos_esperando_actual[i][j] = 0;
            stats.sum_tiempo[i][j] = 0.0;
            stats.max_tiempo_espera[i][j] = 0.0;
        }
    }
}

void destruir_recursos() {
    for (int i = 0; i < 4; i++) sem_destroy(&sem_tramo[i]);
    sem_destroy(&sem_peso_tramo2);
    pthread_mutex_destroy(&mutex_print);
    pthread_mutex_destroy(&stats.mutex_stats);
    pthread_mutex_destroy(&mutex_tramo2_entry);
}

// Reseteo de las estadísticas
void resetear_stats_hora() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 2; j++) {
            stats.vehiculos_por_tramo_hora[i][j] = 0;
        }
    }
}

// Impresión del reporte por hora
void imprimir_un_reporte_hora(int hora, const ReporteHora* reporte) {
    printf("\n\n======================================================\n");
    printf("||              REPORTE DE LA HORA %02d                ||\n", hora);
    printf("======================================================\n");
    printf("TRAMO | SENTIDO (1->4) | SENTIDO (4->1)\n");
    printf("------------------------------------------------------\n");
    for (int i = 0; i < 4; i++) {
        printf("  %d   | %14d | %14d\n", i + 1, reporte->vehiculos_por_tramo[i][SENTIDO_1_A_4], reporte->vehiculos_por_tramo[i][SENTIDO_4_A_1]);
    }
    printf("======================================================\n");
}

// Impresión del resumen diario (Con las horas calculadas)
void imprimir_todos_los_reportes_horarios(const ReporteHora* reportes, int duracion_horas) {
    printf("\n\n\n############################################################\n");
    printf("||                REPORTES POR HORA                   ||\n");
    printf("############################################################\n");
    for (int i = 0; i < duracion_horas; i++) {
        imprimir_un_reporte_hora(i + 1, &reportes[i]);
    }
}

// Impresión de reporte final
void imprimir_reporte_final(int vehiculos_total) {
    pthread_mutex_lock(&stats.mutex_stats);
    pthread_mutex_lock(&mutex_print);
    printf("\n\n\n############################################################\n");
    printf("||                FIN DE LA SIMULACIÓN                  ||\n");
    printf("||                    REPORTE FINAL                     ||\n");
    printf("############################################################\n\n");
    printf("--- Vehículos Totales por Tramo y Sentido ---\n");
    printf("TRAMO | SENTIDO (1->4) | SENTIDO (4->1)\n");
    printf("--------------------------------------------\n");
    for (int i = 0; i < 4; i++) {
        printf("  %d   | %14d | %14d\n", i + 1, stats.vehiculos_por_tramo_dia[i][SENTIDO_1_A_4], stats.vehiculos_por_tramo_dia[i][SENTIDO_4_A_1]);
    }
    printf("\n--- Métricas de Espera en Hombrillos ---\n");
    printf("TRAMO | MAX VEHÍCULOS (1->4) | MAX VEHÍCULOS (4->1)\n");
    printf("----------------------------------------------------\n");
    for (int i = 0; i < 4; i++) {
        printf("  %d   | %20d | %20d\n", i + 1, stats.max_vehiculos_esperando[i][SENTIDO_1_A_4], stats.max_vehiculos_esperando[i][SENTIDO_4_A_1]);
    }

    printf("----------------------------------------------------\n");

    printf("TRAMO | TIEMPO MAX ESPERA (1->4) | TIEMPO MAX ESPERA (4->1)\n");
    printf("----------------------------------------------------\n");
    for (int i = 0; i < 4; i++) {
        printf("  %d   | %lf | %lf\n", i + 1, stats.max_tiempo_espera[i][SENTIDO_1_A_4], stats.max_tiempo_espera[i][SENTIDO_4_A_1]);
    }
    printf("----------------------------------------------------\n");

    printf("TRAMO | TIEMPO PROM ESPERA (1->4) | TIEMPO PROM ESPERA (4->1)\n");
    printf("----------------------------------------------------\n");
    for (int i = 0; i < 4; i++) {
        printf("  %d   | %lf | %lf\n", i + 1, stats.sum_tiempo[i][SENTIDO_1_A_4]/vehiculos_total, stats.sum_tiempo[i][SENTIDO_4_A_1]/vehiculos_total);
    }
    printf("\n############################################################\n");

    pthread_mutex_unlock(&mutex_print);
    pthread_mutex_unlock(&stats.mutex_stats);
}

// Impresión de los recursos usados por el sistema
void imprimir_recursos_sistema() {
    struct rusage usage;
    // Usamos RUSAGE_SELF para obtener los recursos del proceso actual
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        pthread_mutex_lock(&mutex_print);
        printf("\n\n############################################################\n");
        printf("||          REPORTE DE CONSUMO DE RECURSOS          ||\n");
        printf("############################################################\n\n");
        printf("Tiempo de CPU consumido:\n");
        printf("  - Tiempo de Usuario: %ld.%06ld segundos\n", usage.ru_utime.tv_sec, (long)usage.ru_utime.tv_usec);
        printf("  - Tiempo de Sistema: %ld.%06ld segundos\n", usage.ru_stime.tv_sec, (long)usage.ru_stime.tv_usec);
        printf("\nUso de Memoria:\n");
        // ru_maxrss está en kilobytes en Linux
        printf("  - Máximo de memoria residente (RSS): %ld KB\n", usage.ru_maxrss);
        printf("\n############################################################\n");
        pthread_mutex_unlock(&mutex_print);
    } else {
        perror("Error al obtener el uso de recursos con getrusage");
    }
}

// Diseño del mundo
int mostrar_menu(SimParams *params) {
    int opcion = 0;
    printf("\n\n====== SIMULADOR DE TRÁFICO EN AUTOPISTA ======\n");
    printf("1. Ejecutar simulación con datos por defecto\n");
    printf("   (500 veh/h, 20%% camiones, 24 horas)\n");
    printf("2. Ejecutar simulación con datos personalizados\n");
    printf("3. Salir\n");
    printf("==============================================\n");
    printf("Seleccione una opción: ");

    while (scanf("%d", &opcion) != 1 || opcion < 1 || opcion > 3) {
        printf("Opción inválida. Por favor, ingrese 1, 2 o 3: ");
        // Limpiar el buffer de entrada en caso de que no sea un número
        while (getchar() != '\n');
    }
     while (getchar() != '\n'); // Limpiar el buffer de cualquier resto

    switch (opcion) {
        case 1:
            params->vehiculos_por_hora = 500;
            params->porcentaje_camiones = 20;
            params->duracion_horas = 24;
            printf("\nIniciando con parámetros por defecto...\n");
            return 1; // Continuar
        case 2:
            printf("\n--- Configuración Personalizada ---\n");
            printf("Ingrese el promedio de vehículos por hora: ");
            while (scanf("%d", &params->vehiculos_por_hora) != 1 || params->vehiculos_por_hora <= 0) {
                 printf("Entrada inválida. Ingrese un número positivo: ");
                 while (getchar() != '\n');
            }
             while (getchar() != '\n');
            
            printf("Ingrese el porcentaje de camiones (0-100): ");
            while (scanf("%d", &params->porcentaje_camiones) != 1 || params->porcentaje_camiones < 0 || params->porcentaje_camiones > 100) {
                 printf("Entrada inválida. Ingrese un número entre 0 y 100: ");
                 while (getchar() != '\n');
            }
             while (getchar() != '\n');

            printf("Ingrese la duración de la simulación en horas: ");
            while (scanf("%d", &params->duracion_horas) != 1 || params->duracion_horas <= 0) {
                 printf("Entrada inválida. Ingrese un número positivo: ");
                 while (getchar() != '\n');
            }
             while (getchar() != '\n');

            printf("\nIniciando con parámetros personalizados...\n");
            return 1; // Continuar
        case 3:
            printf("Saliendo del programa.\n");
            return 0; // Salir
    }
    return 0; // Salir por defecto
}

int main() {
    SimParams params;
    
    if (!mostrar_menu(&params)) {
        return 0; // Salir si el usuario eligió esa opción
    }
    
    int total_vehiculos = params.vehiculos_por_hora * params.duracion_horas;

    pthread_t* hilos_vehiculos = malloc(total_vehiculos * sizeof(pthread_t));
    Vehiculo* vehiculos = malloc(total_vehiculos * sizeof(Vehiculo));
    ReporteHora* reportes_por_hora = malloc(params.duracion_horas * sizeof(ReporteHora));

    if (!hilos_vehiculos || !vehiculos || !reportes_por_hora) {
        perror("Error de alocación de memoria para la simulación");
        free(hilos_vehiculos);
        free(vehiculos);
        free(reportes_por_hora);
        return 1;
    }

    inicializar_recursos();
    printf("Iniciando simulación de tráfico en la autopista para %d horas...\n", params.duracion_horas);

    int vehiculos_creados = 0;
    for (int hora = 1; hora <= params.duracion_horas; hora++) {
        int vehiculos_esta_hora = params.vehiculos_por_hora;
        long sleep_por_vehiculo = (long)(TIEMPO_SIMULACION_HORA / vehiculos_esta_hora);

        printf("... Simulando Hora %d ...\n", hora);

        for (int i = 0; i < vehiculos_esta_hora; i++) {
            if (vehiculos_creados >= total_vehiculos) break;

            vehiculos[vehiculos_creados].id = vehiculos_creados + 1;
            vehiculos[vehiculos_creados].tipo = (rand() % 100 < params.porcentaje_camiones) ? TIPO_CAMION : TIPO_CARRO;
            vehiculos[vehiculos_creados].sentido = (rand() % 2);
            vehiculos[vehiculos_creados].tiempo_inicio_espera.tv_sec = 0;
            vehiculos[vehiculos_creados].tiempo_inicio_espera.tv_nsec = 0;

            if (pthread_create(&hilos_vehiculos[vehiculos_creados], NULL, logica_vehiculo, &vehiculos[vehiculos_creados]) != 0) {
                perror("Fallo al crear el hilo");
                continue;
            }
            vehiculos_creados++;
            usleep(sleep_por_vehiculo);
        }
        
        sleep(1); // Esperamos que pase el tiempo de la hora simulada

        // Guardamos las estadísticas de la hora que acaba de pasar
        pthread_mutex_lock(&stats.mutex_stats);
        for(int t=0; t<4; t++) {
            for(int s=0; s<2; s++) {
                reportes_por_hora[hora-1].vehiculos_por_tramo[t][s] = stats.vehiculos_por_tramo_hora[t][s];
            }
        }
        resetear_stats_hora();
        pthread_mutex_unlock(&stats.mutex_stats);
    }
    
    printf("\nSimulación de creación de vehículos completada. Esperando a que todos terminen su recorrido...\n");

    for (int i = 0; i < vehiculos_creados; i++) {
        pthread_join(hilos_vehiculos[i], NULL);
    }
    
    printf("Todos los vehículos han completado su recorrido.\n");

    imprimir_todos_los_reportes_horarios(reportes_por_hora, params.duracion_horas);
    imprimir_reporte_final(total_vehiculos);
    imprimir_recursos_sistema();

    destruir_recursos();
    
    // Liberación de memoria dinámica
    free(hilos_vehiculos);
    free(vehiculos);
    free(reportes_por_hora);

    return 0;
}