#include <Wire.h>

// [중요] Mega I2C 통신 배선: SDA -> 20번 핀, SCL -> 21번 핀에 연결
#define PCF8591_ADDRESS 0x48 // PCF8591 기본 I2C 주소
#define AIN_CHANNEL 0x41     // 조도 센서가 연결된 아날로그 채널
#define MotorA 2            // MDD3A PWM (Mega의 PWM 핀: 2~13, 44~46 중 3번 사용)
#define MotorB 3            // MDD3A DIR (방향 제어, 일반 디지털 핀 사용)

// --- 이동 평균 필터 설정 ---
const int NUM_READINGS = 50;
int readings[NUM_READINGS];      
int readIndex = 0;              
long total = 0;                  
int averageLight = 0;                

// --- 제어 변수 설정 ---
int targetPosition = 0;       // 목표 열림 정도 (0 ~ 100)
int currentPosition = 0;      // 현재 추정 위치 (0 ~ 100)
const int MOTOR_SPEED = 150;  // 모터 기본 속도 (0 ~ 255)
const int TOLERANCE = 2;      // 목표 위치 도달 허용 오차 범위

void setup() {
  Wire.begin(); 
  pinMode(MotorA, OUTPUT);
  pinMode(MotorB, OUTPUT);
  Serial.begin(115200);
  
  // 배열 초기화
  for (int i = 0; i < NUM_READINGS; i++) {
    readings[i] = 0;
  }
}

void loop() {
  // 센서 데이터 수집 및 평균 필터 적용
  int rawLight = read_light(AIN_CHANNEL);
  averageLight = applyMovingAverage(rawLight);

  // 조도 값을 커튼 열림 정도(0~100)로 매핑 (조도값 0~255 기준)
  targetPosition = map(averageLight, 0, 255, 0, 100);

  // 3. 모터 구동 알고리즘 (위치 추정 기반)
  if (abs(targetPosition - currentPosition) > TOLERANCE) {
    if (targetPosition > currentPosition) {
      // 커튼 열기
      analogWrite(MotorA, MOTOR_SPEED);
      analogWrite(MotorB, 0);
      currentPosition++; 
    } 
    else if (targetPosition < currentPosition) {
      // 커튼 닫기
      analogWrite(MotorA, 0);
      analogWrite(MotorB, MOTOR_SPEED);
      currentPosition--; 
    }
    delay(50); // 단위 이동당 소요 시간
  } 
  else {
    // 목표 위치 도달 시 모터 정지
    analogWrite(MotorA, 0);
    analogWrite(MotorB, 0);
  }
  
  Serial.print("target position:");
  Serial.print(targetPosition);
  Serial.print(" ");
  Serial.print("current position:");
  Serial.print(" ");
  Serial.print(currentPosition);
  Serial.print("current light:");
  Serial.println(averageLight);
}

// PCF8591 데이터 읽기 함수
int read_light(byte channel) {
  Wire.beginTransmission(PCF8591_ADDRESS);
  Wire.write(channel);
  Wire.endTransmission();
  
  Wire.requestFrom(PCF8591_ADDRESS, 2);
  Wire.read(); // 첫 번째 읽기 값은 이전 변환 결과이므로 버림
  return Wire.read(); // 두 번째 값이 최신 변환 값
}

// 이동 평균 필터 함수
int applyMovingAverage(int newValue) {
  total = total - readings[readIndex];
  readings[readIndex] = newValue;
  total = total + readings[readIndex];
  readIndex = readIndex + 1;

  if (readIndex >= NUM_READINGS) {
    readIndex = 0;
  }
  return total / NUM_READINGS;
}