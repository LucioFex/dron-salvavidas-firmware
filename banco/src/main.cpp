// Banco de empuje de un ducto: barrido de acelerador, celda de carga y sensor de agua.
// Manda todo por USB a la laptop, que guarda el CSV con laptop/guardar_csv.py.
// Conexiones y uso en el README.

#include <Arduino.h>
#include <ESP32Servo.h>
#include <HX711.h>
#include <Preferences.h>

// Pines
const int PIN_ESC = 25;        // solo la señal del ESC y la masa. El rojo de 5 V no se conecta
const int PIN_HX711_DT = 16;
const int PIN_HX711_SCK = 4;
const int PIN_AGUA = 34;       // salida analógica (AO) del módulo LM393
const int PIN_AGUA_VCC = 27;   // alimenta el sensor solo mientras lee, así no se gastan los electrodos
const int PIN_BOOT = 0;        // el botón BOOT de la placa corta el motor

// Acelerador en microsegundos. Ver "El rango del ESC" en el README antes de cambiarlos.
const int US_MINIMO = 1000;
const int US_MAXIMO = 2000;

// Barrido: del 30 al 100 % de a 10. Cada paso dura 10 s y después descansa 10 s al mínimo.
const int PCT_DESDE = 30;
const int PCT_HASTA = 100;
const int PCT_CADA = 10;
const unsigned long MS_RAMPA = 1000;     // sube de a poco para no pegar un tirón
const unsigned long MS_PASO = 10000;     // corta solo a los 10 s
const unsigned long MS_MIDIENDO = 5000;  // el empuje sale de los últimos 5 s del paso
const unsigned long MS_DESCANSO = 10000;

Servo esc;
HX711 celda;
Preferences memoria;
float factor = 0;         // lectura cruda por gramo. Sale de "calibrar" y queda guardado
bool taraHecha = false;
String linea = "";

void acelerar(float pct) {
  esc.writeMicroseconds(US_MINIMO + (int)((US_MAXIMO - US_MINIMO) * pct / 100.0));
}

float gramos(long crudo) {
  return (crudo - celda.get_offset()) / factor;
}

bool celdaLista() {
  if (!celda.wait_ready_timeout(1000)) {
    Serial.println("# No encuentro el HX711. Revisá los cables DT, SCK, VCC y GND.");
    return false;
  }
  return true;
}

void tara() {
  if (!celdaLista()) return;
  Serial.println("# Tara: no toques el banco.");
  celda.tare(20);
  taraHecha = true;
  Serial.println("# Tara lista.");
}

// Cualquier tecla o el botón BOOT cortan el motor.
bool hayCorte() {
  if (Serial.available()) {
    while (Serial.available()) Serial.read();
    acelerar(0);
    Serial.println("corte,tecla");
    return true;
  }
  if (digitalRead(PIN_BOOT) == LOW) {
    acelerar(0);
    Serial.println("corte,boton");
    return true;
  }
  return false;
}

// Mantiene el acelerador durante ms milisegundos y manda cada lectura de la celda.
// Devuelve false si hubo un corte.
bool mantener(char config, int paso, int pct, unsigned long ms, bool descanso, unsigned long t0) {
  unsigned long inicio = millis();
  while (millis() - inicio < ms) {
    unsigned long t = millis() - inicio;
    float ahora = descanso ? 0 : pct;
    if (!descanso && t < MS_RAMPA) ahora = pct * t / (float)MS_RAMPA;
    acelerar(ahora);
    if (hayCorte()) return false;
    if (celda.is_ready()) {
      const char* fase = descanso ? "descanso" : (ms - t <= MS_MIDIENDO ? "midiendo" : "subiendo");
      Serial.printf("dato,%lu,%c,%d,%s,%.1f,%.0f\n", millis() - t0, config, paso, fase, ahora,
                    gramos(celda.read()));
    }
  }
  return true;
}

void barrido(char config) {
  if (factor == 0) {
    Serial.println("# Falta calibrar la celda. Ver \"Calibrar la celda\" en el README.");
    return;
  }
  acelerar(0);
  tara();
  if (!taraHecha) return;
  while (Serial.available()) Serial.read();  // el Enter del comando no cuenta como corte

  int pasos = (PCT_HASTA - PCT_DESDE) / PCT_CADA + 1;
  Serial.printf("inicio,%c,%d\n", config, pasos);
  unsigned long t0 = millis();
  int paso = 0;
  for (int pct = PCT_DESDE; pct <= PCT_HASTA; pct += PCT_CADA) {
    paso++;
    Serial.printf("paso,%d,%d,%d\n", paso, pasos, pct);
    if (!mantener(config, paso, pct, MS_PASO, false, t0)) return;
    if (!mantener(config, paso, 0, MS_DESCANSO, true, t0)) return;
  }
  acelerar(0);
  Serial.println("fin");
}

// Le enseña al ESC dónde están el mínimo y el máximo. Siempre sin hélice.
// Ver "El rango del ESC" en el README.
void rangoEsc() {
  Serial.println("# SIN HELICE. La batería tiene que estar desconectada.");
  Serial.println("# Acelerador al máximo: conectá la batería, esperá los pitidos y apretá Enter.");
  delay(100);
  while (Serial.available()) Serial.read();  // el Enter del comando no cuenta
  esc.writeMicroseconds(US_MAXIMO);
  while (!Serial.available() && digitalRead(PIN_BOOT) == HIGH) delay(10);
  acelerar(0);
  while (Serial.available()) Serial.read();
  Serial.println("# Acelerador al mínimo. Esperá los pitidos que confirman el rango.");
}

void calibrar(float masa) {
  if (!taraHecha) {
    Serial.println("# Primero la tara, con la plataforma vacía: escribí tara.");
    return;
  }
  if (masa <= 0) {
    Serial.println("# Uso: calibrar 1000 (los gramos de lo que pusiste arriba de la plataforma)");
    return;
  }
  if (!celdaLista()) return;
  long lectura = celda.read_average(20) - celda.get_offset();
  if (abs(lectura) < 100) {
    Serial.println("# No veo el peso. Revisá los cables de la celda y que nada roce la plataforma.");
    return;
  }
  factor = lectura / masa;
  memoria.putFloat("factor", factor);
  Serial.printf("# Listo, factor %.2f guardado. Ya podés sacar el peso.\n", factor);
}

void leer() {
  if (factor == 0 || !taraHecha) {
    Serial.println("# Primero tara y calibrar.");
    return;
  }
  if (!celdaLista()) return;
  for (int i = 0; i < 10; i++) Serial.printf("# %.0f g\n", gramos(celda.read_average(5)));
}

void agua(String vaso) {
  if (vaso.length() == 0) vaso = "sin-nombre";
  digitalWrite(PIN_AGUA_VCC, HIGH);
  delay(300);
  long suma = 0;
  for (int i = 0; i < 20; i++) {
    int lectura = analogRead(PIN_AGUA);
    suma += lectura;
    Serial.printf("agua,%lu,%s,%d\n", millis(), vaso.c_str(), lectura);
    delay(100);
  }
  digitalWrite(PIN_AGUA_VCC, LOW);
  Serial.printf("# %s: promedio %ld (de 0 a 4095)\n", vaso.c_str(), suma / 20);
}

void ayuda() {
  Serial.println("# Comandos:");
  Serial.println("#   tara              pone la celda en cero, con el banco quieto");
  Serial.println("#   calibrar 1000     con una masa conocida arriba, en gramos");
  Serial.println("#   leer              muestra lo que pesa la celda ahora");
  Serial.println("#   barrido a         (o b o c) barrido del 30 al 100 %");
  Serial.println("#   agua canilla      lee el sensor de agua con el nombre del vaso");
  Serial.println("#   rango-esc         calibra el rango del ESC, una vez y sin hélice");
  Serial.println("# Durante el barrido, cualquier tecla o el botón BOOT cortan el motor.");
}

void ejecutar(String texto) {
  texto.trim();
  if (texto.length() == 0) return;
  String cmd = texto;
  String arg = "";
  int espacio = texto.indexOf(' ');
  if (espacio > 0) {
    cmd = texto.substring(0, espacio);
    arg = texto.substring(espacio + 1);
    arg.trim();
  }
  cmd.toLowerCase();
  arg.toLowerCase();

  if (cmd == "barrido" && (arg == "a" || arg == "b" || arg == "c")) barrido(arg[0]);
  else if (cmd == "tara") tara();
  else if (cmd == "calibrar") calibrar(arg.toFloat());
  else if (cmd == "leer") leer();
  else if (cmd == "agua") agua(arg);
  else if (cmd == "rango-esc") rangoEsc();
  else ayuda();
}

// Lo primero al prenderse: el acelerador al mínimo, y ahí se queda hasta que llega un comando.
void setup() {
  esc.setPeriodHertz(50);
  esc.attach(PIN_ESC, US_MINIMO, US_MAXIMO);
  acelerar(0);

  pinMode(PIN_BOOT, INPUT_PULLUP);
  pinMode(PIN_AGUA_VCC, OUTPUT);
  digitalWrite(PIN_AGUA_VCC, LOW);

  Serial.begin(115200);
  celda.begin(PIN_HX711_DT, PIN_HX711_SCK);
  memoria.begin("banco", false);
  factor = memoria.getFloat("factor", 0);

  delay(500);
  Serial.println("# Banco de Gavia listo. Acelerador al mínimo.");
  if (factor == 0) Serial.println("# La celda no está calibrada.");
  ayuda();
}

// Junta las letras hasta el Enter, así sirve tanto el script de la laptop como un monitor
// que manda tecla por tecla.
void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      ejecutar(linea);
      linea = "";
    } else {
      linea += c;
    }
  }
}
