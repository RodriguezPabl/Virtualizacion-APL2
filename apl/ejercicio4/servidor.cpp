#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <getopt.h>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <ctime>
#include <algorithm>
#include "shared_mem.hpp"
#include "utils.hpp"

/*############# INTEGRANTES ###############
###     Justiniano, Máximo              ###
###     Mallia, Leandro                 ###
###     Maudet, Alejandro               ###
###     Naspleda, Julián                ###
###     Rodriguez, Pablo                ###
#########################################*/

using namespace std;

volatile sig_atomic_t terminar_suave = 0;
volatile sig_atomic_t terminar_fuerza = 0;

sem_t* sem_server = nullptr; // Para acceder desde los handlers

void handle_sigusr1(int) {
    terminar_suave = 1;
    if (sem_server) sem_post(sem_server); // Desbloquear espera
}

void handle_sigusr2(int) {
    terminar_fuerza = 1;
    if (sem_server) sem_post(sem_server); // Desbloquear espera
}

void ignore_sigint(int) {
    cout << "\n[Servidor] SIGINT ignorado.\n";
}

void configurar_senales() {
    struct sigaction sa_usr1, sa_usr2, sa_int;

    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa_usr1, nullptr);

    sa_usr2.sa_handler = handle_sigusr2;
    sigemptyset(&sa_usr2.sa_mask);
    sa_usr2.sa_flags = SA_RESTART;
    sigaction(SIGUSR2, &sa_usr2, nullptr);

    sa_int.sa_handler = ignore_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, nullptr);
}

void imprimir_ayuda() {
    cout << "Uso: servidor -a archivo -c intentos\n";
    cout << "Opciones:\n";
    cout << "  -a, --archivo    Archivo con frases (requerido)\n";
    cout << "  -c, --cantidad   Cantidad de intentos por partida (requerido)\n";
    cout << "  -h, --help       Muestra esta ayuda\n";
}

bool letra_en_frase(const string& frase, char letra) {
    return frase.find(letra) != string::npos;
}

void actualizar_visible(char visible[], const char* original, char letra) {
    for (int i = 0; original[i]; ++i) {
        if (tolower(original[i]) == tolower(letra)) {
            visible[i] = original[i];
        }
    }
}

bool frase_completa(const char visible[], const char original[]) {
    return strcmp(visible, original) == 0;
}

int main(int argc, char* argv[]) {
    string rutaArchivo;
    int intentos = 0;

    const struct option longopts[] = {
        {"archivo", required_argument, nullptr, 'a'},
        {"cantidad", required_argument, nullptr, 'c'},
        {"help", no_argument, nullptr, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "a:c:h", longopts, nullptr)) != -1) {
        switch (opt) {
            case 'a': rutaArchivo = optarg; break;
            case 'c': intentos = atoi(optarg); break;
            case 'h': imprimir_ayuda(); return 0;
            default: imprimir_ayuda(); return 1;
        }
    }

    if (rutaArchivo.empty() || intentos <= 0) {
        cerr << "Error: Parámetros requeridos faltantes.\n";
        imprimir_ayuda();
        return 1;
    }

    configurar_senales();

    vector<string> frases = leer_frases(rutaArchivo);
    if (frases.empty()) {
        cerr << "Error: El archivo no contiene frases válidas.\n";
        return 1;
    }

    srand(time(nullptr));

    sem_t* sem_unico = sem_open(SEM_UNICO_SERVIDOR, O_CREAT | O_EXCL, 0666, 1);
    if (sem_unico == SEM_FAILED) {
        cerr << "[Servidor] Ya hay un servidor en ejecución.\n";
        return 1;
    }

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(Juego));
    Juego* juego = (Juego*) mmap(nullptr, sizeof(Juego), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    memset(juego, 0, sizeof(Juego));

    sem_unlink(SEM_SERVER_READY);
    sem_unlink(SEM_CLIENT_READY);
    sem_unlink(SEM_TURNO);
    sem_unlink(SEM_END);

    sem_server = sem_open(SEM_SERVER_READY, O_CREAT, 0666, 0); // global
    sem_t* sem_client = sem_open(SEM_CLIENT_READY, O_CREAT, 0666, 0);
    sem_t* sem_turno  = sem_open(SEM_TURNO, O_CREAT, 0666, 0);
    sem_t* sem_end    = sem_open(SEM_END, O_CREAT, 0666, 0);

    vector<tuple<string, long, int>> ranking;

    while (true) {
        cout << "[Servidor] Esperando cliente...\n";
        sem_wait(sem_server);

        // Si se pidió finalización suave y no hay partida, salimos
        if (terminar_suave && juego->en_partida == 0) {
            cout << "[Servidor] SIGUSR1 recibido y no hay partida activa. Finalizando...\n";
            break;
        }

        // Si se pidió finalización forzosa, finalizamos (aunque no haya partida)
        if (terminar_fuerza) {
            if (juego->en_partida) {
                cout << "[Servidor] SIGUSR2 recibido. Finalizando partida en curso...\n";
                juego->gano = 0;
                juego->tiempo_fin = timestamp_ms();
                sem_post(sem_client); // Despertar cliente si estaba esperando
                sem_post(sem_end);    // Avisar fin de partida
                juego->en_partida = 0;

                ranking.emplace_back(string(juego->nickname), juego->tiempo_fin - juego->tiempo_inicio, juego->gano);
            }
            break;
        }


        juego->en_partida = 1;
        string frase = frases[rand() % frases.size()];
        strncpy(juego->frase_original, frase.c_str(), MAX_FRASE);
        string visible = ocultar_frase(frase);
        strncpy(juego->frase_visible, visible.c_str(), MAX_FRASE);
        juego->intentos_restantes = intentos;
        juego->gano = -1;
        juego->tiempo_inicio = timestamp_ms();

        sem_post(sem_client);

        while (juego->gano == -1 && juego->intentos_restantes > 0 && !terminar_fuerza) {
            sem_wait(sem_turno);
            char letra = juego->letras_usadas[strlen(juego->letras_usadas) - 1];
            if (!letra_en_frase(juego->frase_original, letra)) {
                juego->intentos_restantes--;
            } else {
                actualizar_visible(juego->frase_visible, juego->frase_original, letra);
            }
            if (frase_completa(juego->frase_visible, juego->frase_original)) {
                juego->gano = 1;
            } else if (juego->intentos_restantes == 0) {
                juego->gano = 0;
            }
            sem_post(sem_client);
        }

        // Si se recibió SIGUSR2 (final forzoso) y la partida seguía activa
        if (terminar_fuerza && juego->gano == -1) {
            juego->gano = 0;  // Se considera derrota
            sem_post(sem_client); // Despertar cliente si estaba esperando
        }


        juego->tiempo_fin = timestamp_ms();
        long duracion = juego->tiempo_fin - juego->tiempo_inicio;
        ranking.emplace_back(string(juego->nickname), duracion, juego->gano);

        juego->en_partida = 0;
        sem_post(sem_end);
    }

    // Mostrar ranking ordenado
    cout << "\n[Servidor] Finalizando. Ranking de jugadores:\n";

    sort(ranking.begin(), ranking.end(), [](const auto& a, const auto& b) {
        int gano_a = get<2>(a);
        int gano_b = get<2>(b);
        if (gano_a != gano_b) return gano_a > gano_b;  // Ganadores primero
        return get<1>(a) < get<1>(b); // Menor tiempo primero
    });

    for (const auto& r : ranking) {
        const string& nombre = get<0>(r);
        long duracion = get<1>(r);
        int gano = get<2>(r);

        cout << nombre << " - " << duracion << " ms";
        if (gano != 1) cout << " (perdió)";
        cout << "\n";
    }

    // Liberación de recursos
    munmap(juego, sizeof(Juego));
    close(shm_fd);
    shm_unlink(SHM_NAME);

    sem_close(sem_server);
    sem_close(sem_client);
    sem_close(sem_turno);
    sem_close(sem_end);
    sem_close(sem_unico);

    sem_unlink(SEM_SERVER_READY);
    sem_unlink(SEM_CLIENT_READY);
    sem_unlink(SEM_TURNO);
    sem_unlink(SEM_END);
    sem_unlink(SEM_UNICO_SERVIDOR);

    return 0;

}
