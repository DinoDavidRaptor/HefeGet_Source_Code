#include <Bluepad32.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Pines para los motores
const int motor1_entrada1 = 18;
const int motor1_entrada2 = 17;
const int motor2_entrada1 = 8;
const int motor2_entrada2 = 3;
const int sensorVelocidadPin = 9;

// Dimensiones y peso del carro
const float diametroLlantas = 0.06; // 6 cm en metros
const float pesoCarro = 0.780;      // 780 gramos en kg

// Variables de velocidad y potencia
volatile int pulsos = 0;
float velocidad = 0.0;
float aceleracion = 0.0;
float potencia = 0.0;
unsigned long lastTime = 0;
float lastVelocidad = 0.0;

// WiFi del esp
const char *ssid = "CarroControl";
const char *password = "12345678";
AsyncWebServer server(80);

// Variables para el gamepad
ControllerPtr myController = nullptr;

// Interrupción para contar pulsos del sensor
void IRAM_ATTR contarPulsos() {
  pulsos++;
}

// Funciones de control de motores
void motor1_adelante() {
  digitalWrite(motor1_entrada1, HIGH);
  digitalWrite(motor1_entrada2, LOW);
}

void motor1_atras() {
  digitalWrite(motor1_entrada1, LOW);
  digitalWrite(motor1_entrada2, HIGH);
}

void motor1_parar() {
  digitalWrite(motor1_entrada1, LOW);
  digitalWrite(motor1_entrada2, LOW);
}

void motor2_adelante() {
  digitalWrite(motor2_entrada1, HIGH);
  digitalWrite(motor2_entrada2, LOW);
}

void motor2_atras() {
  digitalWrite(motor2_entrada1, LOW);
  digitalWrite(motor2_entrada2, HIGH);
}

void motor2_parar() {
  digitalWrite(motor2_entrada1, LOW);
  digitalWrite(motor2_entrada2, LOW);
}

void adelante() {
  Serial.println("Motores adelante");
  motor1_adelante();
  motor2_adelante();
}

void atras() {
  Serial.println("Motores atrás");
  motor1_atras();
  motor2_atras();
}

void derecha() {
  Serial.println("Girando a la derecha");
  motor1_atras();
  motor2_adelante();
}

void izquierda() {
  Serial.println("Girando a la izquierda");
  motor1_adelante();
  motor2_atras();
}

void parar() {
  Serial.println("Motores detenidos");
  motor1_parar();
  motor2_parar();
}

// Detecta si hay control conectado o no
void onConnectedController(ControllerPtr ctl) {
  Serial.println("Gamepad conectado");
  myController = ctl;
}

void onDisconnectedController(ControllerPtr ctl) {
  Serial.println("Gamepad desconectado");
  if (myController == ctl) {
    myController = nullptr;
  }
}

// Procesar entrada del gamepad
void processGamepad() {
  if (!myController || !myController->isConnected()) return;

  int16_t dpad = myController->dpad(); // cruceta
  int16_t throttle = myController->throttle(); // gatillo izquierdo
  int16_t brake = myController->brake(); // gatillo derecho

  if (throttle > 0) {
    adelante();
  } else if (brake > 0) {
    atras();
  } else if (dpad == 0x08) { // Izquierda
    izquierda();
  } else if (dpad == 0x04) { // Derecha
    derecha();
  } else {
    parar();
  }
}

// Calcular velocidad, aceleración y potencia
void calcularParametros() {
  unsigned long currentTime = millis();
  float tiempo = (currentTime - lastTime) / 1000.0; // Tiempo en segundos
  float circunferencia = diametroLlantas * PI;

  velocidad = (pulsos * circunferencia) / tiempo; // m/s
  aceleracion = (velocidad - lastVelocidad) / tiempo; // m/s²
  potencia = pesoCarro * aceleracion * velocidad; // W

  lastVelocidad = velocidad;
  lastTime = currentTime;
  pulsos = 0; // Resetear contador de pulsos
}

// servidor web Bryan 
void iniciarWiFi() {
  WiFi.softAP(ssid, password, 1, 0, 4); // Canal 1, visible, máx. 4 conexiones
  WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Maxima potencia :)
  Serial.println("Punto de acceso iniciado.");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html><head><title>Monitor Velocidad, Aceleracion y Potencia</title></head><body>";
  html += "<h1>Estado del Carro</h1>";
html += "<p>Velocidad: " + String(velocidad, 2) + " m/s (" + String(velocidad * 3.6, 2) + " km/h)</p>";
html += "<p>Aceleracion: " + String(aceleracion, 2) + " m/s²</p>";
html += "<p>Potencia: " + String(potencia, 2) + " W</p>";
html += "<script>setTimeout(()=>{location.reload()}, 500);</script>";
html += "</body></html>";

    request->send(200, "text/html", html);
  });

  server.begin();
}

void verificarWiFi() {
  if (!WiFi.softAPgetStationNum() && WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
    Serial.println("Reiniciando wifi");
    iniciarWiFi();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); // baudios y codigo que funciono

  // Configurar pines
  pinMode(motor1_entrada1, OUTPUT);
  pinMode(motor1_entrada2, OUTPUT);
  pinMode(motor2_entrada1, OUTPUT);
  pinMode(motor2_entrada2, OUTPUT);
  pinMode(sensorVelocidadPin, INPUT_PULLUP); // no quiten el pullup

  attachInterrupt(digitalPinToInterrupt(sensorVelocidadPin), contarPulsos, RISING); //codigo que funciona para el sensor que encontre en stackoverflow :)

  // Inicializar Bluepad32 (Libreria publica)
  BP32.setup(&onConnectedController, &onDisconnectedController);

  // Iniciar WiFi
  iniciarWiFi();

  Serial.println("Sistema iniciado.");
}

void loop() {
  BP32.update();         // Actualizar Bluepad32
  processGamepad();      // Procesar entradas del controlador
  calcularParametros();  // Calcular parámetros físicos
  verificarWiFi();       // Verificar estado del WiFi
}
