// LED 깜빡임

unsigned long rtimer = 0;
unsigned long rtimer_prev = 0;
unsigned long ytimer = 0;
unsigned long ytimer_prev = 0;
unsigned long gtimer = 0;
unsigned long gtimer_prev = 0;
int LEDr = 3;
int LEDy = 4;
int LEDg = 5;
int LEDr_state = 0;
int LEDy_state = 0;
int LEDg_state = 0;
int button_prev = 1;
int button_count = 0;
int LED_mode = button_count % 3;

unsigned long getTimeGap();
void LED_onoff(int pinNum, int onoff);
void LED(int pinNum, unsigned long on, unsigned long off);



void setup() {
	Serial.begin(9600);
  	pinMode(12, INPUT_PULLUP);
	pinMode(LEDr, OUTPUT);
	pinMode(LEDy, OUTPUT); 
	pinMode(LEDg, OUTPUT); 
	rtimer_prev = millis();
	ytimer_prev = millis();
	gtimer_prev = millis();
} 

void loop() {
	if (button_prev == 1 && digitalRead(12) == 0) {
		button_count++;
		button_prev = 0;
	}
	LED(LED_mode + 3, 100, 100);
}



void LED_onoff(int pinNum, int onoff) {
	if (pinNum == LEDr) {
		if (onoff == 1) {
			digitalWrite(pinNum, 1);
			LEDr_state = 1;
		}
		else if (onoff == 0) {
			digitalWrite(pinNum, 0);
			LEDr_state = 0;
		}
	}
	if (pinNum == LEDy) {
		if (onoff == 1) {
			digitalWrite(pinNum, 1);
			LEDy_state = 1;
		}
		else if (onoff == 0) {
			digitalWrite(pinNum, 0);
			LEDy_state = 0;
		}
	}
	if (pinNum == LEDg) {
		if (onoff == 1) {
			digitalWrite(pinNum, 1);
			LEDg_state = 1;
		}
		else if (onoff == 0) {
			digitalWrite(pinNum, 0);
			LEDg_state = 0;
		}
	}
}

void LED(int pinNum, unsigned long on, unsigned long off) {
	if (pinNum == LEDr) {
      	rtimer = millis();
      	unsigned long timeGap = rtimer - rtimer_prev;
		if (LEDr_state = 0 && timeGap >= off) {
			LED_onoff(pinNum, 1);
			rtimer_prev = millis();
		}
		else if (LEDr_state = 1 && timeGap >= on) {
			LED_onoff(pinNum, 0);
			rtimer_prev = millis();
		}
	}
	if (pinNum == LEDy) {
      	ytimer = millis(); 
      	unsigned long timeGap = ytimer - ytimer_prev; 
		if (LEDy_state = 0 && timeGap >= off) {
			LED_onoff(pinNum, 1);
			ytimer_prev = millis();
		}
		else if (LEDy_state = 1 && timeGap >= on) {
			LED_onoff(pinNum, 0);
			ytimer_prev = millis();
		}
	}
	if (pinNum == LEDg) {
      	gtimer = millis(); 
      	unsigned long timeGap = gtimer - gtimer_prev; 
		if (LEDg_state = 0 && timeGap >= off) {
			LED_onoff(pinNum, 1);
			gtimer_prev = millis();
		}
		else if (LEDg_state = 1 && timeGap >= on) {
			LED_onoff(pinNum, 0);
			gtimer_prev = millis();
		}
	}
}
