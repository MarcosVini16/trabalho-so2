#include <cstdint>

/*
 * Definições base para o Smart Data, um formato compacto para representar 
 * dados numéricos com unidades inspirado no IEEE 1451 e no Smart Data do EPOS.
 */
class SmartData {
public:

    virtual start() = 0;
    virtual stop() = 0;
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
            D64 = 3,
            NUM = D64 // Máscara para acessar o campo de tipo numérico
        };

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

        enum: Code {
            DIR     = 0 << 27, // Unidade é o produto das unidades SI base elevadas às potências registradas nos campos de expoente restantes.
            DIV     = 1 << 27, // Unidade é o quociente das unidades SI base elevadas às potências registradas nos campos de expoente restantes.
            LOG     = 2 << 27, // Unidade é o logaritmo natural do quociente das unidades SI base elevadas às potências registradas nos campos de expoente restantes.
            LOG_DIV = 3 << 27, // Unidade é o logaritmo natural do quociente das unidades SI base elevadas às potências registradas nos campos de expoente restantes.
            MOD     = LOG_DIV  // Máscara para acessar o campo de modo (DIR, DIV, LOG, LOG_DIV)
        };

        // Máscaras para acessar os campos de expoente
        enum: Code {
            SR = 7 << SR_POS, // expoente para segundos (s)
            RAD = 7 << RAD_POS, // expoente para radianos (rad)
            M = 7 << M_POS, // expoente para metros (m)
            KG = 7 << KG_POS, // expoente para quilogramas (kg)
            S = 7 << S_POS, // expoente para segundos (s)
            A = 7 << A_POS, // expoente para amperes (A)
            K = 7 << K_POS, // expoente para kelvin (K)
            MOL = 7 << MOL_POS, // expoente para mols (mol)
            CD = 7 << CD_POS, // expoente para candelas (cd)
        };
    
        static constexpr Code SI_BIT   = 1u << 31; // Bit 31
        static constexpr int EXP       = 4; // Valor de deslocamento para o expoente (expoente real = exp+4)

        template<int _MOD, int _SR, int _RAD, int _M, int _KG, int _S, int _A, int _K, int _MOL, int _CD>
        class SIUnit {
        public:
            enum: Code { UNIT = SI_BIT | _MOD | ((_SR + EXP) << SR_POS) | ((_RAD + EXP) << RAD_POS) | ((_M + EXP) << M_POS) | ((_KG + EXP) << KG_POS) | ((_S + EXP) << S_POS) | ((_A + EXP) << A_POS) | ((_K + EXP) << K_POS) | ((_MOL + EXP) << MOL_POS) | ((_CD + EXP) << CD_POS)};
        };

        // Physical quantities likely to be used in the project
        enum Quantity: Code {
            //                                mod,     sr,    rad,      m,     kg,      s,      A,      K,    mol,     cd           unidade SI correspondente
            Acceleration            = SIUnit<DIR,     +0,     +0,     +1,     +0,     -2,     +0,     +0,     +0,     +0>::UNIT, // m/s2
            Angle                   = SIUnit<DIR,     +0,     +1,     +0,     +0,     +0,     +0,     +0,     +0,     +0>::UNIT, // rad
            Angular_Velocity        = SIUnit<DIR,     +0,     +1,     +0,     +0,     -1,     +0,     +0,     +0,     +0>::UNIT, // rad/s
            Area                    = SIUnit<DIR,     +0,     +0,     +2,     +0,     +0,     +0,     +0,     +0,     +0>::UNIT, // m2
            Current                 = SIUnit<DIR,     +0,     +0,     +0,     +0,     +0,     +1,     +0,     +0,     +0>::UNIT, // Ampere
            Electric_Current        = Current,
            Force                   = SIUnit<DIR,     +0,     +0,     +1,     +1,     -2,     +0,     +0,     +0,     +0>::UNIT, // Newton
            Humidity                = SIUnit<DIR,     +0,     +0,     -3,     +1,     +0,     +0,     +0,     +0,     +0>::UNIT, // kg/m3
            Length                  = SIUnit<DIR,     +0,     +0,     +1,     +0,     +0,     +0,     +0,     +0,     +0>::UNIT, // m
            Luminous_Intensity      = SIUnit<DIR,     +0,     +0,     +0,     +0,     +0,     +0,     +0,     +0,     +1>::UNIT, // cd
            Mass                    = SIUnit<DIR,     +0,     +0,     +0,     +1,     +0,     +0,     +0,     +0,     +0>::UNIT, // kg
            Mass                    = SIUnit<DIR,     +0,     +0,     +0,     +1,     +0,     +0,     +0,     +0,     +0>::UNIT, // kg
            Power                   = SIUnit<DIR,     +0,     +0,     +2,     +1,     -3,     +0,     +0,     +0,     +0>::UNIT, // Watt
            Pressure                = SIUnit<DIR,     +0,     +0,     -1,     +1,     -2,     +0,     +0,     +0,     +0>::UNIT, // Pascal
            Velocity                = SIUnit<DIR,     +0,     +0,     +1,     +0,     -1,     +0,     +0,     +0,     +0>::UNIT, // m/s
            Sound_Intensity         = SIUnit<DIR,     +0,     +0,     +0,     +1,     -3,     +0,     +0,     +0,     +0>::UNIT, // W/m2
            Temperature             = SIUnit<DIR,     +0,     +0,     +0,     +0,     +0,     +0,     +1,     +0,     +0>::UNIT, // Kelvin
            Time                    = SIUnit<DIR,     +0,     +0,     +0,     +0,     +1,     +0,     +0,     +0,     +0>::UNIT, // s
            Speed                   = Velocity,
            Volume                  = SIUnit<DIR,     +0,     +0,     +3,     +0,     +0,     +0,     +0,     +0,     +0>::UNIT, // m3
            Voltage                 = SIUnit<DIR,     +0,     +0,     +2,     +1,     -3,     -1,     +0,     +0,     +0>::UNIT, // Volt
            Water_Flow              = SIUnit<DIR,     +0,     +0,     +3,     +0,     -1,     +0,     +0,     +0,     +0>::UNIT, // m3/s
            Frequency               = SIUnit<DIR,     +0,     +0,     +0,     +0,     -1,     +0,     +0,     +0,     +0>::UNIT, // Hz

            Ratio                   = SIUnit<LOG_DIV, -4,     -4,     -4,     -4,     -4,     -4,     -4,     -4,     -4>::UNIT, // não é SI, um valor adimensional (pode ser > 1)
            Percent                 = SIUnit<LOG_DIV, -4,     -4,     -4,     -4,     -4,     -4,     -4,     -4,     -3>::UNIT, // não é SI, um valor adimensional representando uma porcentagem < 1 (1% = 0.01) - pode ser útil para combustível, bateria, etc.
        };

    }__attribute__((packed));
};