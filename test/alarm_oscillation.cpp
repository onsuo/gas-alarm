// 알림 시간 버퍼
...

if (ppm >= 0 && ppm <= 700) {
	if (gasLevel != 0) {
		levelTime = currentTime;
	}
	gasLevel = 0;
}
...

if (getTimeGap(levelTime) >= 5) {
	alarmLevel = gasLevel;
}
...


// averaging
float aveGasLevel(int ppm) {
	int b[5] = { 0, 0, 0, 0, 0 };
	for (int i = 0; i < 5 - 1; i++) { // 저장된 값을 한칸씩 앞으로 당김
		b[i] = b[i + 1];
	}
	b[4] = ppm; // 현재 기체 ppm 저장
	
	int sum = 0;
	for (int i = 0; i < 5; i++) {
		sum = sum + b[i];
	}
	return sum / 5;
}

float Appm1 = aveGasLevel(); // loop 5번 이상 돌아야 정상 작동
if (Appm1 >= 0 && Appm1 <= 700) {
	gasLevel = 0;
}
...


// hysteresis : 경계 문턱 2개 -> 상 문턱 : 알림 단계 높임 / 하 문턱 : 알림 단계 낮춤
if (ppm <= 650 && alarmLevel >= 1) {
	alarmLevel = 0;
}
else if ((ppm >= 750 && alarmLevel <= 0) || (ppm <= 950 && alarmLevel >= 2)) {
	alarmLevel = 1;
}
else if ((ppm >= 950 && alarmLevel <= 1) || (ppm <= 1950 && alarmLevel >= 3)) {
	alarmLevel = 2;
}
else if ((ppm >= 1950 && alarmLevel <= 2) || (ppm <= 2950 && alarmLevel >= 4)) {
	alarmLevel = 3;
}
else if (ppm >= 2950 && alarmLevel <= 3) {
	alarmLevel = 4;
}
