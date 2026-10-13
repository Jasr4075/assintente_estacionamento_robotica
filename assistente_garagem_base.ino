// Pinos do Arduino
int PinTrigger = 5;     // dispara os pulsos do sensor
int PinEcho = 6;        // lê a saída do sensor
int PinLEDvermelho = 2; // LED vermelho
int PinLEDamarelo = 3;  // LED amarelo
int PinLEDverde = 4;    // LED verde
int PinBuzzer = A0;     // buzzer

// Constantes
const int atraso = 200;                    // tempo do loop (ms)
const float velocidadeSom_mpus = 0.000340; // metros por microsegundo

// Limites das faixas de distância (cm)
const int LimiteVerde = 29;
const int LimiteAmarelo = 17;
const int LimiteVermelho = 10;
const int LimiteBuzzer = 8;

// Frequência (Hz) e intervalo (ms) do bip de cada faixa
const int FreqVerde = 900,    IntervaloVerde = 800;
const int FreqAmarelo = 1400, IntervaloAmarelo = 400;
const int FreqVermelho = 2000, IntervaloVermelho = 200;
const int FreqBuzzer = 2600;

// Faixas
const int Verde = 1, Amarelo = 2, Vermelho = 3, Buzzer = 4;

// Média móvel para estabilizar as leituras
const int numAmostras = 5;
float amostras[numAmostras];
float somaAmostras = 0;
int indiceAmostra = 0;

unsigned long ultimoBip = 0;       // instante do último bip
float DistanciaAnterior = 0;        // distância da leitura anterior
unsigned long ultimoMovimento = 0;  // instante do último movimento detectado
const float tolerancia = 1.0;       // variação mínima para considerar movimento
const unsigned long tempoDesligar = 10000;  // 10 s sem movimento (faixas amarela e buzzer)
const unsigned long tempoVerde = 2000;      // 2 s sem movimento na faixa verde
const unsigned long tempoVermelho = 10000;   // 5 s sem movimento na faixa vermelha

void setup() {
  pinMode(PinTrigger, OUTPUT);
  pinMode(PinEcho, INPUT);
  pinMode(PinLEDvermelho, OUTPUT);
  pinMode(PinLEDamarelo, OUTPUT);
  pinMode(PinLEDverde, OUTPUT);
  pinMode(PinBuzzer, OUTPUT);
  Serial.begin(9600);
  ultimoMovimento = millis();
}

void loop() {
  float distancia = MediaMovel(MedeDistancia());

  if (abs(distancia - DistanciaAnterior) > tolerancia) {
    ultimoMovimento = millis();   // houve movimento: reinicia o tempo
  }
  DistanciaAnterior = distancia;

  int banda = Classifica(distancia);
  unsigned long limite;
  if (banda == Verde)         limite = tempoVerde;      // verde: 2 s
  else if (banda == Vermelho) limite = tempoVermelho;   // vermelho: 10 s
  else                        limite = tempoVermelho;   // amarelo/ 10 s

  if (millis() - ultimoMovimento >= limite) {
    desligaSinais();              // tempo esgotado: desliga tudo
  } else {
    Sinaliza(banda);
  }

  Serial.print("Distancia em centimetros: ");
  Serial.println(distancia);

  delay(atraso);
}

// Dispara o pulso ultrassônico e mede a distância em cm
float MedeDistancia() {
  digitalWrite(PinTrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(PinTrigger, LOW);

  float tempo_us = pulseIn(PinEcho, HIGH);
  return (tempo_us * velocidadeSom_mpus) / 2 * 100;
}

// Média móvel das últimas leituras
float MediaMovel(float nova) {
  somaAmostras -= amostras[indiceAmostra];
  somaAmostras += nova;
  amostras[indiceAmostra] = nova;
  indiceAmostra = (indiceAmostra + 1) % numAmostras;
  return somaAmostras / numAmostras;
}

// Define a faixa conforme a distância
int Classifica(float distancia) {
  if (distancia <= LimiteBuzzer)   return Buzzer;
  if (distancia <= LimiteVermelho) return Vermelho;
  if (distancia <= LimiteAmarelo)  return Amarelo;
  if (distancia <= LimiteVerde)    return Verde;
  return 0;
}

// Acende o LED e emite o bip da faixa
void Sinaliza(int faixa) {
  desligaSinais();

  int freq, intervalo;
  if (faixa == Buzzer) {
    digitalWrite(PinLEDvermelho, HIGH);
    tone(PinBuzzer, FreqBuzzer);        // muito perto: som contínuo
    return;
  } else if (faixa == Vermelho) {
    digitalWrite(PinLEDvermelho, HIGH);
    freq = FreqVermelho; intervalo = IntervaloVermelho;
  } else if (faixa == Amarelo) {
    digitalWrite(PinLEDamarelo, HIGH);
    freq = FreqAmarelo;  intervalo = IntervaloAmarelo;
  } else if (faixa == Verde) {
    digitalWrite(PinLEDverde, HIGH);
    freq = FreqVerde;    intervalo = IntervaloVerde;
  } else {
    return;                             // sem sinalização
  }

  // Bip pulsado: toca quando o intervalo é atingido
  if (millis() - ultimoBip >= intervalo) {
    ultimoBip = millis();
    tone(PinBuzzer, freq, 120);
  }
}

// Desliga LEDs e buzzer
void desligaSinais() {
  digitalWrite(PinLEDverde, LOW);
  digitalWrite(PinLEDamarelo, LOW);
  digitalWrite(PinLEDvermelho, LOW);
  noTone(PinBuzzer);
}
