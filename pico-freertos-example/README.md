# Pico 2 W DHT11 + TM1637 test

Target: Raspberry Pi Pico 2 W (RP2350, Arm Cortex-M33) with FreeRTOS.

The program reads four active-low DIP switches and a DHT11. When any DIP switch
is ON, the TM1637 shows the first active switch number and the LED blinks. When
all switches are OFF, the LED turns off and the TM1637 shows the DHT11 reading:

- `S1On`: DIP switch 1 is ON;
- `S2On`: DIP switch 2 is ON;
- `S3On`: DIP switch 3 is ON;
- `S4On`: DIP switch 4 is ON;
- `t27C`: temperature is 27 degrees Celsius;
- `H 55`: relative humidity is 55 percent;
- `Err`: DHT11 timeout or checksum failure.

On Pico 2 W, the same information can also be served over Wi-Fi as a small web
dashboard.

At startup the display shows progress:

- `8888`: all segments test, before FreeRTOS starts;
- `boot`: program reached the boot/setup stage;
- `run`: FreeRTOS task was created and the scheduler is starting/running;
- `S1On` / `S2On` / `S3On` / `S4On`: a DIP switch is ON;
- `rEAd`: DHT11 read is in progress;
- `t27C`: temperature is 27 degrees Celsius;
- `H 55`: relative humidity is 55 percent;
- `ErrN`: DHT11 diagnostic error code.

## Wiring

Power both modules from 3.3 V and connect all grounds together.

| Module | Module pin | Pico 2 W GPIO | Physical pin |
| --- | --- | --- | --- |
| DHT11 | VCC | 3V3 OUT | 36 |
| DHT11 | DATA | GPIO16 | 21 |
| DHT11 | GND | GND | 23 |
| DIP switch 1 | one side | GPIO17 | 22 |
| DIP switch 2 | one side | GPIO18 | 24 |
| DIP switch 3 | one side | GPIO19 | 25 |
| DIP switch 4 | one side | GPIO20 | 26 |
| DIP switches 1-4 | other side | GND | 23 or 28 |
| LED | GPIO side | GPIO21 through 220-1k Ohm resistor | 27 |
| LED | GND side | GND | 28 |
| TM1637 | VCC | 3V3 OUT | 36 |
| TM1637 | CLK | GPIO14 | 19 |
| TM1637 | DIO | GPIO15 | 20 |
| TM1637 | GND | GND | 18 |

For a bare DHT11, add a 4.7-10 kOhm resistor from DATA to 3.3 V. Most DHT11
modules already contain this pull-up.

The DIP switches use the Pico internal pull-ups. Wire each switch between its
GPIO and GND. OFF reads high; ON connects that GPIO to GND and reads low. If
your DIP board is labeled differently, the important part is: ON must short the
GPIO to GND, not to 3.3 V.

The LED is active-high: GPIO21 high turns it on. A typical connection is:
GPIO21 -> 220-1k Ohm resistor -> LED anode (+), LED cathode (-) -> GND.

The TM1637 driver uses open-drain signaling. Most modules contain CLK and DIO
pull-ups. If yours does not, add 4.7-10 kOhm pull-ups from both signals to
3.3 V.

Do not allow TM1637 CLK or DIO to be pulled up to 5 V; RP2350 GPIO is not
5 V tolerant.

## Build

```sh
cmake --preset pico2w
cmake --build --preset pico2w --clean-first
```

To enable the Wi-Fi web dashboard, configure with your 2.4 GHz Wi-Fi SSID and
password:

```sh
cmake --preset pico2w \
  -DWIFI_SSID="your-wifi-name" \
  -DWIFI_PASSWORD="your-wifi-password"
cmake --build --preset pico2w --clean-first
```

If `WIFI_SSID` is empty, the board still runs the local DHT11/TM1637/DIP/LED
test, but the web server is disabled.

Firmware:

```text
out/build/pico2w/pico_freertos_example.uf2
```

Hold BOOTSEL while connecting the Pico 2 W, copy the UF2, then unplug and
reconnect without holding BOOTSEL.

## Diagnostic output

USB CDC and UART0 GPIO0 transmit the same messages at 115200 baud.

```sh
screen /dev/ttyACM0 115200
```

Expected output:

```text
TM1637 startup test: CLK=GPIO14, DIO=GPIO15, ACK=yes
Starting FreeRTOS scheduler
DHT11/TM1637 test started: DHT11=GPIO16, TM1637 CLK=GPIO14 DIO=GPIO15, DIP=GPIO17/18/19/20 active-low, LED=GPIO21
DIP switches: mask=0x01, showing switch 1
DIP switches: all OFF, returning to DHT11 display
DHT11: temperature=27.5 C, humidity=55.3 %
TM1637: temperature display, ACK=yes
TM1637: humidity display, ACK=yes
Wi-Fi: connected, open http://192.168.1.123/
HTTP: server listening on port 80
```

If DHT11 values are printed but TM1637 reports `ACK=no`, check CLK/DIO order,
power, ground, and signal pull-ups.

## Wi-Fi web dashboard

After flashing a build with `WIFI_SSID` and `WIFI_PASSWORD`, open the URL shown
in the diagnostic output:

```text
http://<pico-ip-address>/
```

The page refreshes once per second and shows:

- DHT11 temperature and humidity;
- DHT11 error code/status when the sensor read fails;
- DIP switch 1-4 ON/OFF state;
- first active switch number;
- whether the external switch LED is blinking;
- uptime and Wi-Fi IP address.

The raw JSON endpoint is:

```text
http://<pico-ip-address>/api/status
```

Pico W / Pico 2 W Wi-Fi is 2.4 GHz only. If connection fails, check that the
SSID is a 2.4 GHz network and the password is correct.

## DHT11 error codes

- `Err2`: sensor never pulled DATA low after the start pulse. Check DATA pin,
  pull-up, VCC, and GND.
- `Err3`: sensor pulled low but did not release high. Check wiring or power.
- `Err4`: sensor response did not finish cleanly.
- `Err5`: data bit start timeout.
- `Err6`: data bit end timeout. Signal quality or pull-up is usually poor.
- `Err7`: checksum error. DATA line timing/noise problem.
