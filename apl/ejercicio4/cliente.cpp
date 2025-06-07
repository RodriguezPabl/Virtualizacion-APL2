#include <iostream>
#include <cstring>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <sys/mman.h>
#include <semaphore.h>
#include <cstdlib>
#include <cctype>
#include "shared_mem.hpp"

using namespace std;

void imprimir_ayuda() {
    cout << "Uso: cliente -n nickname\n";
    cout << "Opciones:\n";
    cout << "  -n, --nickname    Nickname del jugador (requerido)\n";
    cout << "  -h, --help        Muestra esta ayuda\n";
}

void ignore_sigint(int) {
    cout << "\n[Cliente] SIGINT ignorado.\n";
}

void configurar_senales() {
    struct sigaction sa_int;
    sa_int.sa_handler = ignore_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, nullptr);
}

int main(int argc, char* argv[]) {
    string nickname;

    const struct option longopts[] = {
        {"nickname", required_argument, nullptr, 'n'},
        {"help", no_argument, nullptr, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "n:h", longopts, nullptr)) != -1) {
        switch (opt) {
            case 'n': nickname = optarg; break;
            case 'h': imprimir_ayuda(); return 0;
            default: imprimir_ayuda(); return 1;
        }
    }

    if (nickname.empty()) {
        cerr << "Error: Falta el nickname.\n";
        imprimir_ayuda();
        return 1;
    }

    configurar_senales();

    sem_t* sem_unico = sem_open(SEM_UNICO_CLIENTE, O_CREAT | O_EXCL, 0666, 1);
    if (sem_unico == SEM_FAILED) {
        cerr << "[Cliente] Ya hay un cliente en ejecución.\n";
        return 1;
    }

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        cerr << "[Cliente] No se pudo conectar con el servidor.\n";
        sem_unlink(SEM_UNICO_CLIENTE);
        return 1;
    }

    Juego* juego = (Juego*) mmap(nullptr, sizeof(Juego), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    sem_t* sem_server = sem_open(SEM_SERVER_READY, 0);
    sem_t* sem_client = sem_open(SEM_CLIENT_READY, 0);
    sem_t* sem_turno  = sem_open(SEM_TURNO, 0);
    sem_t* sem_end    = sem_open(SEM_END, 0);

    strncpy(juego->nickname, nickname.c_str(), MAX_NICK);
    juego->letras_usadas[0] = '\0';

    sem_post(sem_server); // Avisar al servidor que hay un cliente
    sem_wait(sem_client); // Esperar a que el servidor envíe la frase

    while (juego->gano == -1 && juego->intentos_restantes > 0) {
        cout << "\nFrase: " << juego->frase_visible;
        cout << "\nIntentos restantes: " << juego->intentos_restantes << "\n";

        char letra;
        cout << "Ingrese una letra: ";
        cin >> letra;
        letra = tolower(letra);

        int len = strlen(juego->letras_usadas);
        if (strchr(juego->letras_usadas, letra)) {
            cout << "Letra ya ingresada. Intente otra.\n";
            continue;
        }

        juego->letras_usadas[len] = letra;
        juego->letras_usadas[len + 1] = '\0';

        sem_post(sem_turno); // Enviar letra al servidor
        sem_wait(sem_client); // Esperar respuesta
    }

    if (juego->gano == 1) {
        cout << "\n\033[1;32mGanaste!\033[0m La frase era: " << juego->frase_original << "\n";
    } else {
        cout << "\n\033[1;31mPerdiste.\033[0m La frase era: " << juego->frase_original << "\n";
    }

    sem_wait(sem_end); // Confirmar fin de partida

    munmap(juego, sizeof(Juego));
    close(shm_fd);
    sem_close(sem_server);
    sem_close(sem_client);
    sem_close(sem_turno);
    sem_close(sem_end);
    sem_close(sem_unico);
    sem_unlink(SEM_UNICO_CLIENTE);

    return 0;
}
