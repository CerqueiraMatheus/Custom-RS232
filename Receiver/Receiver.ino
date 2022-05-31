#include "Temporizador.h"

// Pinos
#define PIN_TX 13
#define PIN_RTS 12
#define PIN_CTS 11
#define PIN_CLK 8

// Baud
#define BAUD_RATE 1
#define HALF_BAUD 1000 / (2 * BAUD_RATE)

// Controlador de recepção
bool receiving;

// RTS atual
int curRTS;

// Caractere atual
char curChar;
bool evenParity;

// Posição
int charPosition;

// Mostra o clock no pino 8
void showClock(bool b) {
    digitalWrite(PIN_CLK, b);
    delay(50);
}

// Paridade
bool getParity(char c) {
    bool parity = 0;
    while (c) {
        parity = !parity;
        c = c & (c - 1);
    }
    return parity;
}

// Exibe o dado
void printData() {
    // Checa paridade
    bool par = getParity(curChar);

    // Exibe o dado
    Serial.print((evenParity && !par) || (!evenParity && par) ? String(curChar)
                                                              : "*");
}

// Reseta a transmissão
void resetTransmission() {
    receiving = false;
    evenParity = false;
    charPosition = 7;
}

// Interrupção
ISR(TIMER1_COMPA_vect) {
    // Mostra o clock
    showClock(HIGH);

    // Lê o RTS do pino
    if (digitalRead(PIN_RTS)) {
        // Lê o dado
        bool cBit = digitalRead(PIN_TX);

        // Caso seja componente do char
        if (charPosition >= 0) {
            bitWrite(curChar, charPosition--, cBit);
            return;
        }

        // Caso seja componente de paridade
        if (cBit) {
            evenParity = true;
            return;
        }

        return;
    }

    // Pausa o temporizador
    paraTemporizador();

    // Exibe o dado
    printData();

    // Configura o CTS
    digitalWrite(PIN_CTS, LOW);

    // Indica o fim de recepção
    resetTransmission();
}

void setup() {
    // Desabilita interrupcoes
    noInterrupts();

    // Inicializa serial
    Serial.begin(9600);

    // Configura pinos
    pinMode(PIN_TX, INPUT);
    pinMode(PIN_RTS, INPUT);
    pinMode(PIN_CTS, OUTPUT);
    pinMode(PIN_CLK, OUTPUT);

    // Configura timer
    configuraTemporizador(BAUD_RATE);

    // Reseta operadores
    resetTransmission();

    // Habilita interrupcoes
    interrupts();

    // Inicializa o CTS
    digitalWrite(PIN_CTS, LOW);
}
void loop() {
    // Lê o RTS
    curRTS = digitalRead(PIN_RTS);

    if (!receiving && curRTS == HIGH) {
        // Indica o início de recepção
        receiving = true;

        // Configura o CTS
        digitalWrite(PIN_CTS, HIGH);

        delay(0.3 * HALF_BAUD);

        // Inicia o temporizador
        iniciaTemporizador();
    }

    showClock(LOW);
}