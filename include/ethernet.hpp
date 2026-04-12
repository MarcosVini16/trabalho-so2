#pragma once
#include <cstdint>
#include <cstring>

// representa o protocolo Ethernet
class Ethernet {
    public:

        // tamanho máximo do payload de um frame (padrão Ethernet)
        // MTU = Maximun Transmission Unit
        static const unsigned int MTU = 1500;

        // define o endereço MAC (aa:bb:cc:dd:ee:ff)
        struct Address {
            uint8_t bytes[6];
            
            // operadores de comparação
            bool operator==(const Address& o) const {
                return std::memcmp(bytes, o.bytes, 6) == 0;
            }
            bool operator!=(const Address& o) const { return !(*this == o); }

            // retorna o endereço especial (f:ff:ff:ff:ff:ff) que significa todos os dispositivos da rede local,
            // ao mandar um frame com destino broadcast, todos na rede o recebem
            static Address BROADCAST() {
                return Address{{0xff,0xff,0xff,0xff,0xff,0xff}};
            }
        };

        // apelido
        // no projeto: identificador = 0x888
        using Protocol = uint16_t;

        // formato do frame Ethernet
        struct Frame {
            // endereço MAC de destino (6 bytes)
            Address  dst;
            // endereço MAC de origem (6 bytes)
            Address  src;
            // EtherType (identificador do protocolo) (2 bytes)
            Protocol type;
            // payload (1500 bytes)
            uint8_t  data[MTU];
        }
        // garante que não haverão bytes invisíveis entre os campos
        __attribute__((packed));
};