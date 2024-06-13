#include "Arduino.h"
#include "pins.h"

#include "SPI.h"
#include <MFRC522.h>

#include "Wire.h"
#include "GyverOLED.h"

#include "Map.hpp"
#include "states.h"

#include "EncButton.h"

#define moneyPerTick 25

EncButton eb(5, 6, 7);

//arduino::MbedSPI spi(PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_SCK);
MFRC522 rfid(SS_PIN, RST_PIN);
GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> display;

Map<byte, int> money;
int current_state = SHOW_BALANCE;

byte *getUID();

void add_money(byte *uid, int amount);

void sub_money(byte *uid, int amount);

int get_money(byte *uid);

void display_balance(byte *uidByte);

int get_amount();

void init_pins() {
    pinMode(show_balance_btn, INPUT);
    pinMode(add_money_btn, INPUT);
    pinMode(sub_money_btn, INPUT);
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    SPI.begin();
    rfid.PCD_Init();
    display.init();
    init_pins();
    eb.counter = 0;

    display.clear();
    display.update();
    Serial.println("Tap RFID/NFC Tag on reader");
}

void loop() {
    while (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        if (digitalRead(show_balance_btn) == HIGH) {
            current_state = SHOW_BALANCE;
        } else if (digitalRead(add_money_btn) == HIGH) {
            current_state = ADD_MONEY;
        } else if (digitalRead(sub_money_btn) == HIGH) {
            current_state = SUB_MONEY;
        }
    }
    byte *uidByte = getUID();

    if (current_state == SHOW_BALANCE) {
        display_balance(uidByte);
    } else if (current_state == ADD_MONEY) {
        display_balance(uidByte);
        display.println("ADD MONEY");
        int amount = get_amount();
//        Serial.print("Amount: " + String(amount));
        add_money(uidByte, amount);
        display_balance(uidByte);
    } else if (current_state == SUB_MONEY) {
        display_balance(uidByte);
        display.println("SUB MONEY");
        int amount = get_amount();
//        Serial.print("Amount: " + String(amount));
        sub_money(uidByte, amount);
        display_balance(uidByte);
    }

    delay(100);
}

int get_amount() {
    eb.counter = 0;
    while (true) {
        eb.tick();
        if (eb.turn()) {
            display.setCursor(0, 3);
            display.print("Amount: " + String(abs(eb.counter * moneyPerTick)) + " RUB");
        } else if (eb.click()) return abs(eb.counter * moneyPerTick);

    }
}

void display_balance(byte *uidByte) {
    display.clear();
    display.setCursor(0, 0);
    display.print("UID:");
    Serial.print("UID: ");
    if (uidByte != nullptr) {
        for (byte i = 0; i < 4; i++) {
            Serial.print(uidByte[i] < 0x10 ? "0" : "");
            Serial.print(uidByte[i], HEX);
            display.print(uidByte[i] < 0x10 ? "0" : "");
            display.print(uidByte[i], HEX);
        }
        Serial.println();
        display.println();
        display.println("Money:" + String(get_money(uidByte)) + " RUB");
        display.update();
    }
}


byte *getUID() {
    byte *uidByte = rfid.uid.uidByte;
    rfid.PICC_HaltA(); // halt PICC
    rfid.PCD_StopCrypto1(); // stop encryption on PCD
    return uidByte;
}

void add_money(byte *uid, int amount) {
    money[*uid] += amount;
}

void sub_money(byte *uid, int amount) {
    money[*uid] -= amount;
}

int get_money(byte *uid) {
    int amount = money[*uid];
    Serial.println("Money: " + String(amount));
    return amount;
}
