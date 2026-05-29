#include <cstdint>
#include <cstring>
#include "rng.hpp"

/*
 * Representação de códigos dos tipos de dados que podem ser passados entre as 
 * mensagens da aplicação (campo Type). 
 * Cada código é um uint8_t que representa um tipo específico de dado, como 
 * velocidade ou temperatura.
 * Exemplo de uso: TypeCode::VELOCITY
 */
enum class TypeCode : uint8_t {
    VELOCITY = 0x01, // em km/h -> double
    TEMPERATURE = 0x02, // em graus Celsius -> float
    ACCELERATION = 0x03, // em m/s^2 -> double
    ORIENTATION = 0x04, // em graus (0-360) -> double
    REAR_DISTANCE = 0x05, // em metros (distância do sensor traseiro) -> float
    BATTERY_LEVEL = 0x06, // em porcentagem -> uint8_t
    FUEL_LEVEL = 0x07, // em porcentagem -> uint8_t
    DOOR_STATUS = 0x08, // false = fechado, true = aberto -> bool
    LIGHT_STATUS = 0x09, // false = desligado, true = ligado -> bool
    BRAKE_STATUS = 0x0A, // false = desativado, true = ativado  -> bool
    STEERING_ANGLE = 0x0B, // em graus -> double
    CURRENT_GEAR = 0x0C, // marcha atual (P, R, N, D, etc.) -> char
    ENGINE_RPM = 0x0D, // em rotações por minuto -> uint32_t
    HORN_STATUS = 0x0E, // false = desligado, true = ligado -> bool
    WIPER_STATUS = 0x0F, // false = desligado, true = ligado -> bool
    TURN_SIGNAL_STATUS = 0x10, // 0 = desligado, 1 = esquerda, 2 = direita -> uint8_t
};

/*
 * Retorna o nome do tipo de código fornecido.
 */
constexpr const char* name_of(TypeCode t) {
    switch (t) {
        case TypeCode::VELOCITY:      return "VELOCIDADE";
        case TypeCode::TEMPERATURE:   return "TEMPERATURA";
        case TypeCode::ACCELERATION:  return "ACELERAÇÃO";
        case TypeCode::ORIENTATION:   return "ORIENTAÇÃO";
        case TypeCode::REAR_DISTANCE: return "DISTÂNCIA TRASEIRA";
        case TypeCode::BATTERY_LEVEL: return "NÍVEL DA BATERIA";
        case TypeCode::FUEL_LEVEL:    return "NÍVEL DO COMBUSTÍVEL";
        case TypeCode::DOOR_STATUS:   return "STATUS DA PORTA";
        case TypeCode::LIGHT_STATUS:  return "STATUS DA LUZ";
        case TypeCode::BRAKE_STATUS:  return "STATUS DO FREIO";
        case TypeCode::STEERING_ANGLE: return "ÂNGULO DE DIREÇÃO";
        case TypeCode::CURRENT_GEAR:  return "MARCHA ATUAL";
        case TypeCode::ENGINE_RPM:    return "RPM DO MOTOR";
        case TypeCode::HORN_STATUS:   return "STATUS DA BUZINA";
        case TypeCode::WIPER_STATUS:  return "STATUS DO LIMPADOR";
        case TypeCode::TURN_SIGNAL_STATUS: return "STATUS DO SINAL DE SETA";
    }
    return "DESCONHECIDO";
}

/*
 * Gera dados aleatórios para o tipo de código fornecido.
 * @param code O tipo de código para o qual gerar dados.
 * @param data O buffer para armazenar os dados gerados.
 * @param max_size O tamanho máximo do buffer.
 * @return O tamanho dos dados gerados, ou 0 em caso de erro.
 */
size_t gen_data(TypeCode code, uint8_t* data, size_t max_size) {
    switch (code) {
        case TypeCode::VELOCITY: {
            double value = RNG::getDouble(0.0, 200.0); // Exemplo: velocidade entre 0 e 200 km/h
            if (max_size < sizeof(double)) return 0; // Verifica se há espaço suficiente
            std::memcpy(data, &value, sizeof(double)); // Copia o valor para o buffer
            return sizeof(double);
        }
        case TypeCode::TEMPERATURE: {
            float value = RNG::getFloat(-40.0f, 125.0f); // Exemplo: temperatura entre -40 e 125 °C
            if (max_size < sizeof(float)) return 0;
            std::memcpy(data, &value, sizeof(float));
            return sizeof(float);
        }
        case TypeCode::ACCELERATION: {
            double value = RNG::getDouble(-10.0, 10.0); // Exemplo: aceleração entre -10 e 10 m/s^2
            if (max_size < sizeof(double)) return 0;
            std::memcpy(data, &value, sizeof(double));
            return sizeof(double);
        }
        case TypeCode::ORIENTATION: {
            double value = RNG::getDouble(0.0, 360.0); // Exemplo: orientação entre 0 e 360 graus
            if (max_size < sizeof(double)) return 0;
            std::memcpy(data, &value, sizeof(double));
            return sizeof(double);
        }
        case TypeCode::REAR_DISTANCE: {
            float value = RNG::getFloat(0.0f, 10.0f); // metros
            if (max_size < sizeof(float)) return 0;
            std::memcpy(data, &value, sizeof(float));
            return sizeof(float);
        }
        case TypeCode::BATTERY_LEVEL: {
            uint8_t value = RNG::getUInt8(0, 100); // porcentagem
            if (max_size < sizeof(uint8_t)) return 0;
            std::memcpy(data, &value, sizeof(uint8_t));
            return sizeof(uint8_t);
        }
        case TypeCode::FUEL_LEVEL: {
            uint8_t value = RNG::getUInt8(0, 100); // porcentagem
            if (max_size < sizeof(uint8_t)) return 0;
            std::memcpy(data, &value, sizeof(uint8_t));
            return sizeof(uint8_t);
        }
        case TypeCode::DOOR_STATUS: {
            bool value = RNG::getBool();
            if (max_size < sizeof(bool)) return 0;
            std::memcpy(data, &value, sizeof(bool));
            return sizeof(bool);
        }
        case TypeCode::LIGHT_STATUS: {
            bool value = RNG::getBool();
            if (max_size < sizeof(bool)) return 0;
            std::memcpy(data, &value, sizeof(bool));
            return sizeof(bool);
        }
        case TypeCode::BRAKE_STATUS: {
            bool value = RNG::getBool();
            if (max_size < sizeof(bool)) return 0;
            std::memcpy(data, &value, sizeof(bool));
            return sizeof(bool);
        }
        case TypeCode::STEERING_ANGLE: {
            double value = RNG::getDouble(-180.0, 180.0); // graus
            if (max_size < sizeof(double)) return 0;
            std::memcpy(data, &value, sizeof(double));
            return sizeof(double);
        }
        case TypeCode::CURRENT_GEAR: {
            // Escolhe entre P, R, N, D, 1, 2, 3
            const char gears[] = {'P','R','N','D','1','2','3'};
            char value = gears[RNG::getUInt8(0, sizeof(gears)-1)];
            if (max_size < sizeof(char)) return 0;
            std::memcpy(data, &value, sizeof(char));
            return sizeof(char);
        }
        case TypeCode::ENGINE_RPM: {
            uint32_t value = RNG::getUInt32(0, 8000); // RPM
            if (max_size < sizeof(uint32_t)) return 0;
            std::memcpy(data, &value, sizeof(uint32_t));
            return sizeof(uint32_t);
        }
        case TypeCode::HORN_STATUS: {
            bool value = RNG::getBool();
            if (max_size < sizeof(bool)) return 0;
            std::memcpy(data, &value, sizeof(bool));
            return sizeof(bool);
        }
        case TypeCode::WIPER_STATUS: {
            bool value = RNG::getBool();
            if (max_size < sizeof(bool)) return 0;
            std::memcpy(data, &value, sizeof(bool));
            return sizeof(bool);
        }
        case TypeCode::TURN_SIGNAL_STATUS: {
            uint8_t value = RNG::getUInt8(0, 2); // 0 desligado, 1 esquerda, 2 direita
            if (max_size < sizeof(uint8_t)) return 0;
            std::memcpy(data, &value, sizeof(uint8_t));
            return sizeof(uint8_t);
        }
        default:
            return 0; // Tipo desconhecido ou não implementado
    }
}