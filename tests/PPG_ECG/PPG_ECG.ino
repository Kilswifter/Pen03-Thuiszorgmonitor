//#include 

int ECG_PIN = A0;
int PPG_PIN = A1;

hw_timer_t * timer1 = NULL;
hw_timer_t * timer2 = NULL;
volatile SemaphoreHandle_t timerSemaphoreEcg;
volatile SemaphoreHandle_t timerSemaphorePpg;

volatile uint32_t sample_count_ecg = 0;
volatile int ecg_samples[8];

volatile uint32_t sample_count_ppg = 0;
volatile int ppg_samples[8];

void IRAM_ATTR onTimer1(){
  
  ecg_samples[sample_count_ecg] = analogRead(ECG_PIN);
  sample_count_ecg++;
  if (sample_count_ecg == 8){
    xSemaphoreGiveFromISR(timerSemaphoreEcg, NULL);  
  }
}

void IRAM_ATTR onTimer2(){
  
  ppg_samples[sample_count_ppg] = analogRead(PPG_PIN);
  sample_count_ppg++;
  if (sample_count_ppg == 8){
    xSemaphoreGiveFromISR(timerSemaphorePpg, NULL);  
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Create semaphore to inform us when the timer has fired
  timerSemaphoreEcg = xSemaphoreCreateBinary();
  timerSemaphorePpg = xSemaphoreCreateBinary();

  timer1 = timerBegin(1, 80, true);
  timerAttachInterrupt(timer1, &onTimer1, true);
  timerAlarmWrite(timer1, 2500, true);
  timerAlarmEnable(timer1);

  timer2 = timerBegin(2, 80, true);
  timerAttachInterrupt(timer2, &onTimer2, true);
  timerAlarmWrite(timer2, 2500, true);
  timerAlarmEnable(timer2);
}

void loop() {
  // If Timer has fired
  if (xSemaphoreTake(timerSemaphoreEcg, 0) == pdTRUE){
    sample_count_ecg = 0;
    int ecg_samples_copy[8];
    copy_data(ecg_samples, ecg_samples_copy);
    send_data();
  }
  if (xSemaphoreTake(timerSemaphorePpg, 0) == pdTRUE){
    sample_count_ppg = 0;
    int ppg_samples_copy[8];
    copy_data(ppg_samples, ppg_samples_copy);
    send_data(); 
  }
}

void copy_data(volatile int samples[8], int samples_copy[8]){
  for (int i = 0; i < 8; i++){
    samples_copy[i] = samples[i];
  }
}
