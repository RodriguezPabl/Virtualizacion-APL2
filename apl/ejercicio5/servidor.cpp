/*############# INTEGRANTES ###############
###     Justiniano, Máximo              ###
###     Mallia, Leandro                 ###
###     Maudet, Alejandro               ###
###     Naspleda, Julián                ###
###     Rodriguez, Pablo                ###
#########################################*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <algorithm>
#include <csignal>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <semaphore.h>

using namespace std;

#define MAX_BUFFER 1024

// Estructura para el ranking
struct Jugador {
    string nickname;
    int aciertos;
};

vector<string> frases;
vector<Jugador> ranking;
mutex rankingMutex;
mutex coutMutex;
sem_t semUsuarios;

void cargarFrasesDesdeArchivo(const string &archivo) {
    ifstream file(archivo);
    if (!file.is_open()) {
        cerr << "[ERROR] No se pudo abrir el archivo de frases: " << archivo << endl;
        exit(EXIT_FAILURE);
    }
    string linea;
    while (getline(file, linea)) {
        if (!linea.empty()) frases.push_back(linea);
    }
    file.close();
}

void mostrarAyudaServidor() {
    cout << "Uso: servidor -p <puerto> -u <usuarios_max> -a <archivo_frases>\n";
    cout << "Opciones:\n";
    cout << "  -p, --puerto     Puerto del servidor (requerido)\n";
    cout << "  -u, --usuarios   Cantidad de usuarios concurrentes (requerido)\n";
    cout << "  -a, --archivo    Archivo de frases (requerido)\n";
    cout << "  -h, --help       Muestra esta ayuda\n";
    exit(EXIT_SUCCESS);
}

void atenderCliente(int socketCliente) {
    sem_wait(&semUsuarios);

    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);

    // Recibir nickname
    int bytes = recv(socketCliente, buffer, MAX_BUFFER - 1, 0);
    if (bytes <= 0) {
        cerr << "[ERROR] No se recibió nickname del cliente.\n";
        close(socketCliente);
        sem_post(&semUsuarios);
        return;
    }
    string nickname(buffer);
    nickname.erase(nickname.find_last_not_of(" \n\r\t")+1);

    {
        lock_guard<mutex> lock(coutMutex);
        cout << "[SERVIDOR] Jugador '" << nickname << "' conectado.\n";
    }

    srand(time(nullptr) + socketCliente);
    string frase = frases[rand() % frases.size()];
    string estado(frase.size(), '_');
    for (size_t i = 0; i < frase.size(); ++i)
        if (frase[i] == ' ') estado[i] = ' ';

    int intentosRestantes = 6;
    bool juegoTerminado = false;
    vector<char> letrasUsadas;

    string estadoInicial = "Frase: " + estado + "\nIntentos restantes: " + to_string(intentosRestantes) + "\n";
    send(socketCliente, estadoInicial.c_str(), estadoInicial.size(), 0);

    while (!juegoTerminado && intentosRestantes > 0) {
        memset(buffer, 0, MAX_BUFFER);
        int bytesRecibidos = recv(socketCliente, buffer, MAX_BUFFER - 1, 0);
        if (bytesRecibidos <= 0) {
            cerr << "[SERVIDOR] Jugador '" << nickname << "' desconectado abruptamente.\n";
            break;
        }

        char letra = tolower(buffer[0]);

        if (find(letrasUsadas.begin(), letrasUsadas.end(), letra) != letrasUsadas.end()) {
            string msg = "Ya usaste esa letra. Estado: " + estado + "\nIntentos restantes: " + to_string(intentosRestantes) + "\n";
            send(socketCliente, msg.c_str(), msg.size(), 0);
            continue;
        }

        letrasUsadas.push_back(letra);
        bool acierto = false;
        for (size_t i = 0; i < frase.size(); ++i) {
            if (tolower(frase[i]) == letra) {
                estado[i] = frase[i];
                acierto = true;
            }
        }

        if (!acierto) intentosRestantes--;

        if (estado == frase) {
            string msg = "\n\033[1;32mGanaste!\033[0m La frase era: " + frase + "\n";
            send(socketCliente, msg.c_str(), msg.size(), 0);
            juegoTerminado = true;
        } else if (intentosRestantes == 0) {
            string msg = "\n\033[1;31mPerdiste.\033[0m La frase era: " + frase + "\n";
            send(socketCliente, msg.c_str(), msg.size(), 0);
            juegoTerminado = true;
        } else {
            string estadoMsg = "Frase: " + estado + "\nIntentos restantes: " + to_string(intentosRestantes) + "\n";
            send(socketCliente, estadoMsg.c_str(), estadoMsg.size(), 0);
        }
    }

    int totalAciertos = count_if(estado.begin(), estado.end(), [](char c) { return c != '_' && c != ' '; });
    {
        lock_guard<mutex> lock(rankingMutex);
        ranking.push_back({nickname, totalAciertos});
    }

    close(socketCliente);
    sem_post(&semUsuarios);
}

void manejarSenales(int signum) {
    cout << "\n[SERVIDOR] Finalizando por se\u00f1al (" << signum << ").\n";
    cout << "========== RANKING FINAL ==========" << endl;
    {
        lock_guard<mutex> lock(rankingMutex);
        sort(ranking.begin(), ranking.end(), [](const Jugador &a, const Jugador &b) {
            return a.aciertos > b.aciertos;
        });

        for (size_t i = 0; i < ranking.size(); ++i) {
            cout << i + 1 << ". " << ranking[i].nickname << " - " << ranking[i].aciertos << " aciertos\n";
        }
    }
    cout << "===================================\n";
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    int puerto = -1;
    int usuariosMax = -1;
    string archivoFrases;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-p" || arg == "--puerto") {
            puerto = atoi(argv[++i]);
        } else if (arg == "-u" || arg == "--usuarios") {
            usuariosMax = atoi(argv[++i]);
        } else if (arg == "-a" || arg == "--archivo") {
            archivoFrases = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            mostrarAyudaServidor();
        }
    }

    if (puerto == -1 || usuariosMax == -1 || archivoFrases.empty()) {
        cerr << "[ERROR] Faltan argumentos obligatorios. Use -h para ayuda.\n";
        exit(EXIT_FAILURE);
    }

    cargarFrasesDesdeArchivo(archivoFrases);
    sem_init(&semUsuarios, 0, usuariosMax);

    struct sigaction sa;
    sa.sa_handler = manejarSenales;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cerr << "[ERROR] No se pudo crear el socket\n";
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverConfig;
    memset(&serverConfig, 0, sizeof(serverConfig));
    serverConfig.sin_family = AF_INET;
    serverConfig.sin_addr.s_addr = htonl(INADDR_ANY);
    serverConfig.sin_port = htons(puerto);

    if (bind(serverSocket, (struct sockaddr *)&serverConfig, sizeof(serverConfig)) < 0) {
        cerr << "[ERROR] No se pudo bindear el socket\n";
        exit(EXIT_FAILURE);
    }

    listen(serverSocket, usuariosMax);
    cout << "[SERVIDOR] Escuchando en puerto " << puerto << "...\n";

    while (true) {
        int socketCliente = accept(serverSocket, NULL, NULL);
        if (socketCliente >= 0) {
            thread th(atenderCliente, socketCliente);
            th.detach();
        }
    }

    return EXIT_SUCCESS;
}
