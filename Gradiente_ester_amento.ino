#include <HCSR04.h>
#include <MPU6050_tockn.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#define TRIG A1
#define ECHO A0
const int CHIP_SELECT = 10;   

HCSR04 sensor(TRIG, ECHO);
MPU6050 mpu6050(Wire);

File logFile;

const unsigned long SAMPLE_INTERVAL_MS = 10; 
const unsigned long PRINT_INTERVAL_MS  = 100;

unsigned long lastSampleMs = 0;
unsigned long lastPrintMs  = 0;

// --- Exemplo de "evento": distância abaixo de um limiar ---
const float DIST_EVENT_THRESHOLD_CM = 20.0; 
unsigned long lastEventMs = 0;
bool lastEventState = false; // f

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }

  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true); // calibra giro/acc

  // --- SD ---
  Serial.print("Inicializando cartao SD...");
  if (!SD.begin(CHIP_SELECT)) {
    Serial.println(" falhou!");
    while (1) { delay(10); }
  }
  Serial.println(" ok!");

  // Abre/Cria arquivo
  bool arquivoExiste = SD.exists("datalog.csv");
  logFile = SD.open("datalog.csv", FILE_WRITE);
  if (!logFile) {
    Serial.println("Erro abrindo datalog.csv");
    while (1) { delay(10); }
  }
  // Cabeçalho só se criou agora
  if (!arquivoExiste) {
    logFile.println("t_ms,accY_m_s2,dist_cm,event,dt_event_ms");
    logFile.flush();
  }
}

void loop() {
  unsigned long now = millis();

  // Amostragem fixa para LOG
  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;

    // Atualiza MPU antes de ler
    mpu6050.update();
    
    float accY_ms2 = mpu6050.getAccY() * 9.80665;
    float accX_ms2 = mpu6050.getAccX() * 9.80665;
    float accZ_ms2 = mpu6050.getAccZ() * 9.80665;

  
    float dist_cm = sensor.dist();

    // --- Detecção de evento (exemplo) ---
    bool eventState = (dist_cm > 0 && dist_cm < DIST_EVENT_THRESHOLD_CM);
    unsigned long dtEvent = 0;
    if (eventState && !lastEventState) {
     
      if (lastEventMs == 0) {
        dtEvent = 0; 
      } else {
        dtEvent = now - lastEventMs; 
      }
      lastEventMs = now;
    }
    lastEventState = eventState;

    // --- Monta linha CSV ---
    // Formato: t_ms, accY_m_s2, dist_cm, event(0/1), dt_event_ms
    logFile.print(now);
    logFile.print(",");
    logFile.print(accY_ms2, 3);
    logFile.print(",");
    logFile.print(dist_cm, 2);
    logFile.print(",");
    logFile.print(eventState ? 1 : 0);
    logFile.print(",");
    logFile.println(dtEvent);
    
    if ((now % 1000) < SAMPLE_INTERVAL_MS) logFile.flush();

    
    if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
      lastPrintMs = now;
      Serial.print("t=");
      Serial.print(now);
      Serial.print(" ms | AccY=");
      Serial.print(accY_ms2, 3);
      Serial.print(" ms | AccX=");
      Serial.print(accX_ms2, 3);
      Serial.print(" ms | AccZ=");
      Serial.print(accZ_ms2, 3);
      Serial.print(" m/s^2 | Dist=");
      Serial.print(dist_cm, 2);
      Serial.print(" cm");
      if (dtEvent > 0) {
        Serial.print(" | dt_event=");
        Serial.print(dtEvent);
        Serial.print(" ms");
      }
      Serial.println();
    }
  }

  
  delay(1);
}

