# Wiring guide

The controller-side signal connections are taken directly from the compiled firmware. The power topology reflects the installed 12 V source, relay board, 12-to-5 V buck converter, servo, heater, fan, and 2N2222 buzzer driver.

![Smart Egg Incubator wiring diagram](wiring-diagram.svg)

## Confirmed signal connections

| ESP32 | Connects to | Notes |
| --- | --- | --- |
| GPIO 4 | DHT11 `DATA` | A bare four-pin DHT11 normally needs a pull-up resistor; many three-pin modules include it |
| GPIO 18 | Servo signal | Servo receives 5 V from the buck converter |
| GPIO 21 | OLED `SDA` | SSD1306 I2C display at address `0x3C` |
| GPIO 22 | OLED `SCL` | SSD1306 I2C clock |
| GPIO 25 | Base resistor -> 2N2222 base | Low-side buzzer driver; zener provides transient protection |
| GPIO 26 | Relay board `IN1` / K1 | Heater channel, active-low |
| GPIO 27 | Relay board `IN2` / K2 | Fan channel, active-low |
| GND | Peripheral signal grounds | Required for non-isolated control signals |

## Power distribution

The 12 V source branches into two paths:

1. A 12 V branch powers the relay board and supplies the switched heater/fan circuits.
2. A second 12 V branch enters the buck converter. Its regulated 5 V output powers the servo.

Relay K1 switches the heater. Relay K2 switches the fan. The relay contacts carry load current; GPIO 26 and GPIO 27 only command the relay inputs.

The heater circuit should retain a correctly rated fuse and independent thermal cutoff in its 12 V series path.

## Buzzer driver

GPIO 25 does not drive the buzzer directly. It drives a 2N2222 NPN transistor through a base resistor:

- GPIO 25 -> base resistor -> 2N2222 base;
- 2N2222 emitter -> common ground;
- 2N2222 collector -> buzzer negative terminal;
- buzzer positive terminal -> its positive supply;
- zener protection across the buzzer/collector path, according to the installed orientation.

The diagram assumes the conventional protection orientation: zener cathode toward buzzer positive and anode toward the transistor collector. Confirm this against the actual build before treating that portion as final.

## Details still needed

- Relay-board model and terminal labels, including its exact 12 V power terminals.
- 12 V source current rating.
- Heater wattage/current and fan current.
- Buck-converter model/current rating and measured 5 V output.
- Servo model and stall current.
- Buzzer supply voltage/current.
- Base-resistor resistance, zener part/value, and actual zener orientation.
- Fuse and thermal-cutoff ratings.
- Confirmation that the ESP32, buck output, servo, buzzer driver, and relay input side share the required common ground.

## Before power-up

1. Disconnect both external loads.
2. Verify the relay board powers up with K1 and K2 off.
3. Confirm GPIO 26 operates only K1 and GPIO 27 only K2.
4. Adjust and measure the buck output at 5 V before connecting the servo.
5. Verify the common-ground connections required by the servo, transistor driver, and relay inputs.
6. Connect one load at a time through its fuse and cutoff protection.
7. Disconnect the DHT11 and confirm the heater and humidity outputs turn off and the alarm activates.
