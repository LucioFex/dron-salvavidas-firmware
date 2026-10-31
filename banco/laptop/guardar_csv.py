# /// script
# requires-python = ">=3.10"
# dependencies = ["pyserial>=3.5"]
# ///
"""Habla con el programa del banco por USB y guarda las mediciones en CSV.

Uso:
    uv run guardar_csv.py                 busca el ESP32 solo
    uv run guardar_csv.py COM5            en Windows
    uv run guardar_csv.py /dev/ttyUSB0    en Linux

Lo que escribís se manda al ESP32 (tara, calibrar 1000, barrido a, agua canilla...).
Durante un barrido, Enter corta el motor. Para salir: salir o Ctrl+C.
"""

import csv
import sys
import threading
from datetime import datetime
from pathlib import Path

CARPETA = Path(__file__).resolve().parent / "mediciones"
COLUMNAS = ["t_ms", "config", "paso", "fase", "acelerador_pct", "empuje_g"]


def resumir(filas):
    """Promedio del empuje en los últimos 5 s de cada paso (fase "midiendo")."""
    pasos = {}
    for fila in filas:
        if fila["fase"] == "midiendo":
            pasos.setdefault(int(fila["paso"]), []).append(fila)
    resumen = []
    for paso, lecturas in sorted(pasos.items()):
        empujes = [float(f["empuje_g"]) for f in lecturas]
        resumen.append({
            "paso": paso,
            "acelerador_pct": round(max(float(f["acelerador_pct"]) for f in lecturas)),
            "empuje_medio_g": round(sum(empujes) / len(empujes)),
            "lecturas": len(empujes),
        })
    return resumen


def cartel(texto):
    print("\n" + "=" * 44 + f"\n  {texto}\n" + "=" * 44, flush=True)


class Banco:
    """Interpreta las líneas que manda el ESP32 y escribe los archivos."""

    def __init__(self, carpeta=CARPETA):
        self.carpeta = carpeta
        self.archivo = None
        self.escritor = None
        self.ruta = None
        self.filas = []
        self.ultimo = None
        self.contador = 0

    def procesar(self, linea):
        if not linea:
            return
        if linea.startswith("#"):
            print(linea[1:].strip(), flush=True)
            return
        campos = linea.split(",")
        tipo = campos[0]
        if tipo == "inicio":
            self.empezar(campos[1], int(campos[2]))
        elif tipo == "paso":
            cartel(f"PASO {campos[1]} DE {campos[2]}   {campos[3]} %")
        elif tipo == "dato" and self.escritor:
            self.anotar(dict(zip(COLUMNAS, campos[1:])))
        elif tipo == "fin":
            self.terminar("completo")
        elif tipo == "corte":
            cartel(f"MOTOR CORTADO ({campos[1]})")
            self.terminar("cortado")
        elif tipo == "agua":
            self.anotar_agua(campos[1:])

    def empezar(self, config, pasos):
        self.carpeta.mkdir(exist_ok=True)
        self.ruta = self.carpeta / f"{datetime.now():%Y-%m-%d_%H%M}_config-{config}.csv"
        self.archivo = open(self.ruta, "w", newline="", encoding="utf-8")
        self.escritor = csv.writer(self.archivo)
        self.escritor.writerow(COLUMNAS)
        self.filas, self.ultimo, self.contador = [], None, 0
        cartel(f"BARRIDO {config.upper()}, {pasos} PASOS: Enter corta el motor")

    def anotar(self, fila):
        self.escritor.writerow(fila.values())
        self.filas.append(fila)
        clave = (fila["paso"], fila["fase"])
        if clave != self.ultimo:
            self.ultimo = clave
            print(f"  {fila['fase']}", flush=True)
        self.contador += 1
        if self.contador % 10 == 0:
            print(f"  {float(fila['acelerador_pct']):5.1f} %   {float(fila['empuje_g']):6.0f} g", flush=True)

    def terminar(self, como):
        if not self.archivo:
            return
        self.archivo.close()
        self.archivo = self.escritor = None
        resumen = resumir(self.filas)
        ruta_resumen = self.ruta.with_name(self.ruta.stem + "_resumen.csv")
        with open(ruta_resumen, "w", newline="", encoding="utf-8") as f:
            escritor = csv.DictWriter(f, fieldnames=["paso", "acelerador_pct", "empuje_medio_g", "lecturas"])
            escritor.writeheader()
            escritor.writerows(resumen)
        print(f"\nBarrido {como}. Datos en {self.ruta.name}, resumen en {ruta_resumen.name}")
        print(" paso   acelerador   empuje")
        for r in resumen:
            print(f" {r['paso']:4}   {r['acelerador_pct']:8} %   {r['empuje_medio_g']:6} g")
        print(flush=True)

    def anotar_agua(self, campos):
        self.carpeta.mkdir(exist_ok=True)
        ruta = self.carpeta / f"{datetime.now():%Y-%m-%d}_agua.csv"
        nuevo = not ruta.exists()
        with open(ruta, "a", newline="", encoding="utf-8") as f:
            escritor = csv.writer(f)
            if nuevo:
                escritor.writerow(["t_ms", "vaso", "lectura"])
            escritor.writerow(campos)


def elegir_puerto(list_ports):
    puertos = [p.device for p in list_ports.comports() if p.vid is not None]
    if len(puertos) == 1:
        return puertos[0]
    if not puertos:
        sys.exit("No encuentro el ESP32. ¿Está enchufado? En Linux, ¿estás en el grupo dialout?")
    sys.exit(f"Hay varios puertos: {', '.join(puertos)}. Elegí uno: uv run guardar_csv.py {puertos[0]}")


def main():
    import serial
    from serial.tools import list_ports

    puerto = sys.argv[1] if len(sys.argv) > 1 else elegir_puerto(list_ports)
    esp = serial.Serial()
    esp.port, esp.baudrate, esp.timeout = puerto, 115200, 0.2
    esp.dtr = esp.rts = False  # que abrir el puerto no deje al ESP32 en reset
    esp.open()
    print(f"Conectado a {puerto}. Escribí ayuda para ver los comandos.")

    banco = Banco()
    vivo = True

    def escuchar():
        while vivo:
            linea = esp.readline().decode("utf-8", errors="replace").strip()
            banco.procesar(linea)

    threading.Thread(target=escuchar, daemon=True).start()
    try:
        while True:
            texto = input()
            if texto.strip().lower() == "salir":
                break
            esp.write((texto + "\n").encode())
    except (KeyboardInterrupt, EOFError):
        esp.write(b"x\n")  # por si había un barrido andando
    vivo = False
    banco.terminar("cortado")
    esp.close()


if __name__ == "__main__":
    main()
