// utils.cpp
#include "utils.hpp"
#include <fstream>
#include <chrono>
#include <cctype>

std::string ocultar_frase(const std::string& frase) {
    std::string oculta;
    for (char c : frase)
        oculta += (c == ' ' ? ' ' : '_');
    return oculta;
}

std::vector<std::string> leer_frases(const std::string& ruta) {
    std::vector<std::string> frases;
    std::ifstream f(ruta);
    std::string linea;
    while (getline(f, linea))
        if (!linea.empty()) frases.push_back(linea);
    return frases;
}

long timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}
