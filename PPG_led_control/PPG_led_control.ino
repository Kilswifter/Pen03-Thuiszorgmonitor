//#include 

int led_pin = 1;
int IR_pin = 2;
int interval_nb = 1;

void ICACHE_RAM_ATTR onTimerISR(){
    timer1_write(1250); // 250 us
    interval_nb++;
    if (interval_nb == 3){
      digitalWrite(1, LOW); //LOW
      digitalWrite(2, HIGH); // HIGH
    }
    if (interval_nb == 4){
      state = !state;
      digitalWrite(1, HIGH; // HIGH
      digitalWrite(2, LOW); // LOW
      interval_nb = 0;
    }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  Serial.println("communication starting...");

  pinMode(1, OUTPUT);
  digitalWrite(led_pin, HIGH);
  pinMode(1, OUTPUT);
  digitalWrite(IR_pin, LOW);

  timer1_isr_init(); 
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE); // 5MHZ timer gives 5 ticks per us (80MHz timer divided by 16)
  timer1_write(1250);                             // number of ticks, 1250 ticks / 5 ticks per us = 250 us
  
}

void loop() {
  // put your main code here, to run repeatedly:
}
