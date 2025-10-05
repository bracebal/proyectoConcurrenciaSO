#include "simulacion.h"
#include "utilidades.h"

// Variables
extern CentralTelefonica g_central;
extern Estadisticas g_estadisticas;
extern volatile bool g_simulacion_activa;
extern bool g_verbose;

// Función para mostrar el estado de un teléfono
void mostrar_estado(Telefono* telefono, const char* mensaje_extra) {
    const char* estado_str = estado_a_texto(telefono->estado);
    if (mensaje_extra) {
         printf("[Teléfono %3d][Hilo %lu]: Estado: %-30s | %s\n",
               telefono->id, pthread_self(), estado_str, mensaje_extra);
    } else {
         printf("[Teléfono %3d][Hilo %lu]: Estado: %s\n",
               telefono->id, pthread_self(), estado_str);
    }
}


void* hilo_telefono(void* args) {
    ArgsHilo* data = (ArgsHilo*)args;
    Telefono* self = data->central->telefonos[data->id];
    srand(time(NULL) ^ pthread_self());

    while (*(data->simulacion_activa)) {
        usleep((1000 + rand() % 2000) * 1000); // Simula el tiempo entre acciones del usuario

        if (g_verbose) printf("[DEBUG][Tel %d] Intenta bloquear semáforo.\n", self->id);
        sem_wait(&self->lock);
        if (g_verbose) printf("[DEBUG][Tel %d] Semáforo bloqueado.\n", self->id);
        
        // Lógica de acción del usuario basada en el estado actual
        switch (self->estado) {
            case COLGADO:
                if (rand() % 20 < 2) { // Probabilidad de descolgar
                    self->estado = DESCOLGADO_ESPERANDO_MARCAR;
                    self->tiempo_ultimo_evento = time(NULL);
                    mostrar_estado(self, "Usuario ha descolgado.");
                }
                break;
            case DESCOLGADO_ESPERANDO_MARCAR:
                {
                    int num_a_marcar;
                    do {
                        num_a_marcar = rand() % data->params->num_telefonos;
                    } while (num_a_marcar == self->id);
                    self->estado = MARCANDO;
                    self->numero_marcado = num_a_marcar;
                    self->tiempo_ultimo_evento = time(NULL);
                    char msg[100];
                    sprintf(msg, "Usuario marca el número %d.", num_a_marcar);
                    mostrar_estado(self, msg);
                }
                break;
            case RECIBIENDO_LLAMADA:
                self->estado = EN_LLAMADA;
                self->tiempo_ultimo_evento = time(NULL);
                mostrar_estado(self, "Usuario atiende la llamada.");
                break;
            case EN_LLAMADA:
                if (rand() % 10 < 3) { // Probabilidad de colgar
                    self->estado = COLGADO;
                    self->tiempo_ultimo_evento = time(NULL);
                    mostrar_estado(self, "Usuario cuelga la llamada.");
                }
                break;
            case COMUNICANDO:
            case TIMEOUT:
                self->estado = COLGADO;
                self->tiempo_ultimo_evento = time(NULL);
                mostrar_estado(self, "Colgando tras tono de error.");
                break;
            default:
                break;
        }

        if (g_verbose) printf("[DEBUG][Tel %d] Libera semáforo.\n", self->id);
        sem_post(&self->lock);
    }
    free(data);
    return NULL;
}


void* hilo_central(void* args) {
    ArgsHilo* data = (ArgsHilo*)args;
    CentralTelefonica* central = data->central;
    ParametrosSimulacion* params = data->params;

    printf("[Central][Hilo %lu]: Central telefónica operativa.\n", pthread_self());

    while (*(data->simulacion_activa)) {
        for (int i = 0; i < params->num_telefonos; ++i) {
            Telefono* origen = central->telefonos[i];

            if (g_verbose) printf("[DEBUG][Central] Intenta bloquear semáforo Tel %d.\n", origen->id);
            sem_wait(&origen->lock);
            if (g_verbose) printf("[DEBUG][Central] Semáforo Tel %d bloqueado.\n", origen->id);

            time_t ahora = time(NULL);
            double tiempo_transcurrido = difftime(ahora, origen->tiempo_ultimo_evento);

            switch (origen->estado) {
                case MARCANDO: {
                    int id_destino = origen->numero_marcado;
                    Telefono* destino = central->telefonos[id_destino];
                    
                    printf("[Central][Info]: Tel %d intenta conectar con %d.\n", origen->id, id_destino);

                    // --- Estrategia anti-deadlock: bloquear semáforos en orden de ID ---
                    sem_t* sem1 = (origen->id < id_destino) ? &origen->lock : &destino->lock;
                    sem_t* sem2 = (origen->id < id_destino) ? &destino->lock : &origen->lock;
                    
                    // Ya tenemos el semáforo de origen, ahora necesitamos el de destino
                    // Liberamos 'origen' temporalmente para adquirir ambos en el orden correcto
                    sem_post(&origen->lock);
                    if (g_verbose) printf("[DEBUG][Central] Semáforo Tel %d liberado temporalmente.\n", origen->id);

                    if (g_verbose) printf("[DEBUG][Central] Adquiriendo semáforos en orden: %d, %d.\n", (sem1 == &origen->lock ? origen->id : id_destino), (sem2 == &origen->lock ? origen->id : id_destino));
                    sem_wait(sem1);
                    sem_wait(sem2);
                    if (g_verbose) printf("[DEBUG][Central] Ambos semáforos bloqueados.\n");
                    
                    if (destino->estado == COLGADO) {
                        destino->estado = RECIBIENDO_LLAMADA;
                        destino->conectado_con = origen->id;
                        destino->tiempo_ultimo_evento = ahora;
                        mostrar_estado(destino, NULL);

                        origen->estado = LLAMANDO;
                        origen->conectado_con = id_destino;
                        origen->tiempo_ultimo_evento = ahora;
                    } else {
                        origen->estado = COMUNICANDO;
                        origen->tiempo_ultimo_evento = ahora;
                        
                        sem_wait(&g_estadisticas.lock);
                        g_estadisticas.llamadas_ocupado++;
                        sem_post(&g_estadisticas.lock);
                    }
                    
                    mostrar_estado(origen, NULL);

                    sem_post(sem1);
                    sem_post(sem2);
                    if (g_verbose) printf("[DEBUG][Central] Ambos semáforos liberados.\n");

                    goto siguiente_telefono; // Se mueve la liberación al final del bucle
                }

                case DESCOLGADO_ESPERANDO_MARCAR:
                    if (tiempo_transcurrido > params->timeout1_sin_marcar) {
                        origen->estado = TIMEOUT;
                        origen->tiempo_ultimo_evento = ahora;
                        mostrar_estado(origen, "Timeout 1: No se marcó a tiempo.");
                        
                        sem_wait(&g_estadisticas.lock);
                        g_estadisticas.llamadas_perdidas_timeout++;
                        sem_post(&g_estadisticas.lock);
                    }
                    break;
                
                case LLAMANDO:
                    {
                        if (tiempo_transcurrido > params->timeout2_sin_respuesta) {
                            origen->estado = TIMEOUT;
                            origen->tiempo_ultimo_evento = ahora;
                            mostrar_estado(origen, "Timeout 2: El destino no contesta.");
                             sem_wait(&g_estadisticas.lock);
                             g_estadisticas.llamadas_perdidas_timeout++;
                             sem_post(&g_estadisticas.lock);

                        } else {
                            int id_destino = origen->conectado_con;
                            Telefono* destino = central->telefonos[id_destino];
                            sem_wait(&destino->lock); // Bloquear destino para ver si ya contestó
                            
                            if(destino->estado == EN_LLAMADA){
                                origen->estado = EN_LLAMADA;
                                origen->tiempo_ultimo_evento = ahora;
                                
                                sem_wait(&g_estadisticas.lock);
                                g_estadisticas.llamadas_efectivas++;
                                if (tiempo_transcurrido > g_estadisticas.tiempo_max_espera_conexion) {
                                    g_estadisticas.tiempo_max_espera_conexion = tiempo_transcurrido;
                                }
                                sem_post(&g_estadisticas.lock);

                                char msg[100];
                                sprintf(msg, "Conectado con %d.", id_destino);
                                mostrar_estado(origen, msg);
                            }
                            sem_post(&destino->lock);
                        }
                    }
                    break;
                default:
                    break;
            }
            if (g_verbose) printf("[DEBUG][Central] Libera semáforo Tel %d.\n", origen->id);
            sem_post(&origen->lock);

            siguiente_telefono:; // Etiqueta
        }
        usleep(100000); // Evita sobrecargar el CPU
    }
    
    printf("[Central][Hilo %lu]: Central telefónica cerrando operaciones.\n", pthread_self());
    free(data);
    return NULL;
}


void iniciar_simulacion(ParametrosSimulacion* params) {
    printf("\nIniciando simulación por %d segundos...\n", params->duracion_simulacion);

    g_simulacion_activa = true;
    
    // Constructor de hilos
    pthread_t hilo_id_central;
    pthread_t* hilos_telefonos = malloc(sizeof(pthread_t) * params->num_telefonos);
    
    ArgsHilo* args_central = malloc(sizeof(ArgsHilo));
    args_central->id = -1;
    args_central->central = &g_central;
    args_central->estadisticas = &g_estadisticas;
    args_central->params = params;
    args_central->simulacion_activa = &g_simulacion_activa;
    pthread_create(&hilo_id_central, NULL, hilo_central, args_central);
    
    for (int i = 0; i < params->num_telefonos; ++i) {
        ArgsHilo* args_tel = malloc(sizeof(ArgsHilo));
        args_tel->id = i;
        args_tel->central = &g_central;
        args_tel->estadisticas = &g_estadisticas;
        args_tel->params = params;
        args_tel->simulacion_activa = &g_simulacion_activa;
        pthread_create(&hilos_telefonos[i], NULL, hilo_telefono, args_tel);
    }

    // Esperar a que termine la simulación
    sleep(params->duracion_simulacion);
    
    printf("\n--- TIEMPO DE SIMULACIÓN FINALIZADO ---\nDeteniendo hilos...\n");
    g_simulacion_activa = false;
    
    pthread_join(hilo_id_central, NULL);
    for (int i = 0; i < params->num_telefonos; ++i) {
        pthread_join(hilos_telefonos[i], NULL);
    }
    printf("Todos los hilos han terminado.\n");
    
    // Mostrar estadísticas
    printf("\n\n================= ESTADÍSTICAS FINALES (proyectado a 24h) =================");
    double factor = 86400.0 / params->duracion_simulacion;
    printf("\nLlamadas efectivas: %d (%.0f)\n", g_estadisticas.llamadas_efectivas, g_estadisticas.llamadas_efectivas * factor);
    printf("Llamadas perdidas (timeout): %d (%.0f)\n", g_estadisticas.llamadas_perdidas_timeout, g_estadisticas.llamadas_perdidas_timeout * factor);
    printf("Llamadas perdidas (ocupado): %d (%.0f)\n", g_estadisticas.llamadas_ocupado, g_estadisticas.llamadas_ocupado * factor);
    printf("Tiempo máximo de espera para conexión: %.2f segundos\n", g_estadisticas.tiempo_max_espera_conexion);
    printf("=========================================================================\n\n");
    
    free(hilos_telefonos);
}