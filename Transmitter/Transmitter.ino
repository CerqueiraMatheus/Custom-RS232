#include "Temporizador.h"

// Pinos
#define PIN_RX 13
#define PIN_RTS 12
#define PIN_CTS 11
#define PIN_CLK 8

// Baud
#define BAUD_RATE 1
#define HALF_BAUD 1000 / (2 * BAUD_RATE)

// Controladores de envio
bool sending;
bool completedMessage;

// Caractere atual
char curChar;

// Paridade
bool evenParity;

// Frase
String phrase;

// Posição da frase
int phrasePosition;

// Posição do char
int charPosition;

// CTS atual
bool curCTS;

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

// Configura o char e a paridade
void prepareChar() {
    curChar = phrase.charAt(phrasePosition++);
    evenParity = !getParity(curChar);
}

// Escreve o char no pino
void writeChar() {
    bool b = bitRead(curChar, charPosition--);
    Serial.println(b);
    digitalWrite(PIN_RX, b);
}

// Escreve a paridade no pino
void writeParity() {
    Serial.println("1");
    digitalWrite(PIN_RX, HIGH);
}

// Interrupção
ISR(TIMER1_COMPA_vect) {
    showClock(HIGH);

    if (!completedMessage) {
        // Percorre a string
        if (charPosition != -1) {
            writeChar();
            return;
        }

        // Checa a paridade
        if (evenParity) {
            writeParity();
            completedMessage = true;
            return;
        }

        // Finaliza a mensagem
        completedMessage = true;
    }

    // Finaliza a transmissão
    sending = false;
}

// Recebe a frase
void readInput() {
    Serial.print("Insira a frase: ");
    while (Serial.available() <= 0) {
    }
    phrase = Serial.readString();
    phrase += "\n";
    phrasePosition = 0;
}

// Reseta a transmissão
void resetTransmission() {
    curChar = NULL;
    sending = false;
    evenParity = false;
    completedMessage = false;
    digitalWrite(PIN_RTS, LOW);
}

void setup() {
    // Desabilita interrupcoes
    noInterrupts();

    // Inicializa serial
    Serial.begin(9600);

    // Configura pinos
    pinMode(PIN_RX, OUTPUT);
    pinMode(PIN_RTS, OUTPUT);
    pinMode(PIN_CTS, INPUT);
    pinMode(PIN_CLK, OUTPUT);

    // Configura timer
    configuraTemporizador(BAUD_RATE);

    // Habilita interrupcoes
    interrupts();

    // Reseta
    resetTransmission();

    // Lê a frase
    readInput();
}

void loop() {
    // Lê o CTS
    curCTS = digitalRead(PIN_CTS);

    // Início de char
    if (!sending && !completedMessage && curCTS == LOW &&
        phrasePosition < phrase.length()) {
        // Controle
        Serial.println("Requisitou a transmissao de um char!");

        // Reseta a transmissão do char
        charPosition = 7;

        // Inicializa o RTS
        digitalWrite(PIN_RTS, HIGH);
    }

    // Início de transmissão
    if (!sending && !completedMessage && curCTS == HIGH) {
        // Controle
        Serial.println("Iniciou a transmissao de um char!");

        // Prepara o char
        prepareChar();

        // Indica o início de transmissão
        sending = true;

        // Inicia o temporizador
        iniciaTemporizador();
    }

    // Fim de transmissão
    if (!sending && completedMessage) {
        // Controle
        Serial.println("Completou a transmissao de um char!");

        // Reseta transmissão
        resetTransmission();

        // Pausa o temporizador
        paraTemporizador();

        // Delay para próximo
        delay(HALF_BAUD);
    }

    // Nova frase
    if (!sending && !completedMessage && curCTS == LOW &&
        phrasePosition == phrase.length()) {
        readInput();
    }

    showClock(LOW);
}