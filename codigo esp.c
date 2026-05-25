#include <Arduino.h>
#include "driver/twai.h"

#define CAN_TX_PIN GPIO_NUM_5  
#define CAN_RX_PIN GPIO_NUM_4  

// Daly BMS usa 250 kbps por estándar
#define CAN_BAUD_RATE TWAI_TIMING_CONFIG_250KBITS() 

void setup_twai() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = CAN_BAUD_RATE;
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); 

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        Serial.println("Driver TWAI instalado a 250 kbps.");
    }
    if (twai_start() == ESP_OK) {
        Serial.println("Escuchando protocolo DALY BMS...");
    }
}

// Función para solicitar datos (Maestro -> Esclavo)
void request_daly_data(uint8_t data_id) {
    twai_message_t req_msg;
    req_msg.extd = 1; // Usamos identificador extendido (29 bits)
    req_msg.rtr = 0;
    req_msg.data_length_code = 8;
    
    // Formato ID Request: Prioridad (0x18) + Data ID + BMS (0x01) + PC (0x40)
    req_msg.identifier = 0x18000140 | (data_id << 16);

    // Payload de 8 bytes reservados (en cero) para enviar el comando
    for (int i = 0; i < 8; i++) {
        req_msg.data[i] = 0x00;
    }

    twai_transmit(&req_msg, pdMS_TO_TICKS(10));
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    setup_twai();
}

unsigned long last_request = 0;
float ultimo_voltaje_valido = 0.0;

void loop() {
    // Solicitamos el comando 0x90 cada segundo
    if (millis() - last_request > 1000) {
        request_daly_data(0x90); 
        last_request = millis();
    }

    twai_message_t message;
    
    if (twai_receive(&message, pdMS_TO_TICKS(10)) == ESP_OK) {
        
        // Verificamos que sea una trama extendida
        if (message.flags & TWAI_MSG_FLAG_EXTD) {
            
            // ID Respuesta para 0x90: Prioridad (0x18) + Data ID (0x90) + PC (0x40) + BMS (0x01) = 0x18904001
            if (message.identifier == 0x18904001) {
                
                // Daly usa Big-Endian (el byte de la izquierda es el más significativo)
                
                // Byte 0 y 1: Voltaje Total (Resolución 0.1V)
                float voltaje = (message.data[0] << 8 | message.data[1]) / 10.0;
                
                // Byte 4 y 5: Corriente (Offset 30000, Resolución 0.1A)
                float corriente = ((message.data[4] << 8 | message.data[5]) - 30000) / 10.0;
                
                // Byte 6 y 7: SOC (Resolución 0.1%)
                float soc = (message.data[6] << 8 | message.data[7]) / 10.0;
                
                // Filtro para prevenir saltos bruscos en el empaquetado de datos 
                // Ignora el dato si cae repentinamente a cero o si salta más de 10V respecto a la lectura anterior
                if (ultimo_voltaje_valido == 0.0 || abs(voltaje - ultimo_voltaje_valido) < 10.0) {
                    ultimo_voltaje_valido = voltaje;
                    Serial.printf("⚡ BATERIA -> V: %.1fV | I: %.1fA | SOC: %.1f%%\n", voltaje, corriente, soc);
                } else {
                    Serial.println("⚠️ Anomalía detectada en el bus: Salto brusco de voltaje ignorado.");
                }
            }
        }
    }
}