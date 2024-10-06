#include <PS4Controller.h>
#include "driver/ledc.h"

#define PS4_MAC_ADDRESS "18:1d:ea:f4:4c:95"
#define PWM_DUTY_CYCLE_RES 8
#define PWM_FREQUENCY 3000
#define LEFT_MOTOR_PWM_PIN 27
#define LEFT_MOTOR_FOWARD_PIN 33
#define LEFT_MOTOR_BACKWARD_PIN 25
#define LEFT_MOTOR_PWM_CHANNEL 5
#define RIGHT_MOTOR_PWM_PIN 26
#define RIGHT_MOTOR_FOWARD_PIN 12
#define RIGHT_MOTOR_BACKWARD_PIN 13
#define RIGHT_MOTOR_PWM_CHANNEL 6
#define SAW_PWM_PIN 14
#define SAW_FOWARD_PIN 17
#define SAW_BACKWARD_PIN 15
#define SAW_PWM_CHANNEL 7
#define LINEAR_SPEED_MULTIPLIER 1.8   // Aumentar a velocidade linear, o quão rápido o robô vai ser
#define ANGULAR_SPEED_MULTIPLIER 1.3  // Suavizar as curvas em alta velocidade.

// VARIÁVEL DE INVERSÃO DE MOVIMENTO
int inv = 1;  // Se o robô estiver começando invertido, alternar para -1. Caso  ou 1

void motors_control(int linear, int angular) {
  int result_R = linear - angular;  // Ao somar o angular com linear em cada motor conseguimos a ideia de direção do robô
  int result_L = linear + angular;

  // Não envia valores de potência abaixo de 15, que não são fortes o suficiente para mover o robô
  if (result_R < 15 && result_R > -15) result_R = 0;
  if (result_L < 15 && result_L > -15) result_L = 0;

  // Não permite valores superiores a 255 ou inferiores a -255
  if (result_R > 255) result_R = 255;
  if (result_R < -255) result_R = -255;
  if (result_L > 255) result_L = 255;
  if (result_L < -255) result_L = -255;

  // Manda para a função motor um valor de -255 a 255, o sinal significa a direção
  set_left_motor_speed(result_L);
  set_right_motor_speed(result_R);
}

void set_left_motor_speed(int speed) {  // Se o valor for positivo, gira para um lado e se for negativo, troca o sentido
  if (speed > 0) {
    digitalWrite(LEFT_MOTOR_FOWARD_PIN, HIGH);
    digitalWrite(LEFT_MOTOR_BACKWARD_PIN, LOW);
  } else {
    digitalWrite(LEFT_MOTOR_FOWARD_PIN, LOW);
    digitalWrite(LEFT_MOTOR_BACKWARD_PIN, HIGH);
  }
  ledcWriteChannel(LEFT_MOTOR_PWM_CHANNEL, abs(speed));
}

void set_right_motor_speed(int speed) {
  if (speed > 0) {
    digitalWrite(RIGHT_MOTOR_FOWARD_PIN, HIGH);
    digitalWrite(RIGHT_MOTOR_BACKWARD_PIN, LOW);
  } else {
    digitalWrite(RIGHT_MOTOR_FOWARD_PIN, LOW);
    digitalWrite(RIGHT_MOTOR_BACKWARD_PIN, HIGH);
  }
  ledcWriteChannel(RIGHT_MOTOR_PWM_CHANNEL, abs(speed));
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  PS4.begin(PS4_MAC_ADDRESS);
  Serial.println("Ready.");

  // Configurar PWM nos pinos dos motores
  ledcAttachChannel(LEFT_MOTOR_PWM_PIN, PWM_FREQUENCY, PWM_DUTY_CYCLE_RES, LEFT_MOTOR_PWM_CHANNEL);
  ledcAttachChannel(RIGHT_MOTOR_PWM_PIN, PWM_FREQUENCY, PWM_DUTY_CYCLE_RES, RIGHT_MOTOR_PWM_CHANNEL);
  ledcAttachChannel(SAW_PWM_PIN, PWM_FREQUENCY, PWM_DUTY_CYCLE_RES, SAW_PWM_CHANNEL);

  pinMode(LEFT_MOTOR_FOWARD_PIN, OUTPUT);
  pinMode(LEFT_MOTOR_BACKWARD_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_FOWARD_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_BACKWARD_PIN, OUTPUT);
  pinMode(SAW_FOWARD_PIN, OUTPUT);
  pinMode(SAW_BACKWARD_PIN, OUTPUT);

  digitalWrite(LEFT_MOTOR_FOWARD_PIN, LOW);
  digitalWrite(LEFT_MOTOR_BACKWARD_PIN, LOW);
  digitalWrite(RIGHT_MOTOR_FOWARD_PIN, LOW);
  digitalWrite(RIGHT_MOTOR_BACKWARD_PIN, LOW);
  digitalWrite(SAW_FOWARD_PIN, LOW);
  digitalWrite(SAW_BACKWARD_PIN, LOW);

  while (!PS4.isConnected()) {
    delay(20);
  }
}

void loop() {
  if (!PS4.isConnected()) {  // Failsafe
    motors_control(0, 0);
    Serial.println("Restart");
    PS4.end();
    ESP.restart();
    delay(20);
  }
  int linear_speed = PS4.LStickY();
  int angular_speed = PS4.RStickX();

  if (abs(linear_speed) > 25) {
    motors_control(1.8 * inv * linear_speed, 1.3 * angular_speed);
  } else {  // Controle sobre valores pequenos devido a problemas na função map
    motors_control(inv * linear_speed, 1.5 * angular_speed);
  }

  // Sentido de locomoção invertido - Botão Seta para baixo
  if (PS4.Down()) {
    inv = -1;
    delay(100);
  }
  // Sentido de locomoção principal
  if (PS4.Up()) {
    inv = 1;
    delay(100);
  }

  // Controle da velocidade da arma
  if (PS4.R2Value() > 15) {
    digitalWrite(SAW_FOWARD_PIN, HIGH);
    digitalWrite(SAW_BACKWARD_PIN, LOW);
    ledcWriteChannel(SAW_PWM_CHANNEL, map(PS4.R2Value(), 0, 190, 0, 225));
  } else if (PS4.L2Value() > 15) {
    digitalWrite(SAW_FOWARD_PIN, LOW);
    digitalWrite(SAW_BACKWARD_PIN, HIGH);
    ledcWriteChannel(SAW_PWM_CHANNEL, map(PS4.L2Value(), 0, 190, 0, 225));
  }
}
