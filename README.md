# gavia-firmware

Parte de Gavia, un dron salvavidas autónomo con IA.

Firmware de la botonera del guardavidas (ESP32) y configuración de ArduPilot para el dron salvavidas
autónomo.

> Prototipo experimental en desarrollo. No es un equipo de rescate certificado y no reemplaza a
> un guardavidas. Las pruebas de vuelo se hacen siempre con un piloto de seguridad que puede tomar el
> control por radio.

## Qué hay acá

### `botonera/`: la botonera del guardavidas

Los guardavidas no usan celular en servicio y se meten al agua seguido. Por eso el aviso llega a una
botonera fija en la torre, conectada por cable a la computadora que corre la detección.

- Una pantalla chica muestra la probabilidad de ahogamiento, el sector y la cuenta regresiva para
  soltar el flotador.
- Hay dos botones grandes: Incidente real (rojo), que suelta el flotador en el momento, y
  Falso positivo (gris), que manda al dron de vuelta a la base.
- Suena un buzzer y se enciende una baliza cuando hay una alerta.

Se arma con un ESP32, una pantalla OLED, dos botones tipo arcade y una caja estanca. El proyecto usa
[PlatformIO](https://platformio.org/).

### `ardupilot/`: configuración del dron

El dron usa una controladora de vuelo con [ArduPilot](https://ardupilot.org/), que resuelve la
estabilización, el GPS, las misiones y los failsafes (volver a la base si se corta el enlace o baja
la batería). Acá van los parámetros, los scripts y las notas de configuración.

## Estado

En diseño. La botonera tiene un esqueleto que compila y todavía no hace nada útil.

## Repos del proyecto

- [gavia-vision](https://github.com/LucioFex/gavia-vision): detección y lógica de decisión
- [gavia-firmware](https://github.com/LucioFex/gavia-firmware): botonera ESP32 y configuración de ArduPilot (este repo)
- [gavia-hardware](https://github.com/LucioFex/gavia-hardware): modelos 3D y lista de materiales

## Licencia

[Apache-2.0](LICENSE). Copyright 2026 Luciano Esteban y Elías Espinosa.

## Autores

- Luciano Esteban ([@LucioFex](https://github.com/LucioFex))
- Elías Espinosa
