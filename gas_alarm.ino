#include <stdio.h>
#include <MG811.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/*
    핀번호 설정 필요
    적혀있는 핀 번호는 기본 설정값임
*/
// CO2 Sensor (MG811)
MG811 gas = MG811(A0);
// RGB LED (KY-016)
#define     LED_R           9   // ~
#define     LED_G           10  // ~
#define     LED_B           11  // ~
// LCD (I2C LCD 1602)
LiquidCrystal_I2C LCD(0x27, 16, 2);
// Buzzer (Piezo Buzzer Active)
#define     BUZZER          2
// Motor Driver (SZH-MDBL-002)
#define     FAN1            5   // ~
#define     FAN2            6   // ~
// Button
#define     BTN_LCD         4
#define     BTN_STOP        7
#define     BTN_MUTE        8

#define threshold1 700
#define threshold2 1000
#define threshold3 2000
#define threshold4 3000
#define buffer 50

int gasDensArray[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 };
int gasLevel = 0;
bool isLEDOn = 0;
bool isBtnMuteRel = 1;
bool isMute = 0;
bool isFanOn = 0;
unsigned long tLCDMode;
unsigned long tLCDPrint;
unsigned long tLED;
unsigned long tBuzzStop;
unsigned long tBuzz;

float v400 = 4.535;
float v40000 = 3.206;

int checkGasLevel();
bool checkEmergency(int gasLevel);
void printLCD(int gasLevel, int gasDens, int isMute);
void printRGB(int r, int g, int b);
void printLED(int gasLevel);
bool checkStop();
bool checkMute();
void manageBuzz(int gasLevel);
void manageFan(bool isEmergency);

void setup() 
{
    Serial.begin(9600);
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    pinMode(FAN1, OUTPUT);
    pinMode(FAN2, OUTPUT);
    pinMode(BTN_LCD, INPUT_PULLUP);
    pinMode(BTN_STOP, INPUT_PULLUP);
    pinMode(BTN_MUTE, INPUT_PULLUP);
    gas.begin(v400, v40000);
    gas.calibrate();
    LCD.begin();
    LCD.backlight();

    tLCDMode = millis();
    tLCDPrint = millis();
    tLED = millis();
    tBuzzStop = millis();
    tBuzz = millis();
}

void loop() 
{
    int gasDens = gas.read(); // 가스 농도 ppm 확인
    gasLevel = checkGasLevel(gasDens); // 가스 농도 단계 확인
    bool isEmergency = checkEmergency(gasLevel); // 위급상황 확인
    
    bool isStop = checkStop(); // 알람 중단 확인, 중단 시간 확인
    isMute = checkMute(); // 무음모드 여부 확인

    printLCD(gasLevel, gasDens, isMute); // LCD 정보 제공
    printLED(gasLevel); // LED 점멸
    if ((isStop == 0 && isMute == 0) || isEmergency == 1) {
        manageBuzz(gasLevel); // 가스 농도에 따른 버저 울림
    }
    manageFan(isEmergency); // 위급상황 -> 팬 작동(gasLevel 3 까지)
}

int checkGasLevel(float gasDens) {
    // averaging
    for (int i = 0; i < 10 - 1; i++) { // 저장된 값을 한칸씩 앞으로 당김
        gasDensArray[i] = gasDensArray[i + 1];
    }
    gasDensArray[9] = gasDens; // 현재 기체 ppm 저장

    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum = sum + gasDensArray[i];
    }
    float aveGasDens = (float)sum / 10; // loop 10번 이상 돌아야 정상 작동
    
    // hysteresis : 경계 문턱 2개 -> 상 문턱 : 알림 단계 높임 / 하 문턱 : 알림 단계 낮춤
    if (aveGasDens < threshold1 - buffer && gasLevel >= 1) {
        gasLevel = 0;
    }
    else if ((aveGasDens > threshold1 + buffer && gasLevel <= 0) || (aveGasDens < threshold2 - buffer && gasLevel >= 2)) {
        gasLevel = 1;
    }
    else if ((aveGasDens > threshold2 + buffer && gasLevel <= 1) || (aveGasDens < threshold3 - buffer && gasLevel >= 3)) {
        gasLevel = 2;
    }
    else if ((aveGasDens > threshold3 + buffer && gasLevel <= 2) || (aveGasDens < threshold4 - buffer && gasLevel >= 4)) {
        gasLevel = 3;
    }
    else if (aveGasDens > threshold4 + buffer && gasLevel <= 3) {
        gasLevel = 4;
    }
    return gasLevel;
}

bool checkEmergency(int gasLevel) {
    if (gasLevel == 4) {
        return 1;
    }
    return 0;
}

void printLCD(int gasLevel, int gasDens, int isMute) {
    unsigned long temptLCDPrint = millis();
    if (temptLCDPrint - tLCDPrint >= 500) {
        LCD.clear();
        LCD.setCursor(0, 0);
        LCD.print("CO2");
        LCD.setCursor(4, 0);
        LCD.print("(Level: ");
        LCD.setCursor(12, 0);
        LCD.print(gasLevel);
        LCD.setCursor(13, 0);
        LCD.print(")");
        LCD.setCursor(0, 1);
        LCD.print(gasDens);
        LCD.setCursor(6, 1);
        LCD.print("ppm");
        tLCDPrint = millis();
    }

    if (isMute == 1) {
        LCD.setCursor(13, 1);
        LCD.print("*M*");
    }
    else if (isMute == 0) {
        LCD.setCursor(13, 1);
        LCD.print("   ");
    }


    return;
}

void printRGB(int r, int g, int b) {
    analogWrite(LED_R, r);
    analogWrite(LED_G, g);
    analogWrite(LED_B, b);
}

void printLED(int gasLevel) {
    unsigned long temptLED = millis();
    unsigned long gap = temptLED - tLED;
    /*
        gap 기준을 조절하여 LED 점멸 주기 조절 가능
    */
    if (gap >= 300 && isLEDOn == 0) {
        isLEDOn = !isLEDOn;
        tLED = millis();
    }
    else if (gap >= 1000 && isLEDOn == 1) {
        isLEDOn = !isLEDOn;
        tLED = millis();
    }

    if (isLEDOn == 1) {
        if (gasLevel == 0) {
            printRGB(0, 0, 0);
        }
        else if (gasLevel == 1) {
            printRGB(0, 128, 0);
        }
        else if (gasLevel == 2) {
            printRGB(64, 164, 0);
        }
        else if (gasLevel >= 3) {
            printRGB(128, 0, 0);
        }
    }
    else {
        printRGB(0, 0, 0);
    }
    return;
}

bool checkStop() {
    unsigned long temptBuzzStop = millis();
    // 버튼 누름
    if (digitalRead(BTN_STOP) == 0) {
        tBuzzStop = millis();
        return 1;
    }
    // 2분(120초) 경과 후
    else if (temptBuzzStop - tBuzzStop >= 120000) {
        return 0;
    }
}

bool checkMute() {
    bool tempisBtnMuteRel = digitalRead(BTN_MUTE);
    if (isBtnMuteRel == 1 && tempisBtnMuteRel == 0) {
        isMute = !isMute;
        isBtnMuteRel = tempisBtnMuteRel;
    }
    else if (tempisBtnMuteRel == 1) {
        isBtnMuteRel = tempisBtnMuteRel;
    }
    return isMute;
}

void manageBuzz(int gasLevel) {
    int BuzzOn = 0;
    unsigned long temptBuzz = millis();
    unsigned long gap = temptBuzz - tBuzz;
    /*
        gap 기준을 조절하여 버저 점멸 주기 조절 가능
    */
    if (gasLevel == 3) {
        if (gap >= 150 && BuzzOn == 0) {
            BuzzOn = 1;
            tBuzz = millis();
        }
        else if (gap >= 500 && BuzzOn == 1) {
            BuzzOn = 0;
            tBuzz = millis();
        }
    }
    else if (gasLevel == 4) {
        if (gap >= 75 && BuzzOn == 0) {
            BuzzOn = 1;
            tBuzz = millis();
        }
        else if (gap >= 250 && BuzzOn == 1) {
            BuzzOn = 0;
            tBuzz = millis();
        }
    }

    if (BuzzOn == 1) {
        digitalWrite(BUZZER, 1);
    }
    else if (BuzzOn == 0) {
        digitalWrite(BUZZER, 0);
    }
    return;
}

void manageFan(bool isEmergency) {
    if (isEmergency == 1 || (isFanOn == 1 && gasLevel >=3)) {
        isFanOn = 1;
    }
    else if (gasLevel < 3) {
        isFanOn = 0;
    }

    if (isFanOn == 1) {
        digitalWrite(FAN1, 1);
        digitalWrite(FAN2, 0);
    }
    else if (isFanOn == 0) {
        digitalWrite(FAN1, 0);
        digitalWrite(FAN2, 0);
    }
    return;
}
