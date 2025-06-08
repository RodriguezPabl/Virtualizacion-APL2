#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

using namespace std;

#define MAX_BUFFER 1024

void mostrarAyudaCliente() {
    cout << "Uso: cliente -n <nickname> -p <puerto> -s <servidor>\n";
    cout << "Opciones:\n";
    cout << "  -n, --nickname   Nombre de usuario (requerido)\n";
    cout << "  -p, --puerto     Puerto del servidor (requerido)\n";
    cout << "  -s, --servidor   IP o nombre del servidor (requerido)\n";
    cout << "  -h, --help       Muestra esta ayuda\n";
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
    string nickname;
    int puerto = -1;
    string servidor;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-n" || arg == "--nickname") {
            nickname = argv[++i];
        } else if (arg == "-p" || arg == "--puerto") {
            puerto = atoi(argv[++i]);
        } else if (arg == "-s" || arg == "--servidor") {
            servidor = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            mostrarAyudaCliente();
        }
    }

    if (nickname.empty() || puerto == -1 || servidor.empty()) {
        cerr << "[ERROR] Faltan argumentos obligatorios. Use -h para ayuda.\n";
        return EXIT_FAILURE;
    }

    int socketCliente = socket(AF_INET, SOCK_STREAM, 0);
    if (socketCliente < 0) {
        cerr << "[ERROR] No se pudo crear el socket.\n";
        return EXIT_FAILURE;
    }

    struct sockaddr_in serverConfig;
    memset(&serverConfig, 0, sizeof(serverConfig));
    serverConfig.sin_family = AF_INET;
    serverConfig.sin_port = htons(puerto);

    if (inet_pton(AF_INET, servidor.c_str(), &serverConfig.sin_addr) <= 0) {
        cerr << "[ERROR] Dirección IP inválida o no resolvible.\n";
        return EXIT_FAILURE;
    }

    if (connect(socketCliente, (struct sockaddr*)&serverConfig, sizeof(serverConfig)) < 0) {
        cerr << "[ERROR] No se pudo conectar al servidor.\n";
        return EXIT_FAILURE;
    }

    // Enviar nickname al servidor
    send(socketCliente, nickname.c_str(), nickname.size(), 0);

    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);

    // Recibir estado inicial del juego
    int bytes = recv(socketCliente, buffer, MAX_BUFFER - 1, 0);
    if (bytes <= 0) {
        cerr << "[ERROR] No se recibió estado inicial.\n";
        close(socketCliente);
        return EXIT_FAILURE;
    }
    cout << buffer;

    while (true) {
        cout << "Ingresa una letra: ";
        string entrada;
        getline(cin, entrada);

        if (entrada.empty()) continue;
        char letra = entrada[0];

        send(socketCliente, &letra, 1, 0);

        memset(buffer, 0, MAX_BUFFER);
        int bytesRecibidos = recv(socketCliente, buffer, MAX_BUFFER - 1, 0);
        if (bytesRecibidos <= 0) {
            cerr << "\n[SERVIDOR DESCONECTADO] El servidor ha cerrado la conexión.\n";
            break;
        }
        cout << buffer;

        if (strstr(buffer, "Adivinaste") || strstr(buffer, "Perdiste")) {
            break;
        }
    }

    close(socketCliente);
    return EXIT_SUCCESS;
}
