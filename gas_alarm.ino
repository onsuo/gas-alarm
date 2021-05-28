#include <stdio.h>
#include <MG811.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// **핀번호 설정 필요
// CO2 Sensor (MG811)
MG811 gas = MG811(A0)
// RGB LED (KY-016)
#define     LED_R            9   // ~
#define     LED_G            10  // ~
#define     LED_B            11  // ~
// LCD (I2C LCD 1602)
LiquidCrystal_I2C LCD(0x27, 16, 2);
// Buzzer (Piezo Buzzer Active)
#define     BUZZER          
// Motor Driver (SZH-MDBL-002)
#define     FAN1            5   // ~
#define     FAN2            6   // ~
// Btn
#define     BTN_LCD       
#define     BTN_STOP      
#define     BTN_MUTE      

int gasDensArray[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0};
int gasLevel = 0;
bool isBtnMuteRelPrev = 1;
bool isMute = 0;
bool isFanOn = 0;
unsigned long tLCDModePrev;
unsigned long tLCDPrintPrev;
unsigned long tLEDPrev;
unsigned long tStopPrev;
unsigned long tBuzzPrev;

float v400 = 4.535;
float v40000 = 3.206;

int checkGasLevel();
bool checkEmergency(int gasLevel);
int checkLCDMode();
void printLCD(int gasLevel,int mute, int LCDMode);
void printRGB(int r, int g, int b);
void printLED(int gasLevel);
bool checkStop();
bool checkMute();
void printBuzz(int gasLevel);
void printFan();

void setup() 
{
    Serial.begin(9600);
    pinMode(gas, INPUT);
    pinMode(LEDr, OUTPUT);
    pinMode(LEDg, OUTPUT);
    pinMode(LEDb, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    pinMode(FAN1, OUTPUT);
    pinMode(FAN2, OUTPUT);
    pinMode(BTN_LCD, INPUT_PULLUP);
    pinMode(BTN_STOP, INPUT_PULLUP);
    pinMode(BTN_MUTE, INPUT_PULLUP);
    gas.begin(v400, v40000);

    tLCDModePrev = millis();
    tLCDPrintPrev = millis();
    tLEDPrev = millis();
    tStopPrev = millis();
    tBuzzPrev = millis();
}

void loop() 
{
    int gasDens = gas.read(); // 가스 농도 ppm 확인
    gasLevel = checkGasLevel(gasDens); // 가스 농도 단계 확인
    bool isEmergency = checkEmergency(gasLevel); // 위급상황 확인
    
    int LCDMode = checkLCDMode(); // LCD 모드 번호 확인
    bool isStop = checkStop(); // 알람 중단 확인, 중단 시간 확인
    isMute = checkMute(); // 무음모드 여부 확인

    printLCD(gasLevel, mute, LCDMode); // LCD 정보 제공
    printLED(gasLevel); // LED 점멸
    if ((isStop == 0 && isMute == 0) || isEmergency == 1) {
        printBuzz(gasLevel); // 가스 농도에 따른 버저 울림
    }
    printFan(); // 위급상황 -> 팬 작동(gasLevel 3 까지)
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
    if (aveGasDens < 650 && gasLevel >= 1) {
        gasLevel = 0;
    }
    else if ((aveGasDens > 750 && gasLevel <= 0) || (aveGasDens < 950 && gasLevel >= 2)) {
        gasLevel = 1;
    }
    else if ((aveGasDens > 1050 && gasLevel <= 1) || (aveGasDens < 1950 && gasLevel >= 3)) {
        gasLevel = 2;
    }
    else if ((aveGasDens > 2050 && gasLevel <= 2) || (aveGasDens < 2950 && gasLevel >= 4)) {
        gasLevel = 3;
    }
    else if (aveGasDens > 3050 && gasLevel <= 3) {
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

int checkLCDMode() {
    unsigned long tLCDMode = millis();
    // 버튼 누름
    if (digitalRead(BTN_LCD) == 0) {
        tLCDModePrev = millis();
        return 1;
    }
    // 2초 경과 후
    else if (tLCDMode - tLCDModePrev >= 2000) {
        return 0;
    }
}

void printLCD(int gasLevel, int mute, int LCDMode) {
    if (LCDMode == 0) {
        unsigned long tLCDPrint = millis();
        if (tLCDPrint - tLCDPrintPrev >= 500) {
            LCD.clear();
            LCD.setCursor(0, 0);
            LCD.print("CO2");
            LCD.setCursor(0, 1);
            LCD.print(gasLevel);
            LCD.print(" ppm");

            tLCDPrintPrev = millis();
        }
    }
    else if (LCDMode == 1) {
        LCD.clear();
        LCD.setCursor(0, 0);
        LCD.print("Mute : ");
        if (mute == 1) {
            LCD.print("ON");
        }
        else if (mute == 0) {
            LCD.print("OFF");
        }
    }
    return;
}

void printRGB(int r, int g, int b) {
    analogWrite(LED_R, r);
    analogWrite(LED_G, g);
    analogWrite(LED_B, b);
}

void printLED(int gasLevel) {
    bool isLEDOn = 0;
    unsigned long tLED = millis();
    unsigned long gap = tLED - tLEDPrev;
    if (gap >= 300 && isLEDOn == 0) {
        isLEDOn = !isLEDOn;
        tLEDPrev = millis();
    }
    else if (gap >= 1000 && isLEDOn == 1) {
        isLEDOn = !isLEDOn;
        tLEDPrev = millis();
    }

    if (isLEDOn == 1) {
        if (gasLevel == 0) {
            printRGB(0, 0, 0);
        }
        else if (gasLevel == 1) {
            printRGB(0, 255, 0);
        }
        else if (gasLevel == 2) {
            printRGB(128, 128, 0);
        }
        else if (gasLevel >= 3) {
            printRGB(255, 0, 0);
        }
    }
    else {
        printRGB(0, 0, 0);
    }
    return;
}

bool checkStop() {
    unsigned long tStop = millis();
    // 버튼 누름
    if (digitalRead(BTN_STOP) == 0) {
        tStopPrev = millis();
        return 1;
    }
    // 2분(120초) 경과 후
    else if (tStop - tStopPrev >= 120000) {
        return 0;
    }
}

bool checkMute() {
    bool isBtnMuteRel = digitalRead(BTN_MUTE)
    if (isBtnMuteRelPrev == 1 && isBtnMuteRel == 0) {
        isMute = !isMute;
        isBtnMuteRelPrev = 0;
    }
    else if (btnMuteCurr == 1) {
        isBtnMuteRelPrev = 1;
    }
    return isMute;
}

void printBuzz(int gasLevel) {
    int BuzzOn = 0;
    unsigned long tBuzz = millis();
    unsigned long gap = tBuzz - tBuzzPrev;
    if (gasLevel == 3) {
        if (gap >= 150 && BuzzOn == 0) {
            BuzzOn = 1;
            tBuzzPrev = millis();
        }
        else if (gap >= 500 && isLEDOn == 1) {
            BuzzOn = 0;
            tBuzz = millis();
        }
    }
    else if (gasLevel == 4) {
        if (gap >= 75 && BuzzOn == 0) {
            BuzzOn = 1;
            tBuzzPrev = millis();
        }
        else if (gap >= 250 && isLEDOn == 1) {
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

void printFan() {
    if (emergency == 1 || (isFanOn == 1 && gasLevel >=3)) {
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
