# gavia-firmware

Parte de Gavia, un dron salvavidas autónomo con IA.

Firmware de la botonera del guardavidas (ESP32), el programa del banco de empuje y la configuración
de ArduPilot para el dron salvavidas autónomo.

> Prototipo experimental en desarrollo. No es un equipo de rescate certificado y no reemplaza a
> un guardavidas. Las pruebas de vuelo se hacen siempre con un piloto de seguridad que puede tomar el
> control por radio.

## Qué hay acá

### `botonera/`: la botonera del guardavidas

Los guardavidas no usan celular en servicio y se meten al agua seguido. Por eso el aviso llega a una
botonera fija en la torre, conectada por cable a la computadora que corre la detección.

- Una pantalla chica muestra la probabilidad de ahogamiento, el sector y la cuenta regresiva para
  que el dron se pose al costado de la persona.
- Hay dos botones grandes: Incidente real (rojo), que hace que el dron se pose en el momento (y sirve
  también para despegarlo a mano), y Falso positivo (gris), que lo manda de vuelta a la base.
- Suena un buzzer y se enciende una baliza cuando hay una alerta.

Se arma con un ESP32, una pantalla OLED, dos botones tipo arcade y una caja estanca. El proyecto usa
[PlatformIO](https://platformio.org/).

### `banco/`: el banco de empuje

El ESP32 del banco de un ducto sube el acelerador del 30 al 100 %, corta solo a los 10 s de cada paso,
lee la celda de carga y el sensor de agua, y manda todo a la laptop, que lo guarda en CSV. Se puede
probar en el simulador Wokwi sin tener nada comprado. Instrucciones para quien nunca programó un
ESP32 en [`banco/README.md`](banco/README.md).

### `ardupilot/`: configuración del dron

El dron usa una controladora de vuelo con [ArduPilot](https://ardupilot.org/), que resuelve la
estabilización, el GPS, las misiones y los failsafes (volver a la base si se corta el enlace o baja
la batería). Acá van los parámetros, los scripts y las notas de configuración.

## Estado

En diseño. El programa del banco compila y está listo para probar. La botonera tiene un esqueleto
que compila y todavía no hace nada útil.

## Repos del proyecto

- [gavia-vision](https://github.com/LucioFex/gavia-vision): detección y lógica de decisión
- [gavia-firmware](https://github.com/LucioFex/gavia-firmware): botonera ESP32, banco de empuje y configuración de ArduPilot (este repo)
- [gavia-hardware](https://github.com/LucioFex/gavia-hardware): modelos 3D y lista de materiales

## Licencia

[Apache-2.0](LICENSE). Copyright 2026 Luciano Esteban y Elías Espinosa.

## Autores

- Luciano Esteban ([@LucioFex](https://github.com/LucioFex))
- Elías Espinosa
