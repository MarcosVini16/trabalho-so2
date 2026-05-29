#include <iostream>
#include <random>
#include <cstdint>

/**
 * Classe que fornece métodos para geração de números aleatórios.
 */
class RNG {
private:
    /*
     * Inicializa um gerador de números aleatórios com uma semente única.
     * Garante um único gerador por thread.
    */
    static std::mt19937& getEngine() {
        thread_local static std::random_device rd;
        thread_local static std::mt19937 gen(rd());
        return gen;
    }

public:
    
    /*
     * Gera um número aleatório do tipo double no intervalo [min, max).
     */
    static double getDouble(double min, double max) {
        std::uniform_real_distribution<double> dis(min, max);
        return dis(getEngine());
    }

    /*
     * Gera um número aleatório do tipo float no intervalo [min, max).
     */
    static float getFloat(float min, float max) {
        std::uniform_real_distribution<float> dis(min, max);
        return dis(getEngine());
    }

    /*
     * Gera um número aleatório do tipo bool no intervalo [0, 1).
     * Retorna true com base na probabilidade informada (0.5 = 50% de chance).
     */
    static bool getBool(double probability = 0.5) {
        std::bernoulli_distribution dis(probability);
        return dis(getEngine());
    }

    /*
     * Gera um número aleatório do tipo uint32_t no intervalo [min, max].
     */
    static uint32_t getUInt32(uint32_t min, uint32_t max) {
        std::uniform_int_distribution<uint32_t> dis(min, max);
        return dis(getEngine());
    }

    /*
     * Gera um número aleatório do tipo uint8_t no intervalo [min, max].
     * Nota: Usa uint16_t, depois converte para uint8_t. Cuidado com overflow se 
     * max > 255.
     */
    static uint8_t getUInt8(uint8_t min, uint8_t max) {
        std::uniform_int_distribution<uint16_t> dis(min, max);
        return static_cast<uint8_t>(dis(getEngine()));
    }

    /*
     * Gera um número aleatório do tipo char no intervalo [min, max].
     * Nota: Usa int internamente na distribuição.
     */
    static char getChar(char min, char max) {
        std::uniform_int_distribution<int> dis(min, max);
        return static_cast<char>(dis(getEngine()));
    }
};