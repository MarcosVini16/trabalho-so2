#include <iostream>
#include "include/utils/periodic_thread.hpp"

void some_function() {
    std::cout << "Hello from the periodic thread!" << std::endl;
}

int main() {
    // Cria um PeriodicThread que executa some_function a cada 500000 microssegundos (500 ms)
    PeriodicThread pt(some_function, 500000);

    // Deixa o programa rodar por um tempo para observar a saída
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // O PeriodicThread será automaticamente parado e juntado quando sair do escopo
    return 0;
}