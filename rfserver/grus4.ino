#include <SoftwareSerial.h>
#include <OneWire.h>
#include <Wire.h>
#include "ASOLED.h"

#define ONE_WIRE_PIN 2
#define V12_PIN 3
#define NET_PIN 4
#define TX_PIN 5
#define RX_PIN 6
#define LED_PIN 7
#define BAT_PIN A1
#define GSM_OK 0
#define GSM_SM 1
#define GSM_RG 2
#define GSM_CR "\r\n"
#define CTRL_Z '\x1A'
#define ESCAPE '\x1B'
#define BUFF_SIZE 64
#define USERS_COUNT 5

const char* comrade[USERS_COUNT] = {"+79213303129", "+79214201935", "+79219258698", "+79213320218", "+79214060453"};
const uint8_t deviceAddress[8] = {0x28, 0x79, 0xE9, 0x45, 0x92, 0x15, 0x02, 0x90};
const uint16_t temp_null = 1584;  // 1584 = 99 * 16  = 0x063E
const char lineFeed = '\n';

unsigned long measureTime;
char strSQ[8] = {'\0','\0','\0','\0', '\0','\0','\0','\0'};
int no220 = LOW;
int isNet = LOW;

SoftwareSerial modem(RX_PIN, TX_PIN);
OneWire ds(ONE_WIRE_PIN);

void getSignalQuality() {
  // запросить у GSM модема уровень сигнала и его значение разместить в strSQ
  bool isResponse = false;
  char buff[BUFF_SIZE];
  for (byte j = 0; j < BUFF_SIZE; j++) {
    buff[j] = '\0';
  }
  unsigned long timeout = millis() + 1000L;
  while (modem.available() > 0 && millis() < timeout) {
    modem.read();
  }
  modem.write("AT+CSQ");
  modem.write(GSM_CR);
  delay(100); 
  do {
    if (modem.available()) {
      byte len = modem.readBytesUntil(lineFeed, buff, BUFF_SIZE);
      if (len > 8 && buff[0] == '+' && buff[1] == 'C' && buff[2] == 'S' && buff[3] == 'Q') {
        isResponse = true;
        break;
      }
    }
  } while (millis() < timeout);
  sprintf(strSQ, "SQ %c%c", isResponse ? buff[6] : '?', isResponse ? buff[7] : '?');
}

void printTemp(bool sms) {
  // запросить у датчика ds18b20 значение температуры и отправить 
  // в GSM модем (sms == true) или на дисплей (sms == false)
  char str[32];
  uint8_t scratchPad[9];
  uint16_t currentTemperature;
	ds.reset();     // Initialization
  ds.write(0xCC); // Command Skip ROM to address all devices
	ds.write(0x44); // Command Convert T, Initiates temperature conversion
  digitalWrite(LED_PIN, HIGH); 
  delay(1000);    // maybe 750ms is enough, maybe not
  digitalWrite(LED_PIN, LOW);
  ds.reset();
  ds.select(deviceAddress); // Select a device based on its address
  ds.write(0xBE);  // Read Scratchpad, temperature: byte 0: LSB, byte 1: MSB
  scratchPad[0] = 0x3E; // temp_null LSB
  scratchPad[1] = 0x06; // temp_null MSB
  scratchPad[8] = 0x00; // CRC
  for (uint8_t i = 0; i < 9; i++) {
    scratchPad[i] = ds.read();
  }
  if (ds.crc8(scratchPad, 8) == scratchPad[8]) {
    currentTemperature = (scratchPad[1] << 8) | scratchPad[0];
  } else {
    currentTemperature = temp_null;  // 99,9 * 16
  }
  sprintf(str, "1 %2d", (int) floor(currentTemperature/16));
  if (sms) {
    // в текст SMS сообщения
    modem.write(str);
    modem.write(lineFeed);
  } else {
    // на дисплей прибора
    LD.clearDisplay();
    LD.printString_12x16(str, 0, 0);
  }
}

void printOtherInformation(bool sms) {
  // вывод информации в GSM модем (sms == true) или на дисплей (sms == false)
  char str[32];

  // power supply
  char str220high[] = "Power BATTERY";
  char str220low[] = "Power ~220v";
  if (sms) {
    if (no220 == HIGH) modem.write(str220high);
    else modem.write(str220low);
    modem.write(lineFeed);
  } else {
    if (no220 == HIGH) LD.printString_6x8(str220high, 0, 5); 
    else LD.printString_6x8(str220low, 0, 5); 
  }
  
  // Battery voltage
  float bat_float = (float) analogRead(BAT_PIN) * 0.03557;
  uint8_t bat_int = bat_float; // get the integer part
  uint8_t bat_frac = (bat_float - bat_int) * 10; // get the fractional part
  sprintf(str, "Bat %1d.%1dv", bat_int, bat_frac);
  if (sms) {
    modem.write(str);
    modem.write(lineFeed);
  } else {
    LD.printString_6x8(str, 68, 7); 
  }
  
  // Signal Quality
  if (sms) {
    modem.write(strSQ);
    modem.write(lineFeed);
  } else {
    LD.printString_6x8(strSQ, 68, 6); 
  }

  // Is Submit Information to the Internet
  int isNet = digitalRead(NET_PIN);
  sprintf(str, "Net %s", (isNet == HIGH) ? "ON" : "Off");
  if (sms) {
    modem.write(str);
    modem.write(lineFeed);
  } else {
    LD.printString_6x8(str, 0, 7); 
  }

  // Thermostat Relay
  bool isRelay = false;
  sprintf(str, "Relay %s", isRelay ? "ON" : "Off");
  if (sms) modem.write(str);
  else LD.printString_6x8(str, 0, 6); 
}

/*
 * выдача модему команды и контроль её исполнения
 * command команда
 * mode    ожидаемый ответ
 * retry   количество запросов
 * waitok  количество подряд верных ответов
 */
bool performModem(const char* command, byte mode, byte retry, byte waitok) {
  char buff[BUFF_SIZE];
  char creg1, creg2, creg3;
  byte countok = 0;
  bool resOK = false;
  unsigned long timeout = millis() + 1000L;
  while (modem.available() > 0 && millis() < timeout) {
    modem.read();
  }
  for (byte i = 0; i < retry; i++) {
    modem.write(command); 
    modem.write(GSM_CR);
    resOK = false;
    timeout = millis() + 2000L; // ждём ответа 2 сек
    do {
      if (modem.available()) {
        for (byte j = 0; j < BUFF_SIZE; j++) {
          buff[j] = '\0';
        }
        byte len = modem.readBytesUntil(lineFeed, buff, BUFF_SIZE);
        if (len > 1) {
          switch (mode) {
            case GSM_OK:
              resOK = resOK || (buff[0] == 'O' && buff[1] == 'K');
              break;
            case GSM_SM:
              resOK = resOK || (buff[0] == '>');
              break;
            case GSM_RG:
              // Ждём ответа +CREG: 1,1
              if (len > 9 && buff[0] == '+' && buff[1] == 'C' && buff[2] == 'R' && buff[3] == 'E' && buff[4] == 'G') {
                creg1 = buff[7]; 
                creg2 = buff[9]; 
                creg3 = (len > 10 && buff[10] > 32) ? buff[10] : ' ';
                resOK = resOK || (creg2 == '1' && creg3 == ' ');
              }
              break;
            default:
              resOK = false;
          }
        }
      }
    } while (!resOK && millis() < timeout);
    if (resOK) countok++; else countok = 0;
    if (countok >= waitok) break;
  }
  return resOK && countok >= waitok;
}

void connectModem() {
  digitalWrite(LED_PIN, HIGH);
  delay(9000); 
  modem.begin(9600L);
  delay(5000);
  while (!performModem("AT", GSM_OK, 20, 5)) {} 
  while (!performModem("AT+CREG?", GSM_RG, 1, 1)) {} 
  while (!performModem("ATE0", GSM_OK, 1, 1)) {}            // Set echo mode off 
  while (!performModem("AT+CLIP=1", GSM_OK, 1, 1)) {}       // Set caller ID on
  while (!performModem("AT+CMGF=1", GSM_OK, 1, 1)) {}       // Set SMS to text mode
  while (!performModem("AT+CSCS=\"GSM\"", GSM_OK, 1, 1)) {} // Character set of the mobile equipment
}

void isCall() {
  // при входящем вызове модем выдает несколько раз в секунду строку
  // RING
  // после первой строки RING модем выдаст однократно строку типа 
  // +CLIP: "7XXXXXXXXXX",145,"",,"",0 
  // а если послать запрос AT+CLCC то модем выдаст строку типа
  // +CLCC: 1,1,4,0,0,"7XXXXXXXXXX",145
  unsigned long timeLed = millis(); 
  unsigned long timeout = millis() + 10000L; 
  byte len;
  bool allow;
  bool isNumber = false;
  char str[32];
  char number[13] = {'\0','\0','\0','\0', '\0','\0','\0','\0', '\0','\0','\0','\0', '\0'};
  char buff[BUFF_SIZE];
  for (byte k = 0; k < BUFF_SIZE; k++) {
    buff[k] = '\0';
  }
  while (millis() < timeout && !isNumber) {
    if (modem.available()) {
      len = modem.readBytesUntil(lineFeed, buff, BUFF_SIZE);
      if (len > 20 && buff[1] == 'C' && buff[2] == 'L' && buff[3] == 'I' && buff[4] == 'P') {
        buff[20] = '\0';
        for (byte n = 0; n < 12; n++) {
          number[n] = buff[8+n];
        }
        isNumber = true;
        break;
      }
    }
    int isNet = digitalRead(NET_PIN);
    if (isNet == HIGH && millis() > timeLed) {
      timeLed += 1000;
      digitalWrite(LED_PIN, HIGH); 
      delay(1); 
      digitalWrite(LED_PIN, LOW);
    }
  };

  if (number[0] != '\0') {
    LD.printString_6x8(number, 0, 4); 
    delay(7000);
    performModem("ATH", GSM_OK, 1, 1);
    for (byte i = 0; i < USERS_COUNT; i++ ) {
      allow = true;
      for (byte j = 0; j < 12; j++) {
        allow = allow && number[j] == comrade[i][j];
      }
      if (allow) {
        delay(7000);
        sprintf(str, "AT+CMGS=\"%s\"", comrade[i]);
        if (performModem(str, GSM_SM, 1, 1)) {
          printTemp(true);
          printOtherInformation(true);
          modem.write(CTRL_Z); 
        } else {
          modem.write(ESCAPE); 
        }
        modem.write(GSM_CR); 
        delay(7000);
        // удалить все СМС
        performModem("AT+CMGD=1,4", GSM_OK, 1, 1);
        break;
      }
    }
  }

}

void energySourceSMS() {
  // сообщение при переключении источника питания на резервный
  char str[32];
  delay(1000);
  for (byte i = 0; i < 2 && i < USERS_COUNT; i++ ) {
    sprintf(str, "AT+CMGS=\"%s\"", comrade[i]);
    if (performModem(str, GSM_SM, 1, 1)) {
      modem.println(no220 == LOW ? "restored 220V" : "disconnected 220V");
      printTemp(true);
      printOtherInformation(true);
      modem.write(CTRL_Z); 
    } else {
      modem.write(ESCAPE); 
    }
    modem.write(GSM_CR); 
    delay(7000);
  }
}

void reportTCP() {
  // передать информацию на TCP-сервер 
  LD.printString_6x8("start TCP", 0, 4); 
  modem.println("AT+CIPSTART=\"UDP\",\"109.236.103.6\",3333"); 
  delay(7000);
  modem.println("AT+CIPSEND");
  delay(2000);
  modem.print("Grus 4\n");
  printTemp(true);
  printOtherInformation(true);
  delay(2000);
  modem.println((char)26); // ctrl-Z
  delay(7000); // waitting for reply, important!
  modem.println("AT+CIPCLOSE");
  delay(2000);
  modem.println("AT+CIPSHUT");
  LD.printString_6x8("end", 60, 4); 
  delay(2000);
}

// ==================== main =======================

void setup()
{
  delay(1000);
  LD.init();
  LD.clearDisplay();
  LD.setNormalDisplay();
  LD.SetTurnedOrientation();
  LD.printString_12x16("Hello,", 20, 2);
  LD.printString_12x16("world!", 40, 4);
  delay(1000);
  connectModem();
  delay(1000);
  pinMode(LED_PIN, OUTPUT);
  pinMode(V12_PIN, INPUT);
  pinMode(NET_PIN, INPUT);
  measureTime = millis();
}

void loop()
{
  if (!performModem("AT", GSM_OK, 1, 1)) {
    connectModem(); // перезагрузка если GSM модем не отвечает
  } 
  getSignalQuality();
  isNet = digitalRead(NET_PIN);
  int no220new = digitalRead(V12_PIN);
  // если аварийное отключение 220 В - отправить SMS
  if (no220new != no220) {
    no220 = no220new;
    energySourceSMS();
    reportTCP();
  }
  // информацию на дисплей
  printTemp(false);
  printOtherInformation(false);
  // ожидание входного звонка
  isCall();
  // раз в 4 часа передать информацию на TCP-сервер
  if (millis() > measureTime) {
    measureTime += 14400000L;
    if (isNet == HIGH) {
      reportTCP();
    }
  }
}
