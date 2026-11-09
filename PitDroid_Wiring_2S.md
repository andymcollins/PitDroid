# Pit Droid Wiring — 2S LiPo + RD-03D + 5 Servos + 3 LED Rings

```mermaid
flowchart LR
  LIPO["2S LiPo 7.4V<br/>(8.4V full)<br/>+ fuse + switch"]
  BUCK5["Buck A 5V<br/>(logic: ESP32 VIN + radar + LEDs)"]
  BUCK6["Buck B 6V<br/>(XL4015, servo rail)"]
  ESP["ESP32 WROOM-32<br/>VIN GND<br/>GPIO21 SDA<br/>GPIO22 SCL<br/>GPIO16 RX (radar)<br/>GPIO4 LED data"]
  RADAR["RD-03D mmWave<br/>(chest, metal shield behind)<br/>5V GND TX"]
  PCA["PCA9685<br/>VCC GND SDA SCL<br/>V+ V−<br/>CH0 pan CH1 tilt<br/>CH2-3 arms CH4 hip"]
  SERVOS["5x MG996R<br/>pan/tilt/2 arms/hip"]
  LEDS["3x LED rings<br/>(chained, eye)"]

  LIPO -->|"+ 7.4V (Red)"| BUCK5
  LIPO -->|"− GND (Black)"| BUCK5
  LIPO -->|"+ 7.4V (Red)"| BUCK6
  LIPO -->|"− GND (Black)"| BUCK6

  BUCK5 -->|"5V → VIN (Orange)"| ESP
  BUCK5 -->|"GND (Black)"| ESP
  BUCK5 -->|"5V (Orange)"| RADAR
  BUCK5 -->|"GND (Black)"| RADAR
  BUCK5 -->|"5V (Orange)"| LEDS
  BUCK5 -->|"GND (Black)"| LEDS

  BUCK6 -->|"6V → PCA V+ (Red thick)"| PCA
  BUCK6 -->|"GND → PCA V− (Black thick)"| PCA

  ESP -->|"3V3 logic (Orange)"| PCA
  ESP -->|"GND logic (Black)"| PCA
  ESP -->|"SDA GPIO21 (Blue)"| PCA
  ESP -->|"SCL GPIO22 (Green)"| PCA

  RADAR -->|"TX → GPIO16 RX (Purple)"| ESP

  ESP -->|"GPIO4 → DIN (Yellow)"| LEDS

  PCA -->|"CH0-CH4 signal (Cyan)"| SERVOS
  PCA -->|"V+ 6V (Red)"| SERVOS
  PCA -->|"V− GND (Black)"| SERVOS

  style LIPO fill:#ffe6cc
  style BUCK5 fill:#ffe6cc
  style BUCK6 fill:#ffe6cc
  style ESP fill:#dae8fc
  style RADAR fill:#fff2cc
  style PCA fill:#d5e8d4
  style SERVOS fill:#d5e8d4
  style LEDS fill:#f8cecc
```

## Notes

- **Common GND**: LiPo −, Buck A −, Buck B −, ESP32, PCA, radar, LEDs all tied together.
- **Metal shield behind RD-03D**, plastic chest plate only in front, rigid mount.
- **Do NOT power servos from Buck A** — servo rail is Buck B only.
- **Neck cable**: 20 AWG servo power, 22–24 AWG signal/radar/LED, silicone insulation.
