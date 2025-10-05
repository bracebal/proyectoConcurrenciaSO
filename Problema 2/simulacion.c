// Nombre del archivo: simulacion.c
#include "simulacion.h"
#include "utilidades.h"

// Variables globales definidas en main.c
extern Subtramo g_subtramos[NUM_SUBTRAMOS];
extern Estadisticas g_estadisticas;
extern volatile bool g_simulacion_activa;
extern bool g_verbose;

// --- Lógica de Sincronización para Subtramos ---

void entrar_subtramo_simple(Vehiculo* v, int id_subtramo, int peso) {
    if (g_verbose) printf("[DEBUG][Veh %d] Intentando entrar a subtramo %d (peso %d)\n", v->id, id_subtramo + 1, peso);
    for (int i = 0; i < peso; ++i) {
        sem_wait(&g_subtramos[id_subtramo].sem_capacidad);
    }
}

void salir_subtramo_simple(Vehiculo* v, int id_subtramo, int peso) {
    if (g_verbose) printf("[DEBUG][Veh %d] Saliendo de subtramo %d (peso %d)\n", v->id, id_subtramo + 1, peso);
    for (int i = 0; i < peso; ++i) {
        sem_post(&g_subtramos[id_subtramo].sem_capacidad);
    }
}

void entrar_subtramo_3(Vehiculo* v) {
    Subtramo* st = &g_subtramos[2];
    sem_wait(&st->mutex); // Inicio de sección crítica

    if (st->vehiculos_dentro > 0 && st->direccion_actual != v->direccion) {
        // Si hay vehículos en dirección opuesta, esperar
        if (v->direccion == HACIA_VICTORIA) st->esperando_victoria++;
        else st->esperando_maracay++;
        sem_post(&st->mutex); // Salir de SC para esperar en la puerta
        if (g_verbose) printf("[DEBUG][Veh %d] Esperando en puerta (%s) para tramo 3\n", v->id, dir_a_texto(v->direccion));
        if (v->direccion == HACIA_VICTORIA) sem_wait(&st->puerta_victoria);
        else sem_wait(&st->puerta_maracay);
        // Al despertar, ya se nos dio paso, no necesitamos re-adquirir el mutex aquí
    } else {
        st->vehiculos_dentro++;
        st->direccion_actual = v->direccion;
        sem_post(&st->mutex); // Fin de sección crítica
    }
}

void salir_subtramo_3(Vehiculo* v) {
    Subtramo* st = &g_subtramos[2];
    sem_wait(&st->mutex); // Inicio de sección crítica
    st->vehiculos_dentro--;

    if (st->vehiculos_dentro == 0) {
        // Fui el último en salir, doy paso a la dirección opuesta si hay alguien esperando
        if (v->direccion == HACIA_VICTORIA && st->esperando_maracay > 0) {
             if (g_verbose) printf("[DEBUG][Tramo 3] Cambio de sentido a 4->1. Dando paso a %d veh.\n", st->esperando_maracay);
             int a_pasar = st->esperando_maracay;
             st->esperando_maracay = 0;
             st->vehiculos_dentro = a_pasar; // Ya contamos a los que van a pasar
             st->direccion_actual = HACIA_MARACAY;
             for(int i=0; i < a_pasar; i++) sem_post(&st->puerta_maracay);
        } else if (v->direccion == HACIA_MARACAY && st->esperando_victoria > 0) {
             if (g_verbose) printf("[DEBUG][Tramo 3] Cambio de sentido a 1->4. Dando paso a %d veh.\n", st->esperando_victoria);
             int a_pasar = st->esperando_victoria;
             st->esperando_victoria = 0;
             st->vehiculos_dentro = a_pasar;
             st->direccion_actual = HACIA_VICTORIA;
             for(int i=0; i < a_pasar; i++) sem_post(&st->puerta_victoria);
        }
    }
    sem_post(&st->mutex); // Fin de sección crítica
}


// --- Hilos de Simulación ---

void* hilo_vehiculo(void* args) {
    Vehiculo* v = (Vehiculo*)args;
    v->hilo_id = pthread_self();
    int path[NUM_SUBTRAMOS];
    int peso = (v->tipo == CAMION) ? 2 : 1;

    if (v->direccion == HACIA_VICTORIA) {
        for (int i = 0; i < NUM_SUBTRAMOS; ++i) path[i] = i;
    } else {
        for (int i = 0; i < NUM_SUBTRAMOS; ++i) path[i] = (NUM_SUBTRAMOS - 1) - i;
    }

    for (int i = 0; i < NUM_SUBTRAMOS; ++i) {
        if (!*(v->simulacion_activa)) break;

        int id_subtramo = path[i];
        
        // --- Espera y entrada al subtramo ---
        printf("[Vehículo %5d (%s, %s)]: Esperando para entrar a subtramo %d.\n", v->id, tipo_a_texto(v->tipo), dir_a_texto(v->direccion), id_subtramo + 1);

        double t_inicio_espera = tiempo_actual_preciso();
        pthread_mutex_lock(&g_estadisticas.lock);
        g_estadisticas.vehiculos_esperando_actual++;
        if (g_estadisticas.vehiculos_esperando_actual > g_estadisticas.max_vehiculos_esperando) {
            g_estadisticas.max_vehiculos_esperando = g_estadisticas.vehiculos_esperando_actual;
        }
        pthread_mutex_unlock(&g_estadisticas.lock);

        switch (id_subtramo) {
            case 0: entrar_subtramo_simple(v, 0, 1); break;
            case 1: entrar_subtramo_simple(v, 1, peso); break;
            case 2: entrar_subtramo_3(v); break;
            case 3: entrar_subtramo_simple(v, 3, 1); break;
        }
        
        double t_fin_espera = tiempo_actual_preciso();
        double tiempo_espera = t_fin_espera - t_inicio_espera;

        pthread_mutex_lock(&g_estadisticas.lock);
        g_estadisticas.vehiculos_esperando_actual--;
        if (tiempo_espera > g_estadisticas.max_tiempo_espera) {
            g_estadisticas.max_tiempo_espera = tiempo_espera;
        }
        g_estadisticas.vehiculos_por_subtramo_total[id_subtramo][v->direccion]++;
        g_estadisticas.vehiculos_por_subtramo_hora[id_subtramo][v->direccion]++;
        pthread_mutex_unlock(&g_estadisticas.lock);

        printf("[Vehículo %5d (%s, %s)]: >> Entra a subtramo %d (Esperó %.2fs).\n", v->id, tipo_a_texto(v->tipo), dir_a_texto(v->direccion), id_subtramo + 1, tiempo_espera);

        // --- Recorrido y salida ---
        usleep((500 + rand() % 500) * 1000); // Simula tiempo de recorrido

        printf("[Vehículo %5d (%s, %s)]: << Sale de subtramo %d.\n", v->id, tipo_a_texto(v->tipo), dir_a_texto(v->direccion), id_subtramo + 1);
        switch (id_subtramo) {
            case 0: salir_subtramo_simple(v, 0, 1); break;
            case 1: salir_subtramo_simple(v, 1, peso); break;
            case 2: salir_subtramo_3(v); break;
            case 3: salir_subtramo_simple(v, 3, 1); break;
        }

        if (i < NUM_SUBTRAMOS - 1) usleep((100 + rand() % 200) * 1000); // Simula hombrillo
    }

    printf("[Vehículo %5d (%s, %s)]: === Ha completado su recorrido ===\n", v->id, tipo_a_texto(v->tipo), dir_a_texto(v->direccion));
    free(v);
    return NULL;
}

void* hilo_reloj(void* args) {
    ParametrosSimulacion* params = (ParametrosSimulacion*)args;
    int hora_simulada = 0;
    // Escala de tiempo: 1 segundo real = 1 hora simulada para agilizar
    int segundos_por_hora_simulada = 1;

    while (g_simulacion_activa && hora_simulada < params->duracion_simulacion_horas) {
        sleep(segundos_por_hora_simulada);
        hora_simulada++;

        pthread_mutex_lock(&g_estadisticas.lock);
        printf("\n\n================= REPORTE HORA %d =================\n", hora_simulada);
        for (int i = 0; i < NUM_SUBTRAMOS; i++) {
            printf("Subtramo %d: %lld (%s), %lld (%s)\n", i + 1,
                   g_estadisticas.vehiculos_por_subtramo_hora[i][HACIA_VICTORIA], dir_a_texto(HACIA_VICTORIA),
                   g_estadisticas.vehiculos_por_subtramo_hora[i][HACIA_MARACAY], dir_a_texto(HACIA_MARACAY));
            // Resetear contadores horarios
            g_estadisticas.vehiculos_por_subtramo_hora[i][HACIA_VICTORIA] = 0;
            g_estadisticas.vehiculos_por_subtramo_hora[i][HACIA_MARACAY] = 0;
        }
        printf("===================================================\n\n");
        pthread_mutex_unlock(&g_estadisticas.lock);
    }
    return NULL;
}

void iniciar_simulacion(ParametrosSimulacion* params) {
    int total_vehiculos = params->vehiculos_por_hora * params->duracion_simulacion_horas;
    printf("\nIniciando simulación: ~%d vehículos en %d horas...\n", total_vehiculos, params->duracion_simulacion_horas);

    g_simulacion_activa = true;

    pthread_t* hilos = malloc(sizeof(pthread_t) * total_vehiculos);
    pthread_t hilo_id_reloj;

    pthread_create(&hilo_id_reloj, NULL, hilo_reloj, (void*)params);

    long usec_entre_vehiculos = 3600.0 * 1e6 / params->vehiculos_por_hora;

    for (int i = 0; i < total_vehiculos; i++) {
        if (!g_simulacion_activa) break;
        Vehiculo* v = malloc(sizeof(Vehiculo));
        v->id = i + 1;
        v->tipo = (rand() % 100 < params->porcentaje_camiones) ? CAMION : AUTO;
        v->direccion = (rand() % 2 == 0) ? HACIA_VICTORIA : HACIA_MARACAY;
        v->subtramos = g_subtramos;
        v->estadisticas = &g_estadisticas;
        v->simulacion_activa = &g_simulacion_activa;
        pthread_create(&hilos[i], NULL, hilo_vehiculo, v);
        usleep(usec_entre_vehiculos);
    }

    printf("\nTodos los vehículos han sido creados. Esperando que terminen...\n");
    for (int i = 0; i < total_vehiculos; i++) {
        pthread_join(hilos[i], NULL);
    }

    g_simulacion_activa = false;
    pthread_join(hilo_id_reloj, NULL);

    printf("\n\n================= ESTADÍSTICAS FINALES DEL DÍA =================");
    printf("\nVehículos por subtramo y sentido:\n");
    for (int i = 0; i < NUM_SUBTRAMOS; i++) {
        printf("  Subtramo %d: %lld (%s), %lld (%s)\n", i + 1,
               g_estadisticas.vehiculos_por_subtramo_total[i][HACIA_VICTORIA], dir_a_texto(HACIA_VICTORIA),
               g_estadisticas.vehiculos_por_subtramo_total[i][HACIA_MARACAY], dir_a_texto(HACIA_MARACAY));
    }
    printf("\n--- Métricas de Espera ---\n");
    printf("Número máximo de vehículos esperando en hombrillo (a la vez): %d\n", g_estadisticas.max_vehiculos_esperando);
    printf("Tiempo máximo de espera de un vehículo: %.2f segundos\n", g_estadisticas.max_tiempo_espera);
    printf("================================================================\n\n");

    free(hilos);
}

