#include "driver/ledc.h"

// ================= UART =================
#define UART_RX 17
#define UART_TX 18
#define BAUDRATE 115200

// ================= SERVO PINS =================
#define BASE_PIN     5      // 360° continuous
#define SHOULDER_PIN 12     // 180°
#define ELBOW1_PIN   13     // 180°
#define ELBOW2_PIN   14     // 180°
#define WRIST_PIN    11     // 180°
#define CLAW_PIN     9      // 180° gripper

// ================= LEDC CONFIG =================
#define SERVO_FREQ_HZ   50
#define SERVO_TIMER     LEDC_TIMER_0
#define SERVO_MODE      LEDC_LOW_SPEED_MODE
#define SERVO_RES       LEDC_TIMER_10_BIT

#define BASE_CH      LEDC_CHANNEL_0
#define SHOULDER_CH  LEDC_CHANNEL_1
#define ELBOW1_CH    LEDC_CHANNEL_2
#define ELBOW2_CH    LEDC_CHANNEL_3
#define WRIST_CH     LEDC_CHANNEL_4
#define CLAW_CH      LEDC_CHANNEL_5

// ================= PWM VALUES (10-bit @ 50 Hz) =================
#define DUTY_MIN     26
#define DUTY_CENTER  77
#define DUTY_MAX     128

// ================= CLAW HARD LIMITS =================
#define CLAW_MIN  30    // adjust as needed
#define CLAW_MAX  95    // adjust as needed

// ================= JOG / STEP CONFIG =================
#define POS_STEP        1
#define BASE_STEP       1
#define STEP_DELAY_MS  50

// ================= STATE =================
int baseDir      = 0;
int shoulderDir  = 0;
int elbow1Dir    = 0;
int elbow2Dir    = 0;
int wristDir     = 0;
int clawDir      = 0;

// Base speed offset (relative to center)
int baseOffset = 0;

// Positional servos
int shoulderPos = DUTY_CENTER;
int elbow1Pos   = DUTY_CENTER;
int elbow2Pos   = DUTY_CENTER;
int wristPos    = DUTY_CENTER;
int clawPos     = DUTY_CENTER;

// ================= COMMAND HANDLER =================
void handleCmd(const String& cmd) {
  Serial.print("RX: ");
  Serial.println(cmd);

  if (cmd == "RB,FWD")      baseDir = +1;
  else if (cmd == "RB,BACK") baseDir = -1;
  else if (cmd == "RB,STOP") baseDir = 0;

  else if (cmd == "SH,FWD")  shoulderDir = +1;
  else if (cmd == "SH,BACK") shoulderDir = -1;
  else if (cmd == "SH,STOP") shoulderDir = 0;

  else if (cmd == "EL1,FWD")  elbow1Dir = +1;
  else if (cmd == "EL1,BACK") elbow1Dir = -1;
  else if (cmd == "EL1,STOP") elbow1Dir = 0;

  else if (cmd == "EL2,FWD")  elbow2Dir = +1;
  else if (cmd == "EL2,BACK") elbow2Dir = -1;
  else if (cmd == "EL2,STOP") elbow2Dir = 0;

  else if (cmd == "WR,FWD")  wristDir = +1;
  else if (cmd == "WR,BACK") wristDir = -1;
  else if (cmd == "WR,STOP") wristDir = 0;

  else if (cmd == "CL,FWD")  clawDir = +1;
  else if (cmd == "CL,BACK") clawDir = -1;
  else if (cmd == "CL,STOP") clawDir = 0;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Serial1.begin(BAUDRATE, SERIAL_8N1, UART_RX, UART_TX);

  ledc_timer_config_t timer;
  memset(&timer, 0, sizeof(timer));
  timer.speed_mode       = SERVO_MODE;
  timer.timer_num        = SERVO_TIMER;
  timer.duty_resolution  = SERVO_RES;
  timer.freq_hz          = SERVO_FREQ_HZ;
  timer.clk_cfg          = LEDC_AUTO_CLK;
  ledc_timer_config(&timer);

  ledc_channel_config_t ch;
  memset(&ch, 0, sizeof(ch));
  ch.speed_mode = SERVO_MODE;
  ch.timer_sel  = SERVO_TIMER;
  ch.duty       = DUTY_CENTER;

  ch.channel = BASE_CH;     ch.gpio_num = BASE_PIN;     ledc_channel_config(&ch);
  ch.channel = SHOULDER_CH; ch.gpio_num = SHOULDER_PIN; ledc_channel_config(&ch);
  ch.channel = ELBOW1_CH;   ch.gpio_num = ELBOW1_PIN;   ledc_channel_config(&ch);
  ch.channel = ELBOW2_CH;   ch.gpio_num = ELBOW2_PIN;   ledc_channel_config(&ch);
  ch.channel = WRIST_CH;    ch.gpio_num = WRIST_PIN;    ledc_channel_config(&ch);
  ch.channel = CLAW_CH;     ch.gpio_num = CLAW_PIN;     ledc_channel_config(&ch);

  Serial.println("Slave ready — claw limits enabled");
}

// ================= LOOP =================
void loop() {
  static String buf;
  static unsigned long lastStep = 0;

  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n' || c == '\r') {
      buf.trim();
      if (buf.length()) handleCmd(buf);
      buf = "";
    } else {
      buf += c;
    }
  }

  unsigned long now = millis();
  if (now - lastStep >= STEP_DELAY_MS) {
    lastStep = now;

    if (baseDir != 0) baseOffset += baseDir * BASE_STEP;
    int baseDuty = constrain(DUTY_CENTER + baseOffset, DUTY_MIN, DUTY_MAX);
    ledc_set_duty(SERVO_MODE, BASE_CH, baseDuty);
    ledc_update_duty(SERVO_MODE, BASE_CH);

    if (shoulderDir != 0) shoulderPos += shoulderDir * POS_STEP;
    ledc_set_duty(SERVO_MODE, SHOULDER_CH, shoulderPos);
    ledc_update_duty(SERVO_MODE, SHOULDER_CH);

    if (elbow1Dir != 0) elbow1Pos += elbow1Dir * POS_STEP;
    ledc_set_duty(SERVO_MODE, ELBOW1_CH, elbow1Pos);
    ledc_update_duty(SERVO_MODE, ELBOW1_CH);

    if (elbow2Dir != 0) elbow2Pos += elbow2Dir * POS_STEP;
    ledc_set_duty(SERVO_MODE, ELBOW2_CH, elbow2Pos);
    ledc_update_duty(SERVO_MODE, ELBOW2_CH);

    if (wristDir != 0) wristPos += wristDir * POS_STEP;
    ledc_set_duty(SERVO_MODE, WRIST_CH, wristPos);
    ledc_update_duty(SERVO_MODE, WRIST_CH);

    // ===== CLAW WITH HARD LIMITS ONLY =====
    if (clawDir != 0) {
      clawPos += clawDir * POS_STEP;
      clawPos = constrain(clawPos, CLAW_MIN, CLAW_MAX);
    }
    ledc_set_duty(SERVO_MODE, CLAW_CH, clawPos);
    ledc_update_duty(SERVO_MODE, CLAW_CH);
  }
}
