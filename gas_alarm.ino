#include <stdio.h>
#include <MG811.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// **핀번호 설정 필요
// CO2 Sensor (MG811)
MG811 gas = MG811(A0);
// RGB LED (KY-016)
#define     LEDr            9   // ~
#define     LEDg            10  // ~
#define     LEDb            11  // ~
// LCD (I2C LCD 1602)
LiquidCrystal_I2C LCD(0x27, 16, 2);
// Buzzer (Piezo Buzzer Active)
#define     buzzer          
// Motor Driver (SZH-MDBL-002)
#define     fan1            5   // ~
#define     fan2            6   // ~
// Button
#define     buttonLCD       
#define     buttonStop      
#define     buttonMute      

int gasDensArray[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0};
int gasLevel = 0;
int countMute = 0;
int fanOn = 0;
unsigned long timerLCDPrev;
unsigned long timerLEDPrev;
unsigned long timerStopPrev;
unsigned long timerBuzzPrev;

float v400 = 4.535;
float v40000 = 3.206;

void checkGasLevel();
int checkEmergency(int gasLevel);
int checkLCDMode();
void printLCD(int gasLevel,int mute, int LCDMode);
void printRGB(int r, int g, int b);
void printLED(int gasLevel);
int checkStop();
int checkMute();
void printBuzz(int gasLevel);
void printFan();

void setup() 
{
    Serial.begin(9600);
    pinMode(gas, INPUT);
    pinMode(LEDr, OUTPUT);
    pinMode(LEDg, OUTPUT);
    pinMode(LEDb, OUTPUT);
    pinMode(buzzer, OUTPUT);
    pinMode(fan, OUTPUT);
    pinMode(buttonLCD, INPUT_PULLUP);
    pinMode(buttonStop, INPUT_PULLUP);
    pinMode(buttonMute, INPUT_PULLUP);
    gas.begin(v400, v40000);

    timerLCDPrev = millis();
    timerLEDPrev = millis();
    timerStopPrev = millis();
    timerBuzzPrev = millis();
}

void loop() 
{
    int gasDens = gas.read(); // 가스 농도 ppm 확인
    checkGasLevel(gasDens); // 가스 농도 단계 확인
    int emergency = checkEmergency(gasLevel); // 위급상황 확인
    
    int LCDMode = checkLCDMode(); // LCD 모드 번호 확인
    int stop = checkStop(); // 알람 중단 확인, 중단 시간 확인
    int mute = checkMute(); // 무음모드 여부 확인

    printLCD(gasLevel, mute, LCDMode); // LCD 정보 제공
    printLED(gasLevel); // LED 점멸
    if ((stop == 0 && mute == 0) || emergency == 1) {
        printBuzz(gasLevel); // 가스 농도에 따른 버저 울림
    }
    printFan(); // 위급상황 -> 팬 작동(gasLevel 3 까지)
}

void checkGasLevel(float gasDens) {
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
    return;
}

int checkEmergency(int gasLevel) {
    if (gasLevel == 4) {
        return 1;
    }
    return 0;
}

int checkLCDMode() {
    unsigned long timerLCD = millis();
    // 버튼 누름
    if (digitalRead(buttonLCD) == 0) {
        timerLCDPrev = millis();
        return 1;
    }
    // 2초 경과 후
    else if (timerLCD - timerLCDPrev >= 2000) {
        return 0;
    }
}

void printLCD(int gasLevel, int mute, int LCDMode) {
    LCD.clear();
    if (LCDMode == 0) {
        LCD.setCursor(0, 0);
        LCD.print("CO2");
        LCD.setCursor(0, 1);
        LCD.print(gasLevel);
        LCD.print(" ppm");
    }
    else if (LCDMode == 1) {
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
    analogWrite(LEDr, r);
    analogWrite(LEDg, g);
    analogWrite(LEDb, b);
}

void printLED(int gasLevel) {
    int LEDOn = 0;
    unsigned long timerLED = millis();
    unsigned long gap = timerLED - timerLEDPrev;
    if (gap >= 300 && LEDOn == 0) {
        LEDOn = 1;
        timerLEDPrev = millis();
    }
    else if (gap >= 1000 && LEDOn == 1) {
        LEDOn = 0;
        timerLEDPrev = millis();
    }

    if (LEDOn == 1) {
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

int checkStop() {
    unsigned long timerStop = millis();
    // 버튼 누름
    if (digitalRead(buttonStop) == 0) {
        timerStopPrev = millis();
        return 1;
    }
    // 2분(120초) 경과 후
    else if (timerStop - timerStopPrev >= 120000) {
        return 0;
    }
}

int checkMute() {
    int buttonMuteCurr = digitalRead(buttonMute)
    if (buttonMutePrev == 1 && buttonMuteCurr == 0) {
        countMute++;
        buttonMutePrev = 0;
    }
    else if (buttonMuteCurr == 1) {
        buttonMutePrev = 1;
    }
    return countMute % 2;
}

void printBuzz(int gasLevel) {
    int BuzzOn = 0;
    unsigned long timerBuzz = millis();
    unsigned long gap = timerBuzz - timerBuzzPrev;
    if (gasLevel == 3) {
        if (gap >= 150 && BuzzOn == 0) {
            BuzzOn = 1;
            timerBuzzPrev = millis();
        }
        else if (gap >= 500 && LEDOn == 1) {
            BuzzOn = 0;
            timerBuzz = millis();
        }
    }
    else if (gasLevel == 4) {
        if (gap >= 75 && BuzzOn == 0) {
            BuzzOn = 1;
            timerBuzzPrev = millis();
        }
        else if (gap >= 250 && LEDOn == 1) {
            BuzzOn = 0;
            timerBuzz = millis();
        }
    }

    if (BuzzOn == 1) {
        digitalWrite(buzzer, 1);
    }
    else if (BuzzOn == 0) {
        digitalWrite(buzzer, 0);
    }
    return;
}

void printFan() {
    if (emergency == 1 || (fanOn == 1 && gasLevel >=3)) {
        fanOn = 1;
    }
    else if (gasLevel < 3) {
        fanOn = 0;
    }

    if (fanOn == 1) {
        digitalWrite(fan1, 1);
        digitalWrite(fan2, 0);
    }
    else if (fanOn == 0) {
        digitalWrite(fan1, 0);
        digitalWrite(fan1, 0);
    }
    return;
}
