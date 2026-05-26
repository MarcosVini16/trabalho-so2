// app/rsu_main.cpp
#include <csignal>
#include <unistd.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <iostream>
#include <string>
#include "../include/RSU.hpp"

// ---------------------------------------------------------------------------
// Sinal de parada total
// ---------------------------------------------------------------------------

volatile sig_atomic_t g_stop = 0;

extern "C" void on_stop(int) {
    g_stop = 1;
}

void setup_signals() {
    struct sigaction sa{};
    sa.sa_handler = on_stop;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT,  &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGALRM, &sa, nullptr);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    std::cout << "=== RSU main ===\n";
    setup_signals();

    int timeout_sec = (argc > 2) ? std::stoi(argv[2]) : 60;
    alarm(timeout_sec); // shutdown automático para testes

    const std::string iface = (argc > 1) ? argv[1] : "eth0";

    uint8_t quadrant = (argc > 3) ? std::stoi(argv[3]) : 0;
    RSU rsu(iface, quadrant);
    std::cout << "[rsu] pid=" << getpid() << " iniciado em iface=" << iface << "\n";

    rsu.run(); // bloqueia até g_stop == 1

    std::cout << "[rsu] saiu do loop. Desligando VM.\n";
    sync();
    reboot(RB_POWER_OFF);
    return 0; // nunca alcançado
}