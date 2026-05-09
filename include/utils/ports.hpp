// infra/ports.hpp
#pragma once
#include <cstdint>

/*
 * @brief Portas usadas para comunicação entre componentes e com o RSU
 * 
 * Cada componente tem uma porta fixa para facilitar o roteamento das mensagens.
 * O RSU também tem uma porta reservada para receber mensagens de sincronização de tempo.
*/
namespace Ports {
    static const uint16_t GATEWAY     = 1000;
    static const uint16_t SENSOR      = 1001;
    static const uint16_t ACTUATOR    = 1002;
    static const uint16_t POWERTRAIN  = 1003;
    static const uint16_t TIME_CLIENT = 1004;
    static const uint16_t RSU         = 9999; // porta reservada para RSU
}