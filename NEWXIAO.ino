#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SENSOR 20  
#define RELE 8      
#define BOTAO 9     // Botão para alternar dosagens
#define RESET_BOTAO 5 // Botão para resetar volume
#define SCREEN_WIDTH 128  // Largura do display OLED
#define SCREEN_HEIGHT 64  // Altura do display OLED
#define OLED_RESET -1     // Reset por hardware não é necessário
#define SDA_PIN 4   // Pino SDA
#define SCL_PIN 5   // Pino SCL

// Cria um objeto display para controlar o OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

unsigned long tempoAtivacao = 0;
bool releLigado = false;
bool ignorarSensor = false;
int dosagem = 15;       // Dosagem inicial em ml (começa com 15ml)
int estadoBotaoAnterior = LOW;  
int estadoBotaoAtual = LOW;
int estadoResetBotaoAnterior = LOW;
int estadoResetBotaoAtual = LOW;
unsigned long tempoBotaoPressionado = 0;
bool botaoResetPressionado = false;

// Variáveis para o volume de líquido no frasco
float volumeTotal = 250.0;    // Volume total do frasco em ml
float volumeRestante = 250.0; // Volume restante no frasco, inicialmente igual ao volume total

void setup() {
  pinMode(SENSOR, INPUT);
  pinMode(RELE, OUTPUT);
  pinMode(BOTAO, INPUT_PULLUP);
  pinMode(RESET_BOTAO, INPUT_PULLUP);
  digitalWrite(RELE, LOW);    

  // Inicializa o display OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Endereço I2C pode ser 0x3C ou 0x3D
    Serial.println(F("Falha ao inicializar o OLED"));
    for(;;);
  }
  
  display.clearDisplay();  // Limpa o display
  display.setTextSize(2);  // Aumenta o tamanho do texto
  display.setTextColor(SSD1306_WHITE); // Cor do texto
  display.setCursor(0, 0); // Posiciona o cursor no topo à esquerda
  display.println("Dosagem:");
  display.display();  // Atualiza o display com o conteúdo inicial
}

void loop() {
  int estadoIR = digitalRead(SENSOR);  
  estadoBotaoAtual = digitalRead(BOTAO);  
  estadoResetBotaoAtual = digitalRead(RESET_BOTAO);

  // Alterna entre as dosagens quando o botão é pressionado
  if (estadoBotaoAtual == LOW && estadoBotaoAnterior == HIGH) {
    if (dosagem == 15) {
      dosagem = 25;  // Muda para 25ml
    } else if (dosagem == 25) {
      dosagem = 30;  // Muda para 30ml
    } else {
      dosagem = 15;  // Volta para 15ml
    }
    
    // Atualiza o OLED com a nova dosagem e porcentagem restante
    atualizarDisplay();
  }
  estadoBotaoAnterior = estadoBotaoAtual;

  // Verifica se o botão de reset está sendo pressionado por 3 segundos
  if (estadoResetBotaoAtual == LOW && estadoResetBotaoAnterior == HIGH) {
    tempoBotaoPressionado = millis(); // Marca o tempo em que o botão começou a ser pressionado
    botaoResetPressionado = true;
  }
  
  // Se o botão continuar pressionado por 3 segundos, reseta o volume
  if (estadoResetBotaoAtual == LOW && botaoResetPressionado) {
    if (millis() - tempoBotaoPressionado >= 3000) {
      volumeRestante = volumeTotal;  // Reseta o volume restante para 100%
      exibirMensagemRefil();  // Exibe a mensagem de troca de refil
      botaoResetPressionado = false; // Impede múltiplos resets consecutivos
    }
  }

  // Se o botão for solto antes de 3 segundos, reseta a verificação
  if (estadoResetBotaoAtual == HIGH) {
    botaoResetPressionado = false;
  }
  
  estadoResetBotaoAnterior = estadoResetBotaoAtual;

  // Ajusta o tempo de ativação de acordo com a dosagem selecionada
  int tempoDosagem = 0;
  if (dosagem == 25) {
    tempoDosagem = 2000;   // 25ml em 2 segundos
  } else if (dosagem == 15) {
    tempoDosagem = 1200;   // 15ml em 1.2 segundos
  } else if (dosagem == 30) {
    tempoDosagem = 2400;   // 30ml em 2.4 segundos
  }

  // Se o sensor detectar 1 e não estiver ignorando
  if (estadoIR == HIGH && !ignorarSensor) {
    digitalWrite(RELE, LOW);  // Liga o relé
    tempoAtivacao = millis();   // Armazena o tempo de ativação
    releLigado = true;          // Marca que o relé foi ligado
    ignorarSensor = true;       // Começa a ignorar novas leituras do sensor
  }

  // Se o relé estiver ligado e já passaram os segundos conforme a dosagem
  if (releLigado && (millis() - tempoAtivacao >= tempoDosagem)) {
    digitalWrite(RELE, HIGH);   // Desliga o relé
    releLigado = false;         // Reseta o estado do relé

    // Atualiza o volume restante
    volumeRestante -= dosagem;
    if (volumeRestante < 0) volumeRestante = 0; // Garante que o volume não fique negativo

    // Atualiza o OLED com a nova dosagem e porcentagem restante
    atualizarDisplay();
  }
  
  // Reseta o sistema se o sensor detectar 0
  if (estadoIR == LOW) {
    ignorarSensor = false;     // Volta a detectar o sensor
  }

  delay(100);  
}

// Função para atualizar o display OLED com a dosagem e porcentagem restante
void atualizarDisplay() {
  // Calcula a porcentagem que sobrou
  float porcentagemRestante = (volumeRestante / volumeTotal) * 100.0;

  display.clearDisplay();
  display.setCursor(0, 0); 
  display.setTextSize(1);  // Aumenta o tamanho do texto para 2
  display.println("Dosagem:");
  display.setCursor(0, 20);
  display.setTextSize(3);  // Aumenta o tamanho do texto para 3 para exibir a dosagem
  display.print(dosagem);
  display.println("ml");

  // Exibe a porcentagem restante com tamanho maior
  display.setCursor(0, 50);
  display.setTextSize(1);  
  display.print("Restante: ");
  display.print(porcentagemRestante, 1);
  display.println("%");

  display.display();  // Atualiza o display com as informações
}

// Função para exibir uma mensagem temporária de troca de refil
void exibirMensagemRefil() {
  display.clearDisplay();
  
  // Exibe "Refil" na primeira linha
  display.setCursor(0, 10); 
  display.setTextSize(2);  // Define o tamanho do texto maior
  display.println("Refil");
  
  // Exibe "trocado!" na segunda linha
  display.setCursor(0, 35); 
  display.setTextSize(2);  // Mantém o tamanho do texto grande
  display.println("trocado!");
  
  display.display();  // Atualiza o display com a mensagem

  delay(2000);  // Aguarda 2 segundos antes de voltar às informações normais

  // Após a mensagem, atualiza o display novamente com a dosagem e o volume restante
  atualizarDisplay();
}