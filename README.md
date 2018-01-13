# No early wake-up
Never accept a child to wake you up too early in the morning

## Description

Your child should know if he/she can walk to your bedroom or if they have to keep calm and try to sleep again in his/her bedroom.
A child isn't aware of time reading and has _very_ limited time notions
Now with this a few pieces of electronics and an arduino, they will be able to know if it's time or not to slip into your room.

## Requirements

* Arduino uno (may work with other harware)
* DS1307 clock
* SSD1306 OLED screen 128x32 (bi-color)
* 220Ω resistor
* LED 


## Wiring

- clock
| Arduino | <=> | <=> | Clock |
| :---: | :---: | :---: | :---: |
| 5V | <= | => | 5V |
| Gnd | <= | => | Gnd |
| A4 | <= | => | SDA |
| A5 | <= | => | SCL |

- led
| Arduino | <=> | <=> | LED |
| :---: | :---: | :---: | :---: |
| D13 | <= | 220Ω | . |
| . | 220Ω | LED+ | . |
| . | . | LED- | Gnd |

- OLED Display
| Arduino | <=> | <=> | OLED |
| :---: | :---: | :---: | :---: |
| 3.3V | <= | => | VCC |
| Gnd | <= | => | Gnd |
| A4 | <= | => | SDA |
| A5 | <= | => | SCL |

- Push Button (Normally open)
| Arduino | <=> | <=> | Pushbutton |
| :---: | :---: | :---: | :---: |
| D4 | <= | => | + |
| Gnd | <= | => | - |

