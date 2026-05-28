#include <cstdint>

/*
 * Representação de códigos dos tipos de dados que podem ser passados entre as 
 * mensagens da aplicação (campo Type). 
 * Cada código é um uint8_t que representa um tipo específico de dado, como 
 * velocidade ou temperatura.
 * Exemplo de uso: TypeCode::VELOCITY
 */
enum class TypeCode : uint8_t {
    VELOCITY = 0x01, // em km/h
    TEMPERATURE = 0x02, // em graus Celsius
    ACCELERATION = 0x03, // em m/s^2
    ORIENTATION = 0x04, // em graus
    REAR_DISTANCE = 0x05, // em metros (distância do sensor traseiro)
    BATTERY_LEVEL = 0x06, // em porcentagem
    FUEL_LEVEL = 0x07, // em porcentagem
    DOOR_STATUS = 0x08, // false = fechado, true = aberto
    LIGHT_STATUS = 0x09, // false = desligado, true = ligado
    BRAKE_STATUS = 0x0A, // false = desativado, true = ativado
    STEERING_ANGLE = 0x0B, // em graus
    CURRENT_GEAR = 0x0C, // char representando a marcha atual (P, R, N, D, etc.)
    ENGINE_RPM = 0x0D, // em rotações por minuto
    HORN_STATUS = 0x0E, // false = desligado, true = ligado
    WIPER_STATUS = 0x0F, // false = desligado, true = ligado
    TURN_SIGNAL_STATUS = 0x10, // 0 = desligado, 1 = esquerda, 2 = direita
};