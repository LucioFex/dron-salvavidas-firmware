# Programa del banco de empuje

El ESP32 del banco de un ducto ([gavia-hardware/cad/banco](https://github.com/LucioFex/gavia-hardware/tree/main/cad/banco)) hace esto:

- Sube el acelerador del motor del 30 al 100 %, de a 10.
- Corta solo a los 10 s de cada paso.
- Lee la celda de carga con el HX711 y el sensor de agua.
- Manda todo por USB a la laptop, que lo guarda en CSV.

Volts y amperes no se leen: los muestra el wattímetro y se filman con el celular durante el barrido.

Está pensado para alguien que nunca programó un ESP32. Si es tu caso, seguí los pasos en orden.

> Una hélice de 8 pulgadas a fondo corta un dedo. Mientras la batería esté conectada, nadie pone
> la mano cerca del ducto, todos con anteojos y detrás del banco. La batería se conecta última y se
> desconecta primera.

## Qué hace, paso por paso

1. Al prenderse, el acelerador queda al mínimo, y ahí se queda hasta que le llega un comando.
2. Con `barrido a` (o `b` o `c`, según la configuración del banco) pone la celda en cero y arranca
   8 pasos: 30, 40, 50 y así hasta el 100 %.
3. En cada paso sube en 1 s, se queda hasta completar 10 s y vuelve al mínimo. Después descansa 10 s
   y pasa al siguiente. Son unos 160 s en total.
4. El empuje de cada paso es el promedio de sus últimos 5 s, cuando ya se estabilizó.

Para cortar el motor en cualquier momento, apretá Enter en la laptop o el botón BOOT de la placa.
Si se desenchufa el USB, el ESP32 se apaga y el ESC corta solo porque deja de recibir señal.

## Qué hay en esta carpeta

| Archivo | Qué es |
|---|---|
| `src/main.cpp` | El programa del ESP32 |
| `platformio.ini` | La receta para compilarlo y subirlo |
| `laptop/guardar_csv.py` | Habla con el ESP32 y guarda los CSV en `laptop/mediciones/` |
| `laptop/comparar.py` | Compara las tres configuraciones y saca los números del dron |
| `diagram.json`, `wokwi.toml`, `libraries.txt` | El circuito para probarlo en el simulador Wokwi |

## 1. Instalar lo necesario (una vez)

Sirve igual en Windows y en Ubuntu.

1. [VS Code](https://code.visualstudio.com/).
2. Dentro de VS Code, la extensión PlatformIO IDE. Se busca en el ícono de extensiones (los cuatro
   cuadraditos de la izquierda). Tarda unos minutos la primera vez.
3. [uv](https://docs.astral.sh/uv/getting-started/installation/), que corre los scripts de la
   laptop y baja solo lo que necesitan:
   - en Ubuntu, `curl -LsSf https://astral.sh/uv/install.sh | sh`
   - en Windows, `powershell -c "irm https://astral.sh/uv/install.ps1 | iex"`.
4. En Ubuntu, permiso para usar el puerto USB: `sudo usermod -aG dialout $USER`. Después hay que
   cerrar la sesión y volver a entrar.
5. En Windows, si la placa no aparece al enchufarla, falta el driver del chip USB. Mirá el chip
   chiquito al lado del conector: si dice CH340, el driver es el de WCH, y si dice CP2102, el de
   Silicon Labs.

## 2. Probarlo sin motor

Son tres escalones. Cada uno se hace recién cuando el anterior anduvo bien.

### Escalón 1: en el simulador, sin comprar nada

[Wokwi](https://wokwi.com) simula el ESP32, la celda y un servo que hace de ESC (el brazo del servo
gira como giraría el motor). Un potenciómetro hace de sensor de agua.

En el navegador:

1. En wokwi.com, crear un proyecto nuevo de ESP32 con Arduino.
2. Pegar el contenido de `src/main.cpp` en `sketch.ino`.
3. Reemplazar `diagram.json` por el de esta carpeta.
4. Agregar un archivo `libraries.txt` con el contenido del de esta carpeta.
5. Darle a Play. En el monitor serie de abajo aparece "Banco de Gavia listo".

En VS Code, con la extensión Wokwi Simulator, se compila con PlatformIO (el tilde de la barra de
abajo), se abre `diagram.json` y se le da a Play.

Qué probar:

- Escribir `tara`. Con la simulación andando, hacer clic en la celda para cambiarle el peso,
  ponerle 1 kg y escribir `calibrar 1000`. Después `leer` tiene que mostrar unos 1000 g. Si tu
  versión de Wokwi no muestra ese control, saltealo: la celda se prueba en el escalón 2.
- `barrido a`: el servo tiene que ir subiendo de a pasos y volver a cero entre uno y otro. Escribir
  cualquier letra y Enter en el medio: el servo vuelve a cero y aparece `corte,tecla`.
- `agua prueba` con el potenciómetro en distintas posiciones: el promedio tiene que cambiar.

El simulador no guarda los CSV. Para eso está el escalón 2.

### Escalón 2: el ESP32 de verdad con la celda, sin ESC

1. Conectar solo la celda y el HX711 (ver "Conexiones").
2. Enchufar el ESP32 por USB. En VS Code, abrir esta carpeta (`banco/`) y apretar la flecha de la
   barra de abajo de PlatformIO (Upload). Cuando termina dice SUCCESS.
3. En una terminal, desde `banco/laptop/`: `uv run guardar_csv.py`. Tiene que decir "Conectado a"
   y el nombre del puerto.
4. Calibrar la celda (ver abajo) y hacer un `barrido a` sin motor. Sale un CSV en
   `laptop/mediciones/` con el empuje cerca de cero en todos los pasos. Si apretás la plataforma con
   la mano en el medio, el número tiene que subir.

### Escalón 3: ESC y motor, sin hélice

Es el paso 11 del armado en el roadmap. Con el motor atornillado y sin hélice, calibrar el rango del
ESC (ver abajo) y hacer un `barrido a`. El motor tiene que girar suave, más rápido en cada paso, y
pararse entre uno y otro. Recién después va la hélice.

## Conexiones

Son dos circuitos separados, como en el diagrama de la página 11 del roadmap. El de potencia
(batería, wattímetro, ESC y motor) va con cable grueso y fichas EC5. Este es el de datos, con
cables finitos:

| Desde | Hasta el ESP32 | Nota |
|---|---|---|
| ESC, cable de señal (blanco o amarillo) | GPIO 25 | |
| ESC, masa (negro o marrón) | GND | |
| ESC, rojo de 5 V | nada | El ESP32 ya toma corriente del USB. Se aísla con cinta |
| HX711 VCC | 3V3 | |
| HX711 GND | GND | |
| HX711 DT | GPIO 16 | |
| HX711 SCK | GPIO 4 | |
| Sensor de agua VCC | GPIO 27 | El programa lo prende solo mientras lee |
| Sensor de agua GND | GND | |
| Sensor de agua AO | GPIO 34 | La salida analógica. DO no se usa |

La celda trae cuatro cables (casi siempre rojo, negro, verde y blanco) que van a E+, E-, A+ y A- del
HX711. Los colores pueden variar: mirá la etiqueta de la celda.

## Calibrar la celda (una vez, y cada vez que se desarme)

En el banco, el motor sopla para arriba y empuja la plataforma hacia abajo. Por eso el empuje se
mide igual que un peso, y se calibra con una masa conocida.

1. Pesar algo en la balanza de la cocina, por ejemplo una botella con agua de unos 1000 g. Anotar
   lo que marca.
2. Con la plataforma armada y vacía, escribir `tara`.
3. Apoyar la botella en el centro de la plataforma y escribir `calibrar 1000`, con los gramos que
   marcó la balanza.
4. Sacar la botella. `leer` tiene que dar cerca de 0, y con la botella arriba otra vez, lo que pesa.

El factor queda guardado en el ESP32 aunque se desenchufe. Cada barrido hace su propia tara al
arrancar, así que todo lo que está arriba de la celda (ducto, rejillas, motor) no cuenta.

## El rango del ESC (una vez, sin hélice)

El programa manda el acelerador como un pulso de 1000 µs (mínimo) a 2000 µs (a fondo). Muchos ESC
con BLHeli vienen de fábrica con otro rango, por ejemplo de 1148 a 1832 µs (aprox., a validar con el
manual del EMAX). Si no se calibra, el 30 % del programa no es el 30 % del ESC, y la comparación con
la tabla de Hobbywing queda corrida. Las relaciones entre a, b y c no cambian, porque las tres se
miden igual.

1. Sin hélice y con la batería desconectada, escribir `rango-esc`.
2. Conectar la batería. El ESC hace unos pitidos.
3. Apretar Enter. El programa pasa al mínimo y el ESC confirma con otros pitidos.

La secuencia exacta de pitidos depende del ESC: mirá su manual. Si algo no coincide, desconectá la
batería y repetí.

## 3. Medir

Con el banco armado según el roadmap, la batería en la bolsa ignífuga y el probador de LiPo
enchufado al cable de balanceo (alarma en 3,5 V por celda):

1. `uv run guardar_csv.py` desde `banco/laptop/`.
2. Poner el celular filmando a la vez el wattímetro y la pantalla de la laptop. La pantalla muestra
   el paso en letra grande, así después se sabe qué volts y amperes corresponden a cada paso.
3. Conectar la batería, esperar los pitidos del ESC y escribir `barrido a`.
4. Al terminar, el script muestra el resumen y lo guarda como `..._config-a_resumen.csv`.
5. Desconectar la batería, tocar el motor (si no se aguanta el dedo, esperar a que se enfríe) y
   cambiar a la configuración b. Lo mismo para la c.

Las tres configuraciones son las del roadmap:

| | Configuración | Qué nos dice |
|---|---|---|
| a | Hélice libre, sin el tubo | Si el motor empuja lo que dice Hobbywing (2.771 g al 100 %) |
| b | Con el tubo y el labio | Cuánto suma el ducto (b/a) |
| c | Tubo, labio y las dos rejillas | Cuánto quitan las rejillas (c/b). Es el número que importa |

Después:

```
uv run comparar.py mediciones/<fecha>_config-a_resumen.csv mediciones/<fecha>_config-b_resumen.csv mediciones/<fecha>_config-c_resumen.csv
```

Muestra b/a y c/b en cada paso, los tres valores para `gavia_aro.py` (`kg_por_motor`, `k_ducto` y
`k_rejilla`) y si la configuración c llega a los 1,8 kg por motor al 100 %, que es la regla para
seguir con este motor.

Los CSV usan punto decimal. Para abrirlos en una planilla configurada en castellano, importarlos
indicando que el separador decimal es el punto.

## El sensor de agua

En la mesa de la cocina, con los electrodos de varilla inoxidable (página 12 del roadmap):

1. `uv run guardar_csv.py`.
2. Electrodos en el primer vaso y `agua lluvia`. Enjuagar y secar.
3. Lo mismo con `agua canilla`, `agua pileta` y `agua mar` (35 g de sal por litro).

Cada lectura queda en `mediciones/<fecha>_agua.csv` con el nombre del vaso, y el script muestra el
promedio. En la mayoría de los módulos LM393, el número baja cuanto más conduce el agua (aprox.,
depende del módulo). Buscamos un número fijo que deje la lluvia de un lado y el mar del otro. El
vaso de pileta importa porque las primeras demos van a ser ahí.

## Si algo falla

| Pasa esto | Probá esto |
|---|---|
| "No encuentro el ESP32" | Otro cable USB (muchos solo cargan y no pasan datos). En Ubuntu, el grupo `dialout` |
| "Hay varios puertos" | Pasar el puerto: `uv run guardar_csv.py COM5` o `/dev/ttyUSB0` |
| Upload falla con "Failed to connect" | Mantener apretado BOOT mientras empieza a subir y soltarlo cuando aparece "Connecting" |
| "No encuentro el HX711" | Revisar DT, SCK, VCC y GND. Que DT y SCK no estén cruzados |
| El empuje da negativo | El motor sopla para abajo y levanta la plataforma. Invertir el giro cambiando dos de los tres cables del motor, o revisar que la hélice esté del lado correcto |
| El motor no arranca y el ESC pita cada tanto | No ve la señal. Revisar el cable de señal y la masa en común |
| El motor arranca recién en un paso alto | Falta calibrar el rango del ESC |

## Cómo se relaciona con la wiki

El banco mide los tres supuestos del modelo del aro que nadie midió todavía. El plan y la regla de
decisión están en `18_plan_de_prototipo.md` de la wiki del proyecto, y el armado en el roadmap
(`gavia-hardware/docs/roadmap/`).
