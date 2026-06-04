#include <cstdint>

/*
 * Definições base para o Smart Data, um formato compacto para representar 
 * dados numéricos com unidades inspirado no IEEE 1451 e no Smart Data do EPOS.
 */
class SmartData {
public:
    /*
     * Codificação de unidades inspirada no IEEE 1451 e no Smart Data do EPOS.
     *
     * Layout dos 32 bits (sempre SI, sempre modo direto):
     *
     *   Bit  31    30 29    28 27    24    21    18    15    12    9     6     3     0
     *       +----+-------+-------+------+-----+-----+------+-----+-----+-----+-------+------+
     *       | SI |  NUM  |  MOD  | sr+4 |rad+4| m+4 | kg+4 | s+4 | A+4 | K+4 | mol+4 | cd+4 |
     *       +----+-------+-------+------+-----+-----+------+-----+-----+-----+-------+------+
     *
     *   SI    = 1 (sempre)          1 bit
     *   NUM   = tipo numérico       2 bits  (I32, I64, F32, F64)
     *   MOD   = 0 (sempre DIR)      2 bits
     *   exp+4 = expoente + 4        3 bits cada (expoente válido: -4 a +3)
     */
    class Unit {
        
    public:
        // O campo de código é o valor inteiro de 32 bits que representa a unidade, incluindo os diversos campos de informação.
        using Code = uint32_t;
        // Campo com valor de 2 bits que indica o tipo de dado numérico (I32, I64, F32, F64).
        enum NumType : uint8_t {
            I32 = 0,
            I64 = 1,
            F32 = 2,
            F64 = 3
        };
    
    private:
        static constexpr Code SI_BIT   = 1u << 31; // Bit 31
        static constexpr Code DIR_MODE = 0u << 28; // Bits 28-29 (sempre 00 para modo direto)
        static constexpr int EXP       = 4; // Valor de deslocamento para o expoente (expoente real = exp+4)

        // Posições dos campos de expoente
        static constexpr int SR_POS  = 24;
        static constexpr int RAD_POS = 21;
        static constexpr int M_POS   = 18;
        static constexpr int KG_POS  = 15;
        static constexpr int S_POS   = 12;
        static constexpr int A_POS   = 9;
        static constexpr int K_POS   = 6;
        static constexpr int MOL_POS = 3;
        static constexpr int CD_POS  = 0;
    }__attribute__((packed));
};