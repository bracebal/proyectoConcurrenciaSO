#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include "estructuras.h"

void registrar_inicio_espera(int tramo_idx, int sentido, Vehiculo* v) {
    pthread_mutex_lock(&stats.mutex_stats);
    stats.vehiculos_esperando_actual[tramo_idx][sentido]++;
    if (stats.vehiculos_esperando_actual[tramo_idx][sentido] > stats.max_vehiculos_esperando[tramo_idx][sentido]) {
        stats.max_vehiculos_esperando[tramo_idx][sentido] = stats.vehiculos_esperando_actual[tramo_idx][sentido];
    }
    // Usar clock_gettime para alta precisión
    clock_gettime(CLOCK_MONOTONIC, &v->tiempo_inicio_espera);
    pthread_mutex_unlock(&stats.mutex_stats);
}

void registrar_fin_espera(int tramo_idx, int sentido, Vehiculo* v) {
    pthread_mutex_lock(&stats.mutex_stats);
    stats.vehiculos_esperando_actual[tramo_idx][sentido]--;
    if (v->tiempo_inicio_espera.tv_sec != 0) {
        struct timespec fin_espera;
        clock_gettime(CLOCK_MONOTONIC, &fin_espera);
        double tiempo_esperado = (fin_espera.tv_sec - v->tiempo_inicio_espera.tv_sec);
        tiempo_esperado += (fin_espera.tv_nsec - v->tiempo_inicio_espera.tv_nsec) / 1000000000.0;

        stats.sum_tiempo[tramo_idx][sentido] += tiempo_esperado;
        
        if (tiempo_esperado > stats.max_tiempo_espera[tramo_idx][sentido]) {
            stats.max_tiempo_espera[tramo_idx][sentido] = tiempo_esperado;
        }
    }
    pthread_mutex_unlock(&stats.mutex_stats);
}

void entrar_tramo(Vehiculo* v, int num_tramo) {
    int tramo_idx = num_tramo - 1;
    
    pthread_mutex_lock(&mutex_print);
    printf("Vehículo %d (%s) llegando al tramo %d...\n", v->id, v->tipo == TIPO_CAMION ? "Camión" : "Carro", num_tramo);
    pthread_mutex_unlock(&mutex_print);

    registrar_inicio_espera(tramo_idx, v->sentido, v);

    if (num_tramo == 2) {
        int peso = (v->tipo == TIPO_CAMION) ? 2 : 1;
        pthread_mutex_lock(&mutex_tramo2_entry);
        for (int i = 0; i < peso; ++i) {
            sem_wait(&sem_peso_tramo2);
        }
        pthread_mutex_unlock(&mutex_tramo2_entry);
    } else {
        sem_wait(&sem_tramo[tramo_idx]);
    }
    
    registrar_fin_espera(tramo_idx, v->sentido, v);
    
    pthread_mutex_lock(&mutex_print);
    printf("Vehículo %d ENTRA al tramo %d.\n", v->id, num_tramo);
    pthread_mutex_unlock(&mutex_print);
    
    pthread_mutex_lock(&stats.mutex_stats);
    stats.vehiculos_por_tramo_hora[tramo_idx][v->sentido]++;
    stats.vehiculos_por_tramo_dia[tramo_idx][v->sentido]++;
    pthread_mutex_unlock(&stats.mutex_stats);
}

void salir_tramo(Vehiculo* v, int num_tramo) {
    int tramo_idx = num_tramo - 1;
    if (num_tramo == 2) {
        int peso = (v->tipo == TIPO_CAMION) ? 2 : 1;
        for (int i = 0; i < peso; ++i) {
            sem_post(&sem_peso_tramo2);
        }
    } else {
        sem_post(&sem_tramo[tramo_idx]);
    }
    
    pthread_mutex_lock(&mutex_print);
    printf("Vehículo %d SALE del tramo %d.\n", v->id, num_tramo);
    pthread_mutex_unlock(&mutex_print);
}

void* logica_vehiculo(void* arg) {
    Vehiculo* v = (Vehiculo*)arg;
    
    int tramos[4];
    if (v->sentido == SENTIDO_1_A_4) {
        tramos[0] = 1; tramos[1] = 2; tramos[2] = 3; tramos[3] = 4;
    } else {
        tramos[0] = 4; tramos[1] = 3; tramos[2] = 2; tramos[3] = 1;
    }
    
    pthread_mutex_lock(&mutex_print);
    printf("==> Inicia recorrido Vehículo %d (%s) en sentido %d -> %d\n", v->id, v->tipo == TIPO_CAMION ? "Camión" : "Carro", tramos[0], tramos[3]);
    pthread_mutex_unlock(&mutex_print);

    for (int i = 0; i < 4; i++) {
        entrar_tramo(v, tramos[i]);
        usleep(TIEMPO_EN_TRAMO_MIN + rand() % (TIEMPO_EN_TRAMO_MAX - TIEMPO_EN_TRAMO_MIN + 1));
        salir_tramo(v, tramos[i]);
    }
    
    pthread_mutex_lock(&mutex_print);
    printf("<== Fin de recorrido Vehículo %d\n", v->id);
    pthread_mutex_unlock(&mutex_print);

    return NULL;
}
