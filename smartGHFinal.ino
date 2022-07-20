#include <HTTPClient.h>
#include <WiFi.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <DHT_U.h>
#define bomba 23
#define sensorHum 13
#define echoPin 12
#define trigPin 14
byte aguaVacia[8] = {
    0b01010,
    0b11011,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b11111};
byte customChar[8] = {
    0b01110,
    0b01010,
    0b01110,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000};
byte porcentChar[] = {
    B01100,
    B01101,
    B00010,
    B00100,
    B01000,
    B10110,
    B00110,
    B00000};
byte termometro[] = {
    B00100,
    B01010,
    B01010,
    B01010,
    B01110,
    B11111,
    B11111,
    B01110};
byte gota[] = {
    B00100,
    B00100,
    B01010,
    B01010,
    B10001,
    B10001,
    B10001,
    B01110};
byte humSuelo[] = {
    B00011,
    B00010,
    B00001,
    B01011,
    B01000,
    B11100,
    B11100,
    B01100};
byte aguaLlena[] = {
    B01010,
    B11011,
    B10001,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111};
byte aguaMedioLlena[] = {
    B01010,
    B11011,
    B10001,
    B10001,
    B11111,
    B11111,
    B11111,
    B11111};
byte aguaMitad[] = {
    B01010,
    B11011,
    B10001,
    B10001,
    B10001,
    B11111,
    B11111,
    B11111};
byte aguaBaja[] = {
    B01010,
    B11011,
    B10001,
    B10001,
    B10001,
    B10001,
    B11111,
    B11111};

DHT dht(4, DHT11);

const int rs = 15, en = 16, d4 = 17, d5 = 5, d6 = 18, d7 = 19; // vo = potenciometro, vdd = 5V, k = ground, a = 5V,rw = ground, vss = ground
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const char *ssid = "Ubee72B3-2.4G";
const char *password = "NFOYQQU30E";

IPAddress ip_local(192,168,0,26);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

const char *url = "http://192.168.89.216/prueba/esp32-post.php";

float t, h, humS, hS, agua;
long seg, distance;
int bombaEstado = 0;

unsigned long time1, time2;
int intervaloLecturas = 0, intervaloActualizacion = 0;

void setup()
{
  Serial.begin(115200);
  dht.begin();
  lcd.begin(16, 2);
  lcd.createChar(0, customChar);
  lcd.createChar(1, porcentChar);
  lcd.createChar(2, termometro);
  lcd.createChar(3, gota);
  lcd.createChar(4, humSuelo);
  lcd.clear();
  pinMode(bomba, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);
  digitalWrite(bomba, LOW);
  bombaEstado = 0;

  if (!WiFi.config(ip_local, gateway, subnet)) {
    Serial.println("Error en configuracion");
  }
  
  WiFi.begin(ssid, password);

  Serial.print("Conectando...");
  while (WiFi.status() != WL_CONNECTED) { //Check for the connection
    delay(500);
    Serial.print(".");
  }

  Serial.print("Conectado con éxito, mi IP es: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
}

void loop()
{
  time1 = millis(); // cada 15 segundos actualizar datos
  time2 = millis();

  if (time2 - intervaloActualizacion >= 2000)
  {
    intervaloActualizacion = time2;
    t = dht.readTemperature();
    h = dht.readHumidity();
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    seg = pulseIn(echoPin, HIGH);
    distance = seg / 59; // 2 cm = 100%, 25cm = 0%
    agua = ((25.0 - distance) * 100.0) / 23.0;
    humS = analogRead(sensorHum); // 4095 seco y 1554 humedad perfecta
    hS = ((4095.0 - humS) * 100.0) / 2541.0;
    Serial.println("Temperatura: " + String(t) + "°C " + "Humedad: " + String(h) + "%");
    Serial.println("Humedad de suelo: " + String(hS) + "% Nivel de agua: " + String(agua) + "%");
    Serial.println("Distancia:" + String(distance) + "cm Estado bomba: " + String(bombaEstado) + "\n");
  }

  if (hS > 100.0)
  {
    hS = 100.0;
  }
  if (hS < 0.0)
  {
    hS = 0.0;
  }

  if (agua > 100.0 || distance <= 3)
  {
    agua = 100.0;
  }
  if (agua < 0.0 || distance >= 24)
  {
    agua = 0.0;
  }

  if (bombaEstado == 0)
  { // si la bomba esta apagada
    mostrarDHT11();
    mostrarHumedadSuelo();
    mostrarNivelAgua();
    if (hS <= 49 && agua >= 15)
    {
      bombaEstado = 1;
      lcd.clear();
    }
  }
  else
  {
    regar();
  }

  if (time1 - intervaloLecturas >= 15000)
  {
    intervaloLecturas = time1;
    mandarDatos();
  }
}
void regar()
{
  digitalWrite(bomba, HIGH);
  lcd.setCursor(1,0);
  lcd.print("R E G A N D O");
  mostrarHumedadSuelo();
  mostrarNivelAgua();
  if (hS >= 50 || agua <= 0)
  {
    digitalWrite(bomba, LOW);
    bombaEstado = 0;
  }
}

void mostrarDHT11()
{
  // mostrar temperatura ambiental
  lcd.setCursor(0, 0);
  lcd.write(byte(2));
  lcd.print(t);
  lcd.write(byte(0));
  lcd.print("C ");
  // mostrar humedad ambiental
  lcd.setCursor(9, 0);
  lcd.write(byte(3));
  lcd.print(h);
  lcd.write(byte(1));
}
void mostrarHumedadSuelo()
{
  // mostrar humedad de suelo
  if (hS <= 0.0)
  {


      lcd.setCursor(0, 1);
      lcd.write(byte(4));
      lcd.print(hS);
      lcd.write(byte(1));
      lcd.setCursor(6, 1);
      lcd.print("   ");
    
  }
  else
  {
    lcd.setCursor(0, 1);
    lcd.write(byte(4));
    lcd.print(hS);
    lcd.write(byte(1));
    if (hS < 10.0)
    {
      lcd.setCursor(6, 1);
      lcd.print(" ");
      lcd.setCursor(7, 1);
      lcd.print(" ");
    }
    lcd.setCursor(7, 1);
    lcd.print(" ");
    if (hS == 100.0)
    {
      lcd.setCursor(7, 1);
      lcd.write(byte(1));
    }
  }
}
void mostrarNivelAgua()
{
  // mostrar nivel de agua
  if (agua <= 0.0)
  {
    
      lcd.createChar(5, aguaVacia);
      lcd.setCursor(9, 1);
      lcd.write(byte(5));
      lcd.print(agua);
      lcd.write(byte(1));
      lcd.setCursor(15, 1);
      lcd.print(" ");
  }
  else
  {
    if (agua >= 0 && agua <= 24)
      lcd.createChar(5, aguaBaja);
      lcd.setCursor(9, 1);
      lcd.write(byte(5));
    if (agua >= 25 && agua <= 49)
    {
      lcd.createChar(5, aguaMitad);
      lcd.setCursor(9, 1);
      lcd.write(byte(5));
    }
    if (agua >= 50 && agua <= 74)
    {
      lcd.createChar(5, aguaMedioLlena);
      lcd.setCursor(9, 1);
      lcd.write(byte(5));
    }
    if (agua >= 75 && agua <= 100)
    {
      lcd.createChar(5, aguaLlena);
      lcd.setCursor(9, 1);
      lcd.write(byte(5));
    }
    lcd.setCursor(10, 1);
    lcd.print(agua);
    lcd.write(byte(1));
    if (agua <= 9)
    {
      lcd.setCursor(15, 1);
      lcd.print(" ");
    }
    if (agua == 100.0)
    {
      lcd.setCursor(15, 1);
      lcd.write(byte(1));
    }
  }
}

void mandarDatos(){
  if (WiFi.status() == WL_CONNECTED)
  { // Check WiFi connection status
    HTTPClient http;
    String datos = "temp=" + String(t) + "&hum=" + String(h) + "&humS=" + String(hS) + "&nivelAgua=" + String(agua) + "&bomba=" + String(bombaEstado);
    Serial.println(datos);
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Preparamos el header text/plain si solo vamos a enviar texto plano sin un paradigma llave:valor.

    int response = http.POST(datos); // Enviamos el post pasándole, los datos que queremos enviar. (esta función nos devuelve un código que guardamos en un int)

    if (response > 0)
    {
      Serial.println("Código HTTP ► " + String(response)); // Print return code

      if (response == 200)
      {
        Serial.println("Todo salio bien :)");
      }
    }
    else
    {
      Serial.print(" Error enviando POST, código: ");
      Serial.println(response);
    }
    http.end(); // libero recursos
  }
  else
  {
    Serial.println("Error en la conexión WIFI");
  }
}
