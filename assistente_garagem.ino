/* Programa de assistente de garagem com Sensor de Distância Ultrassônico      Blog Eletrogate - https://blog.eletrogate.com/
   Arduino Uno - LCD 16/2 azul - IDE 2.1.0
   Baseado no código de Mauro Vianna   27/junho/2023

*/
// Pinos do arduino utilizados
int PinTrigger = 5;     // pino usado para disparar os pulsos do sensor
int PinEcho = 6;        // pino usado para ler a saida do sensor
int PinLEDvermelho = 2; // pino usado pelo LED vermelho
int PinLEDamarelo = 3;  // pino usado pelo LED amarelo
int PinLEDverde = 4;    // pino usado pelo LED verde
int PinBuzzer = A0;     // pino usado pelo buzzer (passivo)
// Variaveis e constantes para controle de distancia
const int atraso = 200;                     // tempo de espera do loop (ms)
const int MaxContador = 5000 / atraso;      // número de iterações para considerar parado (5s)
const float tolerancia = 1.0;               // tolerancia para detecção de movimento
const float velocidadeSom_mpus = 0.000340;  // velocidade do som em metros por microssegundo
const unsigned long TimeoutEcho = 30000UL;  // tempo máximo de espera do eco (~5 m)
float Distancia = 0;          // Distancia medida (média)
float DistanciaAnterior = 0;  // posição de referência (onde o carro começou a ficar parado)
int contador = 0;             // contador de tempo para detecção de movimento
// Buffer da média móvel para estabilizar as leituras do sensor
const int numAmostras = 5;           // quantidade de amostras na média
float amostras[numAmostras];         // buffer circular das amostras
float somaAmostras = 0;              // soma das amostras (para média rápida)
int indiceAmostra = 0;               // posição da próxima amostra
// Valores limites das faixas de sinalização (cm)
const int LimiteVerde = 29;
const int LimiteAmarelo = 17;
const int LimiteVermelho = 10;
const int LimiteBuzzer = 8;
// Comandos de sinalização
const int Verde = 1;
const int Amarelo = 2;
const int Vermelho = 3;
const int Buzzer = 4;
// Parâmetros dos bips por faixa de sinalização
const int FreqVerde = 900;      // frequência do bip na faixa verde (Hz)
const int FreqAmarelo = 1400;   // frequência do bip na faixa amarela (Hz)
const int FreqVermelho = 2000;  // frequência do bip na faixa vermelha (Hz)
const int FreqBuzzer = 2600;    // frequência do som contínuo na faixa buzzer (Hz)
int contadorBip = 0;            // contador de iterações entre bips
int faixaAtual = 0;             // faixa que está sinalizando agora
// Inicialização
void setup() {
  pinMode(PinTrigger, OUTPUT);      // configura pino Trigger como saída
  digitalWrite(PinTrigger, LOW);    // pino trigger - nível baixo
  pinMode(PinEcho, INPUT);          // configura pino ECHO como entrada
  pinMode(PinLEDvermelho, OUTPUT);  // configura pino LED vermelho como saída
  pinMode(PinLEDamarelo, OUTPUT);   // configura pino LED amarelo como saída
  pinMode(PinLEDverde, OUTPUT);     // configura pino LED verde como saída
  pinMode(PinBuzzer, OUTPUT);       // configura pino Buzzer como saída
  Serial.begin(9600);               // inicializa monitor serial 9600 Bps (debug)
  delay(100);                       // atraso de 100 milissegundos
  for (int i = 0; i < numAmostras; i++) {   // pré-enche o buffer com leituras iniciais
    float leitura = MedeDistancia();
    somaAmostras += leitura;
    amostras[i] = leitura;
    delay(60);                              // intervalo mínimo entre disparos do sensor
  }
  Distancia = somaAmostras / numAmostras;   // inicia com a média
  DistanciaAnterior = Distancia;
  desligaSinais();
}
// Loop principal
void loop() {
  Distancia = MediaMovel(MedeDistancia());
  if (estaParado()) { faixaAtual = 0; desligaSinais(); }
  else if (Distancia <= LimiteBuzzer) { Sinaliza(Buzzer); }
  else if (Distancia <= LimiteVermelho) { Sinaliza(Vermelho); }
  else if (Distancia <= LimiteAmarelo) { Sinaliza(Amarelo); }
  else if (Distancia <= LimiteVerde) { Sinaliza(Verde); }
  else { faixaAtual = 0; desligaSinais(); }

  Serial.print("Distancia em centimetros: ");   // mostra no monitor serial
  Serial.println(Distancia);                    // distância média em cm
  delay(atraso);                                // tempo de atraso
}
// Envia um pulso de 10 us
void DisparaPulsoUltrassonico() {
  digitalWrite(PinTrigger, HIGH);  // pulso alto de Trigger
  delayMicroseconds(10);           // atraso de 10 microssegundos
  digitalWrite(PinTrigger, LOW);   // pulso baixo de Trigger
}
// Calcula a distância com base no tempo do pulso
float CalculaDistancia(float tempo_us) {
  return ((tempo_us * velocidadeSom_mpus) / 2);  // calcula distancia em metros
}
// Mede a distância do sensor ultrassônico
float MedeDistancia() {
  static float ultimaValida = 400;                          // começa "longe" (fora das faixas)
  DisparaPulsoUltrassonico();                               // dispara pulso ultrassonico
  unsigned long TempoEcho = pulseIn(PinEcho, HIGH, TimeoutEcho); // duração do eco em micro seg
  if (TempoEcho == 0) {                                     // sem eco (timeout):
    return ultimaValida;                                    // ignora e repete a última leitura válida
  }
  ultimaValida = CalculaDistancia(TempoEcho) * 100;         // converte para cm
  return ultimaValida;
}
// Média móvel para suavizar as leituras e reduzir ruído
float MediaMovel(float nova) {
  somaAmostras -= amostras[indiceAmostra];   // retira a amostra mais antiga
  somaAmostras += nova;                      // adiciona a nova amostra
  amostras[indiceAmostra] = nova;            // armazena no buffer circular
  indiceAmostra++;
  if (indiceAmostra >= numAmostras) { indiceAmostra = 0; }
  return somaAmostras / numAmostras;         // retorna a média
}
// Sinaliza a faixa apropriada
void Sinaliza(int faixa) {
  if (faixa != faixaAtual) {      // mudou de faixa: bip imediato e reinicia o ciclo
    contadorBip = 0;
    faixaAtual = faixa;
  }
  desligaSinais();
  if (faixa == Verde) {
    digitalWrite(PinLEDverde, HIGH);
  } else if (faixa == Amarelo) {
    digitalWrite(PinLEDamarelo, HIGH);
  } else if (faixa == Vermelho || faixa == Buzzer) {
    digitalWrite(PinLEDvermelho, HIGH);
  }
  bipFaixa(faixa);  // cada faixa tem seu próprio bip
}
// Bip distinto para cada faixa: mais perto, tom mais agudo e mais rápido
void bipFaixa(int faixa) {
  if (faixa == Buzzer) {
    tone(PinBuzzer, FreqBuzzer);   // muito perto: som contínuo
    return;
  }
  int freq;
  int intervalo_ms;
  if (faixa == Verde) {
    freq = FreqVerde;
    intervalo_ms = 800;
  } else if (faixa == Amarelo) {
    freq = FreqAmarelo;
    intervalo_ms = 400;
  } else {                         // Vermelho
    freq = FreqVermelho;
    intervalo_ms = 200;
  }
  int iteracoes = intervalo_ms / atraso;   // iterações do loop entre bips
  if (contadorBip == 0) {
    tone(PinBuzzer, freq, 120);            // bip curto
  }
  contadorBip++;
  if (contadorBip >= iteracoes) {
    contadorBip = 0;
  }
}
// Desliga a sinalização
void desligaSinais() {
  digitalWrite(PinLEDverde, LOW);
  digitalWrite(PinLEDamarelo, LOW);
  digitalWrite(PinLEDvermelho, LOW);
  noTone(PinBuzzer);
}
// Verifica se está parado (não saiu mais de 1 cm da posição de referência por 5 s)
byte estaParado() {
  if (fabs(Distancia - DistanciaAnterior) <= tolerancia) {  // continua perto da referência
    if (contador < MaxContador) {
      contador++;
      if (contador == MaxContador) {                        // avisa uma vez só
        Serial.println("Sem movimento. Desligando sinais...");
      }
    }
  } else {                                                  // se mexeu: nova referência
    contador = 0;
    DistanciaAnterior = Distancia;
  }
  return (contador >= MaxContador);
}
