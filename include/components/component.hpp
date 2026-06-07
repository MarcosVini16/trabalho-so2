#pragma once
#include "../nic/nic_base.hpp"
#include "../nic/nic.hpp"
#include "../engine/shm_engine.hpp"
#include "../protocol.hpp"
#include "../communicator.hpp"
#include "../smart_data/smart_data.hpp"
#include <time.h>
#include <cstdint>
#include <vector>
#include <memory>

extern volatile sig_atomic_t g_stop; // variável global para sinalizar parada total, definida em vehicle_main.cpp

/*
 * Classe base para os componentes do veículo. 
 * Pode se comunicar com outros componentes locais via memória compartilhada.
 */
class Component {
public:

    using SDVector = std::vector<std::unique_ptr<SmartData>>;
    Component(key_t key, Ethernet::Address mac)
        : nic(key, mac), protocol(&nic), _mac(mac) {
            add_smart_datas(); // Método para adicionar SmartData específicos de cada componente
            start_smart_datas(); // Inicia o processamento dos SmartData associados a este componente
    }

    ~Component() {
        // Limpeza de recursos, se necessário
        for (auto& sd : smart_data_units) {
            sd->stop(); // Para o processamento de cada SmartData associado a este componente
        }
    }

    // Método para adicionar SmartData específicos de cada componente, a ser implementado pelas subclasses
    virtual void add_smart_datas() = 0; 

    void start_smart_datas() {
        for (auto& sd : smart_data_units) {
            sd->start(); // Inicia o processamento de cada SmartData associado a este componente
        }
    }

protected:
    NIC<ShmEngine> nic; // NIC para comunicação com outros componentes locais.
    Protocol protocol; // Protocolo de comunicação
    key_t clock_key; // chave para o segmento de memória compartilhada do relógio (pode ser útil para componentes que precisam acessar o relógio sincronizado)
    SDVector smart_data_units; // lista de SmartData associados a este componente (pode ser útil para gerenciar o ciclo de vida dos SmartData)
    Ethernet::Address _mac; // endereço MAC deste componente (pode ser útil para identificação e comunicação)
};