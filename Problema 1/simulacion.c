#include "simulacion.h"
#include "utilidades.h"

#define SIMULATION_SPEEDUP_FACTOR 3600

double diferencia_tiempo_seg(struct timespec t_fin, struct timespec t_ini) {
    return (t_fin.tv_sec - t_ini.tv_sec) + (t_fin.tv_nsec - t_ini.tv_nsec) / 1.0e9;
}

// Variables globales externas
extern CentralTelefonica g_central;
extern Estadisticas g_estadisticas;
extern volatile bool g_simulacion_activa;
extern bool g_verbose;

// Funcion para mostrar el estado de un telefono
void mostrar_estado(Telefono* telefono, const char* mensaje_extra) {
    const char* estado_str = estado_a_texto(telefono->estado);
    if (mensaje_extra) {
         printf("[Telefono %3d]: Estado: %-30s | %s\n",
               telefono->id, estado_str, mensaje_extra);
    } else {
         printf("[Telefono %3d]: Estado: %s\n",
               telefono->id, estado_str);
    }
}


//  Hilo que simula las acciones de un usuario sobre un telefono.
//  Su Unica responsabilidad es cambiar el estado LOCAL del telefono
//  como si un usuario estuviera interactuando con él.
 
void* hilo_telefono(void* args) {
    ArgsHilo* data = (ArgsHilo*)args;
    Telefono* self = data->central->telefonos[data->id];
    srand(time(NULL) ^ pthread_self());

    while (*(data->simulacion_activa)) {
        usleep((1000 + rand() % 2000) * 1000 / SIMULATION_SPEEDUP_FACTOR); // Simula tiempo entre acciones

        sem_wait(&self->lock);
        
        // Lógica de acción del usuario basada en el estado actual
        switch (self->estado) {
            case COLGADO:
                if (rand() % 100 < 5) { // Probabilidad de descolgar
                    self->estado = DESCOLGADO_ESPERANDO_MARCAR;
                    clock_gettime(CLOCK_MONOTONIC, &self->tiempo_ultimo_evento);
                    mostrar_estado(self, "Usuario ha descolgado.");
                }
                break;
            case DESCOLGADO_ESPERANDO_MARCAR:
                if (rand() % 100 < 50) { // Probabilidad de marcar
                    int num_a_marcar;
                    do {
                        num_a_marcar = rand() % data->params->num_telefonos;
                    } while (num_a_marcar == self->id);
                    
                    self->estado = MARCANDO; // Pone el teléfono en modo "quiero marcar"
                    self->numero_marcado = num_a_marcar;
                    clock_gettime(CLOCK_MONOTONIC, &self->tiempo_ultimo_evento);
                    char msg[100];
                    sprintf(msg, "Usuario marca el n�mero %d.", num_a_marcar);
                    mostrar_estado(self, msg);
                }
                break;
            case RECIBIENDO_LLAMADA:
                if (rand() % 100 < 60) { // Probabilidad de contestar
                    self->estado = EN_LLAMADA;
                    clock_gettime(CLOCK_MONOTONIC, &self->tiempo_ultimo_evento);
                    mostrar_estado(self, "Usuario atiende la llamada.");
                }
                break;
            case EN_LLAMADA:
                if (rand() % 100 < 10) {
                    self->estado = COLGADO;
                    clock_gettime(CLOCK_MONOTONIC, &self->tiempo_ultimo_evento);
                    mostrar_estado(self, "Usuario cuelga la llamada.");
                }
                break;
            case COMUNICANDO:
                // Si hay un tono de error, el usuario eventualmente cuelga
                self->estado = COLGADO;
                clock_gettime(CLOCK_MONOTONIC, &self->tiempo_ultimo_evento);
                mostrar_estado(self, "Colgando tras tono de error.");
                break;
            default:
                break;
        }
        sem_post(&self->lock);
    }
    free(data);
    return NULL;
}


/**
 Hilo que simula la CENTRAL TELEFÓNICA.
 Es el único responsable de la lógica de conexión y de los timeouts.
 Itera sobre todos los teléfonos y gestiona las transiciones de estado de la red.
 */
void* hilo_central(void* args) {
    ArgsHilo* data = (ArgsHilo*)args;
    CentralTelefonica* central = data->central;
    ParametrosSimulacion* params = data->params;

    printf("[Central]: Central telefonica operativa.\n");

    while (*(data->simulacion_activa)) {
        // Itera sobre todos los teléfonos para revisar sus estados
        for (int i = 0; i < params->num_telefonos; ++i) {
            struct timespec ahora;
            clock_gettime(CLOCK_MONOTONIC, &ahora);
            Telefono* origen = central->telefonos[i];
            
            // Usamos trylock para no bloquearnos si un hilo de teléfono
            // está actualizando. Si no podemos, lo intentamos en el siguiente ciclo.
            if (sem_trywait(&origen->lock) != 0) {
                continue; // No se pudo bloquear, pasar al siguiente teléfono
            }

            double tiempo_real_transcurrido = diferencia_tiempo_seg(ahora, origen->tiempo_ultimo_evento);
            double tiempo_simulado_transcurrido = tiempo_real_transcurrido * SIMULATION_SPEEDUP_FACTOR;

            switch (origen->estado) {
                case MARCANDO: {
                    int id_destino = origen->numero_marcado;
                    Telefono* destino = central->telefonos[id_destino];
                    
                    // Estrategia anti-deadlock: bloquear semaforos en orden de ID
                    // Se necesita bloquear el telofono de destino, pero para evitar deadlocks,
                    // liberamos el origen y bloqueamos ambos en orden canonico.
                    sem_post(&origen->lock);

                    sem_t* sem1 = (origen->id < id_destino) ? &origen->lock : &destino->lock;
                    sem_t* sem2 = (origen->id < id_destino) ? &destino->lock : &origen->lock;
                    
                    sem_wait(sem1);
                    sem_wait(sem2);
                    
                    // Solo proceder si el estado del origen sigue siendo MARCANDO
                    if (origen->estado == MARCANDO) {
                        if (destino->estado == COLGADO) {
                            destino->estado = RECIBIENDO_LLAMADA;
                            destino->conectado_con = origen->id;
                            clock_gettime(CLOCK_MONOTONIC, &destino->tiempo_ultimo_evento);
                            mostrar_estado(destino, "Llamada entrante.");
                            sem_wait(&g_estadisticas.lock);
                            g_estadisticas.llamadas_efectivas++;
                            sem_post(&g_estadisticas.lock);

                            origen->estado = LLAMANDO;
                            origen->conectado_con = id_destino;
                            clock_gettime(CLOCK_MONOTONIC, &origen->tiempo_ultimo_evento);
                            mostrar_estado(origen, NULL);
                        } else {
                            origen->estado = COMUNICANDO; // El destino esta ocupado
                            clock_gettime(CLOCK_MONOTONIC, &origen->tiempo_ultimo_evento);
                            mostrar_estado(origen, "Destino ocupado.");
                            sem_wait(&g_estadisticas.lock);
                            g_estadisticas.llamadas_ocupado++;
                            sem_post(&g_estadisticas.lock);
                        }
                    }
                    sem_post(sem1);
                    sem_post(sem2);
                    break;
                }

                case DESCOLGADO_ESPERANDO_MARCAR:
                    if (tiempo_simulado_transcurrido > params->timeout1_sin_marcar) {
                        origen->estado = COMUNICANDO;
                        clock_gettime(CLOCK_MONOTONIC, &origen->tiempo_ultimo_evento);
                        mostrar_estado(origen, "Timeout T1: No se marco a tiempo.");
                        sem_wait(&g_estadisticas.lock);
                        g_estadisticas.llamadas_perdidas_timeout++;
                        sem_post(&g_estadisticas.lock);
                    }
                    sem_post(&origen->lock);
                    break;
                
                case LLAMANDO: {
                    int id_destino = origen->conectado_con;
                    if (id_destino == -1) { // Chequeo de seguridad
                        sem_post(&origen->lock);
                    break;
                    }
    
                    Telefono* destino = central->telefonos[id_destino];
                    sem_wait(&destino->lock); // Bloquear destino para ver el estado
    
                    if (tiempo_simulado_transcurrido > params->timeout2_sin_respuesta) {
        
                        origen->estado = COMUNICANDO;
                        origen->conectado_con = -1;
                        clock_gettime(CLOCK_MONOTONIC, &origen->tiempo_ultimo_evento);
                        mostrar_estado(origen, "Timeout T2: El destino no contesta.");
        
                        // Limpiar el estado del destino si estaba sonando
                        if (destino->estado == RECIBIENDO_LLAMADA && destino->conectado_con == origen->id) {
                            destino->estado = COLGADO;
                            destino->conectado_con = -1;
                            mostrar_estado(destino, "Llamada perdida (origen cancelo).");
                        }
        
                        sem_wait(&g_estadisticas.lock);
                        g_estadisticas.llamadas_perdidas_timeout++;
                        sem_post(&g_estadisticas.lock);

                    } else if (destino->estado == EN_LLAMADA && destino->conectado_con == origen->id) {
        
                        double tiempo_espera = diferencia_tiempo_seg(ahora, origen->tiempo_ultimo_evento);
        
                        sem_wait(&g_estadisticas.lock);
                        g_estadisticas.llamadas_efectivas++; 
        
                        g_estadisticas.total_tiempo_espera_conexion += tiempo_espera;
                        if (tiempo_espera > g_estadisticas.tiempo_max_espera_conexion) {
                            g_estadisticas.tiempo_max_espera_conexion = tiempo_espera;
                        }
                        sem_post(&g_estadisticas.lock);
        
                        origen->estado = EN_LLAMADA;
                        origen->tiempo_ultimo_evento = ahora;
                        char msg[100];
                        sprintf(msg, "Conectado con %d.", id_destino);
                        mostrar_estado(origen, msg);
        
                    } 
                    // Si no se cumple ninguna de las dos condiciones, el teléfono sigue en estado LLAMANDO.

                    sem_post(&destino->lock);
                    sem_post(&origen->lock);
                    break;
                }
                case EN_LLAMADA: {
                    // Lógica para detectar si el otro extremo ha colgado
                    int id_otro = origen->conectado_con;
                     if (id_otro == -1) { // Chequeo de seguridad
                        sem_post(&origen->lock);
                        break;
                    }
                    Telefono* otro = central->telefonos[id_otro];
                    sem_post(&origen->lock); // Liberar origen para bloquear en orden
                    
                    // LÓGICA ANTI-DEADLOCK 
                    sem_t* sem1 = (origen->id < id_otro) ? &origen->lock : &otro->lock;
                    sem_t* sem2 = (origen->id < id_otro) ? &otro->lock : &origen->lock;
                    sem_wait(sem1);
                    sem_wait(sem2);

                    // Si el otro ha colgado, el estado de origen pasa a COMUNICANDO
                    if (otro->estado == COLGADO && origen->estado == EN_LLAMADA) { 
                        
                        double duracion = diferencia_tiempo_seg(ahora, origen->tiempo_ultimo_evento);
                        
                        sem_wait(&g_estadisticas.lock);
                        g_estadisticas.tiempo_total_conversacion += duracion;
                        if (duracion > g_estadisticas.max_duracion_conversacion) {
                            g_estadisticas.max_duracion_conversacion = duracion;
                        }
                        sem_post(&g_estadisticas.lock);
                        
                        origen->estado = COMUNICANDO;
                        origen->conectado_con = -1;
                        clock_gettime(CLOCK_MONOTONIC, &origen->tiempo_ultimo_evento);
                        mostrar_estado(origen, "El otro extremo ha colgado.");
                    }
                    sem_post(sem1);
                    sem_post(sem2);
                    break;
                }
                
                default:
                    // Si no es un estado que la central deba gestionar, simplemente libera el lock.
                    sem_post(&origen->lock);
                    break;
            }
        }
        usleep(100000 / SIMULATION_SPEEDUP_FACTOR); // Espera de 100ms simulados (reducida en tiempo real)
    }
    
    printf("[Central]: Central telefonica cerrando operaciones.\n");
    free(data);
    return NULL;
}


void iniciar_simulacion(ParametrosSimulacion* params) {
    sem_wait(&g_estadisticas.lock);
    memset(&g_estadisticas, 0, sizeof(Estadisticas));
    sem_post(&g_estadisticas.lock);

    for (int i = 0; i < params->num_telefonos; ++i) {
        Telefono* tel = g_central.telefonos[i];
        sem_wait(&tel->lock);
        tel->estado = COLGADO;
        tel->conectado_con = -1;
        tel->numero_marcado = -1;
        clock_gettime(CLOCK_MONOTONIC, &tel->tiempo_ultimo_evento);
        sem_post(&tel->lock);
    }

    printf("\n\n========================================\n");
    printf("         INICIANDO SIMULACION\n");
    printf("========================================\n");
    printf("  Telefonos: %d | Duracion: %d segundos\n", params->num_telefonos, params->duracion_simulacion);
    printf("========================================\n\n");

    g_simulacion_activa = true;

    // Crear el hilo de la Central
    pthread_t hilo_id_central;
    ArgsHilo* args_central = (ArgsHilo*)malloc(sizeof(ArgsHilo));
    args_central->id = -1;
    args_central->central = &g_central;
    args_central->params = params;
    args_central->simulacion_activa = &g_simulacion_activa;
    pthread_create(&hilo_id_central, NULL, hilo_central, args_central);

    // Crear los hilos de los Teléfonos
    pthread_t hilos_telefonos[params->num_telefonos];
    for (int i = 0; i < params->num_telefonos; ++i) {
        ArgsHilo* args_tel = (ArgsHilo*)malloc(sizeof(ArgsHilo));
        args_tel->id = i;
        args_tel->central = &g_central;
        args_tel->params = params;
        args_tel->simulacion_activa = &g_simulacion_activa;
        pthread_create(&hilos_telefonos[i], NULL, hilo_telefono, args_tel);
    }

    // Duración REAL de la espera
    int duracion_real = params->duracion_simulacion / SIMULATION_SPEEDUP_FACTOR;
    if (duracion_real == 0){
        duracion_real = 1; // Mínimo de 1 segundo
    }
    sleep(duracion_real);

    printf("\n--- TIEMPO DE SIMULACION FINALIZADO ---\nIniciando detencion controlada...\n");
    g_simulacion_activa = false; // Señaliza a todos los hilos que deben terminar
    
    // CIERRE CONTROLADO
    // Ya no se fuerzan los semáforos. Simplemente esperamos a que los hilos
    // terminen su ciclo al ver la bandera g_simulacion_activa en false.
    
    pthread_join(hilo_id_central, NULL);
    for (int i = 0; i < params->num_telefonos; ++i) {
        pthread_join(hilos_telefonos[i], NULL);
    }
    
    printf("\nTodos los hilos han terminado. Volviendo al menu...\n");
    
    printf("\n\n================= ESTADISTICAS FINALES =================\n");
    
    int total_perdidas = g_estadisticas.llamadas_perdidas_timeout + g_estadisticas.llamadas_ocupado;
    int total_intentadas = g_estadisticas.llamadas_efectivas + total_perdidas;

    // CÁLCULOS DE MÉTRICAS ADICIONALES
    double tiempo_prom_espera_conexion = (g_estadisticas.llamadas_efectivas > 0) ? 
                                         g_estadisticas.total_tiempo_espera_conexion * SIMULATION_SPEEDUP_FACTOR / g_estadisticas.llamadas_efectivas : 0.0;
    
    // Intensidad de Tráfico (Erlangs) = Tiempo Total de Conversación / Duración de Simulación
    double erlang_traffic = g_estadisticas.tiempo_total_conversacion * SIMULATION_SPEEDUP_FACTOR / (double)params->duracion_simulacion;


    printf("\n--- Metricas Generales ---\n");
    printf("Tiempo de simulacion real (segundos): %d\n", params->duracion_simulacion/3600);

    printf("\n--- Resultados de Llamadas ---\n");
    printf("Llamadas efectivas (comunicacion lograda): %d\n", g_estadisticas.llamadas_efectivas);
    printf("Llamadas perdidas (total): %d\n",total_perdidas);
    printf("  Por timeout (T1 o T2): %d\n", g_estadisticas.llamadas_perdidas_timeout);
    printf("  Por numero ocupado (Tuu-Tuu-Tuu): %d\n", g_estadisticas.llamadas_ocupado);

    double porc_exito = ((float)g_estadisticas.llamadas_efectivas / total_intentadas) * 100.0;
    printf("Porcentaje de exito (efectivas/intentadas): %.2f%%\n", porc_exito);
    
    printf("\n--- Tiempos y Trafico ---\n");
    
    // NUEVA ESTADISTICA: T2 Promedio
    printf("Tiempo maximo de espera para la conexion (T2 max): %.2f segundos\n", g_estadisticas.tiempo_max_espera_conexion * SIMULATION_SPEEDUP_FACTOR);
    printf("Tiempo promedio de espera para la conexion (T2 prom): %.2f segundos\n", tiempo_prom_espera_conexion);
    
    printf("Tiempo total de conversacion: %.2f segundos\n", g_estadisticas.tiempo_total_conversacion * SIMULATION_SPEEDUP_FACTOR);
    
    // NUEVA ESTADISTICA: Duracion Maxima
    printf("Duracion maxima de una sola conversacion: %.2f segundos\n", g_estadisticas.max_duracion_conversacion * SIMULATION_SPEEDUP_FACTOR);

    if (g_estadisticas.llamadas_efectivas > 0) {
        printf("Tiempo promedio de conversacion: %.2f segundos\n", g_estadisticas.tiempo_total_conversacion / g_estadisticas.llamadas_efectivas * SIMULATION_SPEEDUP_FACTOR);
    } else {
        printf("Tiempo promedio de conversacion: N/A\n");
    }

    // NUEVA ESTADISTICA: Erlangs
    printf("\nIntensidad de Trafico (Erlangs): %.4f\n", erlang_traffic);


    printf("==========================================================================\n");
}
