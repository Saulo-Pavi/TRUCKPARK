/*
 * PROJETO: TruckPark - Controle de Acesso e Gestão de Vagas
 * CURSO: Sistemas de Informação - UNOESC Chapecó
 * DISCIPLINA: Introdução ao Hardware (2026/2)
 * PROFESSOR: Jacson Luiz Matte
 * ALUNOS: Saulo Pavi, Erik Ricardo, Mateus Rocha, Alisson
 */

#include <Servo.h>

// --- DEFINIÇÃO DOS PINOS ---
const int PIN_TRIG = 2;
const int PIN_ECHO = 3;
const int PIN_LED_VERDE = 4;
const int PIN_LED_VERMELHO = 5;
const int PIN_BUZZER = 6;
const int PIN_SERVO = 9;

// --- OBJETOS E VARIÁVEIS ---
Servo cancela;

unsigned long tempoAbertura = 0;
unsigned long tempoAnteriorSerial = 0;

const long TEMPO_ESPERA = 5000;     // 5 segundos aberta
const long INTERVALO_SERIAL = 500;  // Atualiza Serial a cada 500ms

// --- GESTÃO DE VAGAS DO PÁTIO ---
const int VAGAS_TOTAIS = 5;         // Capacidade total do pátio
int vagasDisponiveis = VAGAS_TOTAIS; // Contador de vagas livres

enum EstadoCancela { FECHADA, ABERTA, AGUARDANDO_SAIDA };
EstadoCancela estadoAtual = FECHADA;

// --- FUNÇÃO PARA TOCAR SOM SUAVE ---
void tocarSomSuave() {
  tone(PIN_BUZZER, 523, 150);
  delay(180);
  tone(PIN_BUZZER, 659, 200);
}

// --- FUNÇÃO PARA LER O SENSOR ULTRASSÔNICO ---
long lerDistancia() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  
  long duracao = pulseIn(PIN_ECHO, HIGH, 30000);
  long distanciaCm = duracao * 0.034 / 2;
  
  if (distanciaCm <= 0 || distanciaCm > 400) {
    return -1; 
  }
  return distanciaCm;
}

void setup() {
  Serial.begin(9600);
  
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  
  cancela.attach(PIN_SERVO);
  cancela.write(0); // Cancela começa fechada (0 graus)
  
  digitalWrite(PIN_LED_VERMELHO, HIGH);
  digitalWrite(PIN_LED_VERDE, LOW);
  
  Serial.println("==========================================");
  Serial.println("   Sistema TruckPark Inicializado         ");
  Serial.print("   Vagas Totais do Patio: ");
  Serial.println(VAGAS_TOTAIS);
  Serial.println("==========================================");
}

void loop() {
  unsigned long tempoAtual = millis();
  long distancia = lerDistancia();

  // Exibição limpa no Monitor Serial
  if (tempoAtual - tempoAnteriorSerial >= INTERVALO_SERIAL) {
    tempoAnteriorSerial = tempoAtual;
    if (distancia != -1) {
      Serial.print("Distancia: ");
      Serial.print(distancia);
      Serial.print(" cm | Vagas Livres: ");
      Serial.println(vagasDisponiveis);
    }
  }

  // --- MÁQUINA DE ESTADOS DA CANCELA ---
  switch (estadoAtual) {
    
    case FECHADA:
      // Se detectar veículo próximo (<= 15cm)
      if (distancia > 0 && distancia <= 15) {
        
        // VERIFICAÇÃO DE VAGAS
        if (vagasDisponiveis > 0) {
          vagasDisponiveis--; // Decrementa uma vaga livre
          
          Serial.println("\n------------------------------------------");
          Serial.println(">>> VEICULO DETECTADO! ENTRADA LIBERADA <<<");
          Serial.print(">>> Vagas Restantes: ");
          Serial.print(vagasDisponiveis);
          Serial.println(" <<<");
          Serial.println("------------------------------------------\n");
          
          cancela.write(90); // Abre cancela
          digitalWrite(PIN_LED_VERMELHO, LOW);
          digitalWrite(PIN_LED_VERDE, HIGH);
          tocarSomSuave();
          
          tempoAbertura = tempoAtual; 
          estadoAtual = ABERTA;
        } 
        else {
          // Caso o pátio esteja lotado
          Serial.println("\n------------------------------------------");
          Serial.println(">>> PATIO LOTADO! ACESSO NEGADO. <<<");
          Serial.println("------------------------------------------\n");
          
          // Alerta sonoro grave de erro/bloqueio
          tone(PIN_BUZZER, 200, 400);
          delay(500); 
        }
      }
      break;

    case ABERTA:
      // Conta 5 segundos e FECHA a cancela
      if (tempoAtual - tempoAbertura >= TEMPO_ESPERA) {
        Serial.println("\n------------------------------------------");
        Serial.println(">>> TEMPO ESGOTADO (5s). FECHANDO CANCELA... <<<");
        Serial.println("------------------------------------------\n");
        
        cancela.write(0); // Fecha cancela
        digitalWrite(PIN_LED_VERDE, LOW);
        digitalWrite(PIN_LED_VERMELHO, HIGH);
        tone(PIN_BUZZER, 330, 250); // Sinal sonoro grave de fechamento
        
        estadoAtual = AGUARDANDO_SAIDA;
      }
      break;

    case AGUARDANDO_SAIDA:
      // Só volta a permitir nova leitura quando o objeto se afastar (> 15cm)
      if (distancia > 15 || distancia == -1) {
        estadoAtual = FECHADA;
      }
      break;
  }
}