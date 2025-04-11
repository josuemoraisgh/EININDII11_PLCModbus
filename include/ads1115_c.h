/**
 * @file ADS1115_c.h
 * @brief Classe para simplificar o uso do ADS1115 com o Adafruit_ADS1X15.
 *
 * Esta classe herda a funcionalidade do Adafruit_ADS1115, fornecendo uma interface simplificada
 * para configuração e leitura de valores analógicos.
 */

#include <Adafruit_ADS1X15.h>

/**
 * @class ADS1115_c
 * @brief Classe para interação simplificada com o ADC ADS1115.
 *
 * Esta classe encapsula o funcionamento do ADS1115, definindo o ganho padrão
 * e oferecendo um método de leitura direta de canais analógicos.
 */
class ADS1115_c : protected Adafruit_ADS1115 {
public:
    /**
     * @brief Construtor padrão.
     *
     * Inicializa a classe base Adafruit_ADS1115.
     */
    ADS1115_c() : Adafruit_ADS1115() {}

    /**
     * @brief Inicializa o dispositivo ADS1115.
     *
     * Define o ganho padrão como GAIN_TWOTHIRDS e inicializa o dispositivo.
     * @return true se o dispositivo foi inicializado com sucesso, false caso contrário.
     */
    bool begin() {
        ((Adafruit_ADS1115 *)this)->setGain(adsGain_t::GAIN_TWOTHIRDS);
        return ((Adafruit_ADS1115 *)this)->begin();
    }

    /**
     * @brief Lê o valor analógico de um canal especificado.
     *
     * @param channel O canal analógico a ser lido (0 a 3).
     * @return Valor analógico lido do canal (16 bits).
     */
    uint16_t analogRead(uint8_t channel) {
        return ((Adafruit_ADS1115 *)this)->readADC_SingleEnded(channel);
    }
};
