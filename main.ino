#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <arduinoFFT.h>

// Definições do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Definições da FFT
unsigned int amostras = 1024;
volatile int indice_amostrar = 0;  // Ajustado para começar em 0
double amplitude_pico = 0;
double freq_pico = 0;
double dados_real[1024], dados_imag[1024];
ArduinoFFT<double> FFT = ArduinoFFT<double>(dados_real, dados_imag, amostras, 1024);

// Timer
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// Frequência de amostragem
double taxa_amostragem = 1024.0;  // Definido para 1024 Hz

// Função de interrupção do timer
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  if (indice_amostrar < amostras) {
    dados_real[indice_amostrar] = analogRead(35);  // Leitura do pino analógico
    dados_imag[indice_amostrar] = 0;               // Inicializa a parte imaginária
    indice_amostrar++;
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {
  // Inicia a comunicação serial para debugging
  Serial.begin(115200);

  // Inicia o display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Endereço I2C padrão
    Serial.println(F("Falha ao inicializar o display OLED"));
    for (;;);  // Para o programa se o display falhar
  }

  // Inicializa o timer
  timer = timerBegin(0, 80, true);  // Timer 0, prescaler 80 para 1us por tick
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000 / taxa_amostragem, true);  // Configura o alarme para taxa de amostragem
  timerAlarmEnable(timer);  // Habilita o alarme
}

void loop() {
  amostra_calcula_FFT();
  afinador();
}

// Função para amostrar e calcular a FFT
void amostra_calcula_FFT() {
  indice_amostrar = 0;  // Reinicia a amostragem
  amplitude_pico = 0;

  // Aguarda a coleta de todas as amostras
  while (indice_amostrar < amostras) {
    // Espera passiva, mas seria melhor usar um flag em sistemas mais complexos
    delay(1);
  }

  FFT.dcRemoval();  // Remove o offset DC
  FFT.compute(FFTDirection::Forward);  // Calcula a FFT
  FFT.complexToMagnitude();  // Calcula o módulo

  for (int i = 2; i < (amostras / 2); i++) {  // Ignorando os primeiros componentes
    if (dados_real[i] > amplitude_pico) {
      freq_pico = i * (taxa_amostragem / amostras);  // Converte índice da FFT para Hz
      amplitude_pico = dados_real[i];
    }
  }
}

// Função para exibir o valor da frequência detectada no display OLED
void afinador() {

  float freq_central, freq_max, freq_min, largura_faixa;
  char notacao[2];

  if(freq_pico < 68){
    notacao[0] = '--'; notacao[1] = '--';
    freq_min = 0;
    freq_central = 0;
    freq_max = 0;
    largura_faixa = 1;
  }

  if(freq_pico > 68 && freq_pico <= 96){
    notacao[0] = 'E'; notacao[1] = '2';
    freq_min = 68;
    freq_central = 82;
    freq_max = 96;
    largura_faixa = 28;
  }

  if(freq_pico > 96 && freq_pico <= 124){
    notacao[0] = 'A'; notacao[1] = '2';
    freq_min = 96;
    freq_central = 110;
    freq_max = 124;
    largura_faixa = 28;
  }

  if(freq_pico > 124 && freq_pico <= 168){
    notacao[0] = 'D'; notacao[1] = '3';
    freq_min = 124;
    freq_central = 146;
    freq_max = 168;
    largura_faixa = 44;
  }

  if(freq_pico > 168 && freq_pico <= 224){
    notacao[0] = 'G'; notacao[1] = '3';
    freq_min = 168;
    freq_central = 196;
    freq_max = 224;
    largura_faixa = 56;
  }

  if(freq_pico > 224 && freq_pico <= 270){
    notacao[0] = 'B'; notacao[1] = '3';
    freq_min = 224;
    freq_central = 247;
    freq_max = 270;
    largura_faixa = 46;
  }

  if(freq_pico > 270 && freq_pico <= 390){
    notacao[0] = 'E'; notacao[1] = '4';
    freq_min = 270;
    freq_central = 330;
    freq_max = 390;
    largura_faixa = 120;
  }

  if(freq_pico > 390){
    notacao[0] = '--'; notacao[1] = '--';
    freq_min = 0;
    freq_central = 0;
    freq_max = 0;
    largura_faixa = 1;
  }

  display.clearDisplay();

  // Desenho de um quadro no display
  display.drawLine(0, 0, 0, 63, 1);
  display.drawLine(0, 0, 82, 0, 1);
  display.drawLine(82, 0, 82, 63, 1);
  display.drawLine(0, 63, 82, 63, 1);

  // Ajusta a posição do cursor com base no valor da frequência
  if (freq_pico < 100) {
    display.setCursor(96, 9);
  } else {
    display.setCursor(90, 9);
  }

  // Exibe o valor da frequência
  display.setTextSize(2);  // Tamanho da fonte (2x)
  display.setTextColor(SSD1306_WHITE);
  display.print(freq_pico, 0);  // Exibe a frequência em Hz
  display.setCursor(97,45);
  display.print(notacao);
  display.setTextSize(1);
  display.setCursor(98,28);
  display.print("Hz");

    // Criando o Arco do afinador

  for(float i = -0.7; i <= 0.7; i = i+ 0.015)
  display.drawPixel(41+sin(i)*50,62-cos(i)*50,1);
  display.drawLine(1,61,81,61,0);
  display.drawLine(1,62,81,62,0);

  display.setCursor(32,3);
  display.print(freq_central,0); // Legenda Central

  display.setCursor(3,8);
  display.print(freq_min,0); // Legenda Minima

  display.setCursor(63,8);
  display.print(freq_max,0); // Legenda Máxima

  float horizontal = (freq_pico - freq_central) * (66 / largura_faixa) + 41;
  if(freq_central >= 68)
    display.drawLine((int)horizontal,17+abs((horizontal-41)*0.36),41,60,1);
  else{
    display.setTextSize(2);
    display.setCursor(36,32);
    display.print("?");
  }

  display.drawCircle(41,59,3,1);
  display.drawRect(40,13,3,10,1);
  display.drawLine(8,24,10,27,1);
  display.drawLine(73,24,71,27,1);
  display.drawLine(23,16,24,19,1);
  display.drawLine(58,16,57,19,1);
  display.display();
}
