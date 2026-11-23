# Plan — Proyecto Final Microprocesadores (Programa Monitor)

Tarjeta: **NUCLEO-G431KB** (STM32G431KB, Cortex-M4) montada en la **unidad básica diseñada para NUCLEO-F303K8**
(`uPs_2018_SCH_.pdf`). Enunciado: `Proyecto_Final_2025.pdf`.

Flujo de trabajo acordado: **una función a la vez → se programa → se carga → se prueba en CoolTerm → siguiente.**
No se avanza a la siguiente hasta que la anterior funcione.

---

## 0. Reglas para mí (Claude) — leer antes de tocar código

- El usuario escribió a mano `main.c` y `basics.s` y los tiene que explicar al profesor. **No reescribir su código ni
  cambiar su estilo.** Solo agregar `else if` nuevos en `main.c` y módulos nuevos (`buzzer.c`, `rgb.c`, ...).
- **Pocos comentarios.** Solo los necesarios, cortos, en español. Nada de bloques de explicación largos.
- Estilo: C simple, registros CMSIS (`GPIOB->MODER`, `TIM16->ARR`...), sin HAL, sin CubeMX.
- `basics.s` (RD/RM con `TBB`) se queda **como lo hizo el usuario**. RD toma la foto de los registros en el
  momento del RD (el profe compara eso); RM escribe el registro físico. No "arreglar" esa semántica.
- Cada comando nuevo (verde): `xxx.h` + `xxx.c` en **`G4_Test1/mediums/`**, agregarlos a
  `G4_Test1/G4_Test1.cproject.yml` (grupo `Mediums`, ruta `./mediums/xxx.c`), en `main.c`
  `#include "mediums/xxx.h"`, y agregar la línea correspondiente al `HELP`.
- Compilar: `cbuild G4_Test1.csolution.yml -c G4_Test1.Debug+STM32G431KBTx --packs`
- Cargar (ST-Link USB conectado):
  `pyocd load --pack C:/Users/josei/AppData/Local/Arm/Packs/.Download/Keil.STM32G4xx_DFP.1.4.0.pack -t stm32g431kbtx out/G4_Test1/STM32G431KBTx/Debug/G4_Test1.hex`
  (el task "CMSIS Load+Run" de VS Code también funciona ahora que la carpeta no tiene espacios).
- Si algo que dice este plan resulta falso en la tarjeta, **corregir este archivo**.
- Documentos en la carpeta del proyecto: `STM32G4_User_Manual.pdf` (UM2397, pinout de la Nucleo), `STM32G4 DataSheet.pdf`
  (DS12589, tabla de AF págs. 61–64), `uPs_2018_SCH_.pdf` (esquemático), `Proyecto_Final_2025.pdf` (enunciado).
  Reference manual (registros): `C:/Users/josei/AppData/Local/Arm/Packs/Keil/STM32G4xx_DFP/1.4.0/Documentation/RM0440.pdf`.

---

## 1. Estado actual (hecho y probado)

| Item | Estado |
|---|---|
| Comandos amarillos RD, RM, MD, MM, BF, RUN, CALL, HELP | ✅ funcionando |
| UART del monitor | ✅ **USART1, PA9 (TX, D1) / PA10 (RX, D0)** → FTDI. Driver propio `USART.c` (antes era `G4_Test1.lib` en USART2) |
| Reloj | HSI 16 MHz, sin PLL (`system_stm32g4xx.c` plantilla). `SystemCoreClock = 16000000`. **Todos los timers/periféricos corren a 16 MHz.** |
| Stack | 4 KB (`Device/G4_Test1.sct`), heap 3 KB |
| Carga | pyocd + DFP pack (arriba) |

Notas del hardware ya aprendidas:
- La batería de 9 V debe estar arriba de ~7 V con carga (VIN de la Nucleo pide 7–12 V).
- El ST-Link VCP (COM5/COM6) usa PA2/PA3 = A7/A2. En el circuito A7 = POT y A2 = KEY6, por eso el monitor va por el FTDI.
  **Con el USB del ST-Link conectado, el TX del ST-Link mete señal en PA3 (KEY6)** → probar el teclado sin USB.
- Flujo de uso: cargar por USB del ST-Link (no hace falta sacar la tarjeta del zócalo) → desconectar USB →
  batería + FTDI → CoolTerm 115200 8N1 en el COM del FTDI → reset.

---

## 2. Mapa de pines (esquemático → Nucleo → MCU)

Fuentes: **UM2397 Rev 2, Tabla 11** (`STM32G4_User_Manual.pdf`, pág. 22) para Nucleo → MCU, y
**DS12589 Rev 6, Tabla 13** (`STM32G4 DataSheet.pdf`, págs. 61–64) para los números de AF. Todo ✅ = confirmado en PDF.

| Pin U5 | Nucleo | Señal del circuito | MCU (G431KB) | Función que vamos a usar (AF confirmado) |
|---|---|---|---|---|
| 1 | D1 | RXD (FTDI) | PA9 ✅ | USART1_TX **AF7** (probado) |
| 2 | D0 | TXD (FTDI) | PA10 ✅ | USART1_RX **AF7** (probado) |
| 5 | D2 | IRQ1 (U2 INTB) | PA12 ✅ | entrada GPIO |
| 6 | D3 | KEY3 | PB0 ✅ | entrada GPIO (teclado col1) |
| 7 | D4 | SDA | PB7 ✅ | I2C1_SDA **AF4** |
| 8 | D5 | SCL | PA15 ✅ | I2C1_SCL **AF4** → **I2C por hardware sí se puede** |
| 9 | D6 | KEY1 | PB6 ✅ | entrada GPIO (teclado col2) |
| 10 | D7 | N.C. | PF0 ✅ | — (compartido con OSC_IN, SB8/SB13) |
| 11 | D8 | KEY5 | PF1 ✅ | entrada GPIO (teclado col3) — compartido con OSC_OUT por SB11 ⚠️ |
| 12 | D9 | SIGNAL3 (LED_R / L_LED por J4) | PA8 ✅ | TIM1_CH1 **AF6** (RGB) / GPIO (luz LCD) |
| 13 | D10 | SIGNAL2 (LED_G / D/C por J2) | PA11 ✅ | TIM1_CH4 **AF11** (RGB) / GPIO (D/C LCD) |
| 14 | D11 | SIGNAL1 (LED_B / MOSI por J3) | PB5 ✅ | TIM3_CH2 **AF2** (RGB) / SPI1_MOSI **AF5** (LCD) |
| 15 | D12 | BUZZER | PB4 ✅ | TIM16_CH1 **AF1** (alternativa TIM3_CH1 AF2) |
| 16 | D13 | SCLK (LCD) | PB3 ✅ | SPI1_SCK **AF5** |
| 19 | A0 | KEY2 | PA0 ✅ | salida GPIO (teclado fila1) |
| 20 | A1 | KEY4 | PA1 ✅ | salida GPIO (teclado fila4) |
| 21 | A2 | KEY6 | PA3 ✅ | salida GPIO (teclado fila3) — también VCP RX (SB12) |
| 22 | A3 | KEY7 | PA4 ✅ | salida GPIO (teclado fila2) |
| 23 | A4 | DAC | PA5 ✅ | DAC1_OUT2 (analógico) — puede estar unido a PB7 por SB2 ⚠️ |
| 24 | A5 | N.C. | PA6 ✅ | — puede estar unido a PA15 por SB3 ⚠️ |
| 25 | A6 | IRQ0 (U4 INTB) | PA7 ✅ | entrada GPIO |
| 26 | A7 | POT | PA2 ✅ | ADC1_IN3 (analógico) — también VCP TX (SB1) |
| 28 | NRST | NRST | PG10-NRST | también resetea la LCD |
| 30 | VIN | VIN | — | batería **7–12 V, máx 800 mA** (UM2397 Tabla 5) |

Puentes de soldadura que nos afectan (UM2397 Tabla 9, pág. 17). El PDF marca el estado de fábrica en
**negritas**, y eso no se puede leer en el texto extraído → **se confirma con continuidad en la Fase 0**.

| SB | Si está ON | Qué nos importa |
|---|---|---|
| SB2 | PB7 (SDA) unido al pin A4 | **A4 = DAC y D4 = SDA quedarían en corto.** Lo necesitamos **OFF** |
| SB3 | PA15 (SCL) unido al pin A5 | A5 está N.C. en el circuito; si está ON, PA6 se deja como entrada flotante (reset) y listo |
| SB1 | T_VCP_TX unido a PA2 | PA2 = POT. Al ser entrada analógica no pasa nada |
| SB12 | T_VCP_RX unido a PA3 | PA3 = KEY6. Con USB del ST-Link conectado, el ST-Link maneja ese pin → probar teclado sin USB |
| SB14 | pin A2 unido a PA3 | debe estar ON (si no, KEY6 no llega al micro) |
| SB11 | PF1 unido al pin D8 | debe estar ON (si no, KEY5 no llega al micro) |
| SB4 | SWO del ST-Link en PB3 | PB3 = SCLK. SWO es entrada del ST-Link, no estorba |

Periféricos del circuito:
- **U2 MCP23017 (GPIO EXPANSION PORT 1):** GPB0–GPB7 = 8 switches (S1–S4, S9–S12). GPA0–GPA7 = LEDs D8…D1
  (GPA7 = D1, GPA0 = D8) con R4 100 Ω y **ánodo a 3V3 → LED encendido con 0 (activo en bajo)**. INTB → IRQ1 (D2).
- **U4 MCP23017 (GPIO EXPANSION PORT 0):** GPA0..GPA7 = segmentos A,B,C,D,E,F,G,DP (por R6 100 Ω).
  GPB3 = DIGIT1, GPB2 = DIGIT2, GPB1 = DIGIT3, GPB0 = DIGIT4 (por 22 kΩ a NPN 2N3904 → **cátodo común**, segmento y
  dígito activos en alto). GPB7..GPB4 = botones S5..S8 (PB0..PB3 del esquemático). INTB → IRQ0 (A6).
- **Dirección I2C de U2 y U4:** depende de A0–A2, no se lee bien en el PDF → **se averigua con un escaneo I2C (Fase 6)**.
  Rango posible 0x20–0x27.
- **Teclado 4x3 (P4):** fila1 (1,2,3) = KEY2, fila2 (4,5,6) = KEY7, fila3 (7,8,9) = KEY6, fila4 (*,0,#) = KEY4.
  col1 (1,4,7,*) = KEY3, col2 (2,5,8,0) = KEY1, col3 (3,6,9,#) = KEY5. (Tabla del esquemático: "1: 2+3" = KEY2+KEY3, etc.)
- **LED RGB (P5):** ánodo común a 5V, cada cátodo por 100 Ω a un NPN; base por 22 kΩ desde LED_R/LED_G/LED_B.
  **PWM activo en alto** (más duty = más brillo).
- **Buzzer:** 5V → buzzer → NPN Q7, base por 1 kΩ desde BUZZER (D12). Activo en alto, onda cuadrada = sonido.
- **Nokia 5110 (PCD8544, SPI):** SCE a GND (siempre seleccionada), RST = NRST de la tarjeta, D/C por J2 → SIGNAL2 (D10),
  MOSI por J3 → SIGNAL1 (D11), SCLK = D13, luz L_LED por J4 → SIGNAL3 (D9).
- **⚠️ Jumpers J2/J3/J4:** SIGNAL1/2/3 son **compartidos** entre RGB y LCD. Con los jumpers en RGB no hay LCD y viceversa.
  No se pueden usar los dos al mismo tiempo. (Para demostrar: cambiar jumpers.)
- **POT:** 10 kΩ entre 3V3 y GND, cursor → A7 (PA2).
- **DAC:** A4 → P6 y por J5 → VOUT.

---

## 3. Puntos (para decidir qué hacer)

Objetivo realista: **150 %** = 35 (amarillos) + 115 en verdes + 10 netos (naranja obligatorio).

| Comando | Pts | Decisión |
|---|---|---|
| Línea de comando (amarillos) | 35 | ✅ hecho |
| SOUND / MUTE | 10 | hacer |
| RGBOUT | 10 | hacer |
| VOUT | 10 | hacer (anula AOUT, no hacer AOUT) |
| VIN | 10 | hacer (anula AIN, no hacer AIN) |
| LCD | 5 | hacer |
| SEGMENT | 10 | hacer |
| IOMAP / IOUNMAP | 10 | hacer |
| POT2SEG | 25 | hacer — **no hacer POT2LCD** (lo anula) ni POT2LED |
| LCDWATCH | 25 | hacer — **no hacer SEGWATCH ni LEDWATCH** (los anula) |
| Naranja (obligatorio, sin él = 0) | 5–10 netos | **elegir uno** (ver Fase 11) |

---

## Fase 0 — Revisar puentes de soldadura (≈10 min, antes de programar periféricos)

El pinout ya está confirmado con UM2397. Solo falta saber cómo vienen de fábrica SB2, SB3, SB11 y SB14.

Paso 1: SB2 y SB3 (continuidad, tarjeta **fuera del zócalo** y sin alimentar)
- Multímetro en continuidad.
- Medir pin **A4 ↔ D4**: si pita → SB2 ON → **DAC y SDA en corto**. Hay que quitar SB2 (desoldar el puente en la
  cara de abajo de la tarjeta) o no usar DAC e I2C al mismo tiempo.
- Medir pin **A5 ↔ D5**: si pita → SB3 ON. No pasa nada (A5 está N.C.), solo no configurar PA6 como salida.
- Anotar resultado en la bitácora.

Paso 2: SB11 (PF1 → D8 = KEY5) y SB14 (PA3 → A2 = KEY6), con el monitor y el multímetro (tarjeta alimentada,
**USB del ST-Link desconectado** para que no maneje PA3).
- Reloj de GPIOA y GPIOF: `MD 0x4002104C 0x4002104C` (RCC_AHB2ENR) → `MM 0x4002104C <leído | 0x21> 4`
  (bit0 = GPIOA, bit5 = GPIOF).
- **PF1 como salida:** `MD 0x48001400 0x48001400` (GPIOF_MODER) → bits [3:2] = 01 →
  valor = (leído & ~0xC) | 0x4 → `MM 0x48001400 <valor> 4`
- Prender PF1: `MM 0x48001418 0x00000002 4` (GPIOF_BSRR bit1) → medir **3.3 V en D8**. Apagar: `MM 0x48001418 0x00020000 4`.
  Si no cambia → SB11 OFF → KEY5 no llega (soldar SB11 o mover KEY5 a otro pin).
- **PA3 como salida:** `MD 0x48000000 0x48000000` (GPIOA_MODER) → bits [7:6] = 01 →
  valor = (leído & ~0xC0) | 0x40 → `MM 0x48000000 <valor> 4`
- Prender PA3: `MM 0x48000018 0x00000008 4` → medir **3.3 V en A2**. Apagar: `MM 0x48000018 0x00080000 4`.
  Si no cambia → SB14 OFF.
- ⚠️ **En GPIOA_MODER solo cambiar los 2 bits del pin.** No tocar PA9/PA10 (bits 18–21 = UART del monitor) ni
  PA13/PA14 (bits 26–29 = SWD) o se pierde el monitor o la carga. Por eso siempre `MD` primero y luego `MM`.
- Al terminar: reset y todo vuelve a como estaba.

Direcciones útiles: GPIOA 0x48000000, GPIOB 0x48000400, GPIOF 0x48001400. MODER +0x00, IDR +0x10, ODR +0x14,
BSRR +0x18 (escribir 1 en bit n = prender, en bit n+16 = apagar; no hace falta leer antes).

Concepto importante (G4): al reset casi todos los pines están en **modo analógico (MODER = 11)**. Por eso para
configurar **siempre se limpia primero y luego se pone**:
```c
GPIOB->MODER &= ~(3U << (0*2));   // limpiar
GPIOB->MODER |=  (1U << (0*2));   // 01 = salida
```
Con solo `|=` el pin se quedaría en 11 (analógico).

Prueba: SB2/SB3/SB11/SB14 anotados en la bitácora. SB2 tiene que quedar OFF antes de la Fase 3 (DAC) y la Fase 6 (I2C).

---

## Base común (se hace una vez, antes o junto con Fase 1)

### Base de tiempo SysTick (1 ms)
Se necesita para retardos (`delay_ms`) y para las tareas en paralelo (Fases 9–10).
- Paso 1: `SysTick_Config(SystemCoreClock / 1000);` → interrupción cada 1 ms.
- Paso 2: `volatile uint32_t ms_ticks;` y `void SysTick_Handler(void){ ms_ticks++; }`
- Paso 3: `delay_ms(n)`: esperar mientras `ms_ticks - inicio < n`.
- Archivo: `timebase.c/.h`.

### Lazo principal con tareas (para lo "no bloqueante")
`main` ya es `while(1){ if(cmd_ready){...} }`. Se agrega al final del while una llamada `tasks_update();` que revisa
banderas (pot2seg_on, lcdwatch_on, iomap activo...) y hace un pedacito de trabajo cada X ms usando `ms_ticks`.
Así el monitor sigue recibiendo comandos (la UART recibe por interrupción).

### Parseo de argumentos
Ya existen `data_isValid()` (hex 0x...) y `atoi`. Para decimales/voltajes usar `strtoul(str, NULL, 10)` y `atof()`/
`strtof()`. Validar cantidad de tokens (`token_cont`) **antes** de usar `tokens[n]` (lección aprendida: leer tokens no
escritos truena).

---

## Fase 1 — SOUND freq / MUTE (10 pts)

Hardware: D12 (PB4) → buzzer. Onda cuadrada por PWM 50 %.
Timer: **TIM16_CH1 en PB4 (AF1)** ✅ DS12589 Tabla 13. Se elige TIM16 para dejar TIM3 libre para el RGB.
(Alternativa: TIM3_CH1 AF2, pero TIM3 lo usa el azul del RGB.)

Paso 1: Relojes
- `RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;`
- `RCC->APB2ENR |= RCC_APB2ENR_TIM16EN;`

Paso 2: PB4 en función alterna AF1
- `GPIOB->MODER &= ~(3U << (4*2)); GPIOB->MODER |= (2U << (4*2));` (10 = AF)
- `GPIOB->AFR[0] &= ~(0xFU << (4*4)); GPIOB->AFR[0] |= (1U << (4*4));`

Paso 3: Timer en PWM modo 1
- Prescaler para 1 MHz: `TIM16->PSC = 16 - 1;`
- `TIM16->CCMR1 &= ~TIM_CCMR1_OC1M; TIM16->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;` (PWM1 + preload)
- `TIM16->CCER |= TIM_CCER_CC1E;`
- **TIM16 es timer con break → `TIM16->BDTR |= TIM_BDTR_MOE;`** (sin esto no sale nada)
- `TIM16->CR1 |= TIM_CR1_ARPE;`

Paso 4: `sound(freq)`
- Si freq == 0 → `TIM16->CR1 &= ~TIM_CR1_CEN;` y forzar salida baja (CCR1 = 0) → silencio.
- Si no: `TIM16->ARR = 1000000 / freq - 1;` `TIM16->CCR1 = (TIM16->ARR + 1) / 2;` `TIM16->EGR = TIM_EGR_UG;`
  `TIM16->CR1 |= TIM_CR1_CEN;`
- Rango con PSC=15: ~16 Hz a 1 MHz (ARR es de 16 bits). Para freq < 16 Hz subir PSC.

Paso 5: Comandos en `main.c`
- `SOUND <freq>` (decimal, validar token_cont >= 2) → `sound(freq)`.
- `MUTE` → `sound(0)`.

Prueba:
- `SOUND 440` → tono La. `SOUND 1000` → más agudo. `SOUND 0` y `MUTE` → silencio.
- Si no suena: medir D12 con multímetro (≈1.6 V promedio con 50 %), revisar MOE y AF.

---

## Fase 2 — RGBOUT color (10 pts)

Hardware: jumpers J2/J3/J4 en posición **LED_G / LED_B / LED_R** (no LCD).
- R = SIGNAL3 = D9 (PA8) → **TIM1_CH1 AF6** ✅
- G = SIGNAL2 = D10 (PA11) → **TIM1_CH4 AF11** ✅
- B = SIGNAL1 = D11 (PB5) → **TIM3_CH2 AF2** ✅
(AFs confirmados en DS12589 Tabla 13.)

Paso 1: Relojes: `GPIOAEN`, `GPIOBEN` (AHB2ENR), `TIM1EN` (APB2ENR), `TIM3EN` (APB1ENR1).

Paso 2: Pines en AF
- PA8: MODER bits [17:16] = 10, `AFR[1]` nibble (8-8)=0 → 6
- PA11: MODER bits [23:22] = 10, `AFR[1]` nibble 3 → 11
- PB5: MODER bits [11:10] = 10, `AFR[0]` nibble 5 → 2

Paso 3: Timers a 8 bits de resolución (0–255), ~1 kHz
- `PSC = 16 - 1` (1 MHz), `ARR = 255` → ~3.9 kHz (sin parpadeo).
- TIM1: CH1 en `CCMR1` (OC1M = 110, OC1PE), CH4 en `CCMR2` (OC4M = 110, OC4PE), `CCER` CC1E | CC4E,
  **`TIM1->BDTR |= TIM_BDTR_MOE;`**, `CR1 |= ARPE | CEN`.
- TIM3: CH2 en `CCMR1` (OC2M = 110, OC2PE), `CCER` CC2E, `CR1 |= ARPE | CEN`.

Paso 4: `rgb_set(uint32_t color)`
- `r = (color >> 16) & 0xFF; g = (color >> 8) & 0xFF; b = color & 0xFF;`
- `TIM1->CCR1 = r; TIM1->CCR4 = g; TIM3->CCR2 = b;`

Paso 5: Comando `RGBOUT <RRGGBB>` en hex. Aceptar con o sin "0x" (el enunciado usa `FF0000`). Validar 6 dígitos hex.

Prueba: `RGBOUT FF0000` rojo, `00FF00` verde, `0000FF` azul, `008000` verde medio, `C000C0` morado, `808080` gris,
`000000` apagado. Si un color sale cambiado → revisar qué SIGNAL va a qué color (jumpers).

---

## Fase 3 — VOUT voltage [res] (10 pts)

Hardware: A4 (PA5) → **DAC1_OUT2** ✅ DS12589. **Requisito: SB2 OFF (Fase 0)**, si no A4 está en corto con SDA.
Vref = 3.3 V (VDDA).

Paso 1: Relojes: `RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_DAC1EN;`

Paso 2: PA5 en analógico: `GPIOA->MODER |= (3U << (5*2));` (11, ya es el valor de reset)

Paso 3: DAC canal 2
- Modo normal con buffer: `DAC1->MCR &= ~DAC_MCR_MODE2;` (000 = buffer ON, pin externo)
- `DAC1->CR |= DAC_CR_EN2;` esperar ~10 µs (`delay` corto o lazo).

Paso 4: `vout(float v, int res)`
- res permitido: 8 o 12 (default 8). (Si se pide otro, escalar a 12 bits.)
- `max = (1 << res) - 1; code = (uint32_t)(v / 3.3f * max + 0.5f);` limitar a max.
- res 8 → `DAC1->DHR8R2 = code;` res 12 → `DAC1->DHR12R2 = code;`

Paso 5: Comando `VOUT <voltaje> [res]`, voltaje decimal (ej. `1.65`), validar 0 ≤ v ≤ 3.3.

Prueba (multímetro en A4 o P6):
- `VOUT 0` → ~0 V, `VOUT 1.65` → ~1.65 V, `VOUT 3.3` → ~3.3 V (el buffer satura cerca de 3.2 V, es normal).
- `VOUT 1 12` vs `VOUT 1 8` → ambos ~1.00 V.

---

## Fase 4 — VIN [res] (10 pts)

Hardware: POT → A7 (PA2) → **ADC1_IN3** ✅ UM2397 Tabla 11.

Paso 1: Relojes
- `RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN | RCC_AHB2ENR_GPIOAEN;`
- Reloj del ADC síncrono: `ADC12_COMMON->CCR |= (1U << ADC_CCR_CKMODE_Pos);` (HCLK/1 = 16 MHz)

Paso 2: PA2 analógico: `GPIOA->MODER |= (3U << (2*2));`

Paso 3: Sacar el ADC de deep power down y calibrar
- `ADC1->CR &= ~ADC_CR_DEEPPWD;`
- `ADC1->CR |= ADC_CR_ADVREGEN;` esperar ≥ 20 µs.
- `ADC1->CR &= ~ADC_CR_ADCALDIF; ADC1->CR |= ADC_CR_ADCAL; while(ADC1->CR & ADC_CR_ADCAL);`

Paso 4: Habilitar
- `ADC1->ISR = ADC_ISR_ADRDY; ADC1->CR |= ADC_CR_ADEN; while(!(ADC1->ISR & ADC_ISR_ADRDY));`
- Secuencia de 1 conversión, canal 3: `ADC1->SQR1 = (3U << ADC_SQR1_SQ1_Pos);` (L = 0)
- Tiempo de muestreo largo para el pot: `ADC1->SMPR1 |= (7U << ADC_SMPR1_SMP3_Pos);`

Paso 5: `adc_read(int res)`
- res: 12, 10, 8, 6 → `CFGR.RES` = 00, 01, 10, 11. `ADC1->CFGR &= ~ADC_CFGR_RES; ADC1->CFGR |= (x << ADC_CFGR_RES_Pos);`
  (cambiar RES solo con ADSTART = 0)
- `ADC1->CR |= ADC_CR_ADSTART; while(!(ADC1->ISR & ADC_ISR_EOC)); return ADC1->DR;`

Paso 6: Comando `VIN [res]` → `v = valor * 3.3 / ((1<<res)-1)` → imprimir con 4 dígitos: `snprintf(buf, ..., "%.4f V")`.
⚠️ `printf` con `%f` en armclang funciona con la librería estándar; si imprime vacío, armar el número a mano
(parte entera + 4 decimales con enteros).

Prueba: girar el pot de un extremo al otro → `VIN` va de ~0.0000 a ~3.3000. `VIN 12` más fino que `VIN 8`.
Cruzada: `VOUT 1.5` + cable de A4 a A7 (sin pot) → `VIN` ≈ 1.5.

---

## Fase 5 — LCD string (5 pts) — Nokia 5110 / PCD8544

Hardware: jumpers J2/J3/J4 en **D/C / MOSI / L_LED** (no RGB). SPI1: SCK = D13 (PB3), MOSI = D11 (PB5), D/C = D10 (PA11)
como GPIO, luz = D9 (PA8) como GPIO. RST = NRST (la LCD se resetea con el reset de la tarjeta).

Paso 1: Relojes: `GPIOAEN`, `GPIOBEN`, `RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;`

Paso 2: Pines
- PB3, PB5: AF5 (MODER = 10, `AFR[0]` nibbles 3 y 5 = 5).
- PA11 (D/C) y PA8 (luz): salidas (MODER = 01).

Paso 3: SPI1 maestro, solo TX, modo 0, ≤ 4 MHz
- `SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (1U << SPI_CR1_BR_Pos);` (16 MHz / 4 = 4 MHz)
- `SPI1->CR2 = (7U << SPI_CR2_DS_Pos) | SPI_CR2_FRXTH;` (8 bits)
- `SPI1->CR1 |= SPI_CR1_SPE;`
- Enviar byte: esperar `TXE`, escribir `*(volatile uint8_t *)&SPI1->DR = b;` (acceso de 8 bits, si no manda 16),
  al final esperar `BSY = 0` antes de cambiar D/C.

Paso 4: Init PCD8544 (D/C = 0 para comandos)
- `0x21` (instrucciones extendidas), `0xB8` (contraste Vop; ajustar entre 0xB0 y 0xC8), `0x04` (coef. temp),
  `0x14` (bias 1:48), `0x20` (básicas), `0x0C` (modo normal).

Paso 5: Funciones
- `lcd_goto(x, fila)`: `0x80 | x` (0–83), `0x40 | fila` (0–5).
- `lcd_clear()`: 504 bytes de 0x00 con D/C = 1.
- Fuente 5x7 (tabla `font[96][5]` ASCII 0x20–0x7F), cada carácter = 5 columnas + 1 vacía → 14 caracteres × 6 filas.
- `lcd_print(char *s)`: escribir carácter por carácter, saltar de fila al llegar a 14.

Paso 6: Comando `LCD <string>`: el texto puede traer espacios → **no usar solo tokens[1]**; tomar todo lo que viene
después de "LCD " del buffer original (copiar `rx_data` antes de `strtok`, o volver a unir los tokens con espacios).

Prueba: `LCD Hola mundo` → aparece. Texto largo → salta de línea. Si no se ve nada: subir/bajar contraste (0xB0–0xC8),
revisar jumpers, prender luz con PA8.

---

## Fase 6 — Bus I2C + MCP23017 (base para SEGMENT, IOMAP, POT2SEG)

Hardware: SDA = D4 (PB7, I2C1_SDA AF4), SCL = D5 (PA15, I2C1_SCL AF4) ✅ UM2397 + DS12589.
Pull-ups 4.7 kΩ (R1, R2) ya están en el circuito. **Requisito: SB2 OFF (Fase 0).**

- Paso 1: Relojes: `RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;` + `GPIOAEN`, `GPIOBEN` en AHB2ENR.
  Reloj del I2C1: `RCC->CCIPR` bits I2C1SEL = 00 (PCLK1 = 16 MHz, es el valor de reset, no hay que tocarlo).
- Paso 2: Pines
  - PB7: `GPIOB->MODER` bits [15:14] = 10, `GPIOB->AFR[0]` nibble 7 = 4, `GPIOB->OTYPER |= (1U << 7);` (open-drain)
  - PA15: `GPIOA->MODER` bits [31:30] = 10, `GPIOA->AFR[1]` nibble (15−8)=7 = 4, `GPIOA->OTYPER |= (1U << 15);`
  - PA15 al reset es JTDI (JTAG). Se puede reconfigurar sin problema: la carga es por SWD (PA13/PA14), no JTAG.
- Paso 3: Timing 100 kHz con 16 MHz — **RM0440 Tabla 384**: PRESC = 3, SCLDEL = 0x4, SDADEL = 0x2, SCLH = 0x0F, SCLL = 0x13
  → `I2C1->TIMINGR = 0x30420F13;` (con PE = 0)
- Paso 4: `I2C1->CR1 |= I2C_CR1_PE;`
- Paso 5: `i2c_write(addr, reg, data)`:
  `I2C1->CR2 = (addr << 1) | (2U << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_START;`
  esperar `TXIS` → `TXDR = reg` → esperar `TXIS` → `TXDR = data` → esperar `STOPF` → `I2C1->ICR = I2C_ICR_STOPCF;`
- Paso 6: `i2c_read(addr, reg)`: escribir reg con NBYTES = 1 sin AUTOEND, esperar `TC`; luego
  `CR2 = (addr << 1) | I2C_CR2_RD_WRN | (1U << NBYTES_Pos) | AUTOEND | START`; esperar `RXNE`, leer `RXDR`; esperar `STOPF`.
- Paso 7: Si un dispositivo no contesta, sale `NACKF` en `I2C1->ISR` → limpiar con `ICR` y regresar error
  (para no quedarse colgado esperando `TXIS` para siempre). Ponerle timeout a todos los `while`.

Paso común: **escaneo** — comando de prueba `I2CSCAN` que intenta 0x20–0x27 y reporta cuál responde ACK.
Esperado: 2 direcciones (U2 y U4). **Escaneo (2026-11-22): contestan 0x20 y 0x24.** **U2 (LEDs/switches) = 0x24 ✅** (los LEDs prendieron con OLATA = 0). **U4 (display) = 0x20.**

Paso común: Init MCP23017 (registros con IOCON.BANK = 0):
- IODIRA 0x00, IODIRB 0x01, GPPUA 0x0C, GPPUB 0x0D, GPIOA 0x12, GPIOB 0x13, OLATA 0x14, OLATB 0x15.
- **U2:** IODIRA = 0x00 (LEDs salida), IODIRB = 0xFF (switches entrada), OLATA = 0xFF (LEDs apagados, activos en bajo),
  GPPUB = 0xFF si los switches lo necesitan.
- **U4:** IODIRA = 0x00 (segmentos), IODIRB = 0xF0 (GPB0–3 dígitos salida, GPB4–7 botones entrada), OLATA = 0, OLATB = 0.

Prueba: `I2CSCAN` muestra 2 direcciones. Escribir OLATA de U2 = 0x00 → todos los LEDs prenden.

---

## Fase 7 — SEGMENT num (10 pts)

Display 4 dígitos, cátodo común, multiplexado: se prende un dígito a la vez muy rápido (≥ 60 Hz por dígito).
Segmentos por U4 GPA (A=bit0 … G=bit6, DP=bit7), dígitos por U4 GPB0–3 (DIGIT4=GPB0 … DIGIT1=GPB3).

Paso 1: Tabla `seg[10] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};` (+ A–F para hex: 0x77,0x7C,0x39,0x5E,0x79,0x71)

Paso 2: `seg_digits[4]` con el número separado en dígitos (num % 10, /10...). Rango 0–9999.

Paso 3: Tarea de refresco **no bloqueante** en `tasks_update()`: cada 2–4 ms
- apagar dígitos (OLATB bits 0–3 = 0) → poner segmentos del siguiente dígito (OLATA) → prender ese dígito.
- Son 2 escrituras I2C por paso, a 100 kHz cada una ~0.3 ms → cabe.

Paso 4: Comando `SEGMENT <num>` → cargar `seg_digits`, `seg_on = 1`. "Permanentemente": se queda hasta otro SEGMENT.

Prueba: `SEGMENT 1234` → se lee 1234 sin parpadeo, y el monitor sigue respondiendo (`HELP` mientras se ve el display).
Si se ve al revés → invertir orden de dígitos. Si parpadea → refrescar más seguido.

---

## Fase 8 — IOMAP in out [not] [all] / IOUNMAP (10 pts)

Entradas: switches U2 GPB0–7 (in 0–7). Salidas: LEDs U2 GPA (out 0–7). Elegir numeración (ej. LED n = el que está
arriba del switch n) y documentarla aquí.
⚠️ LEDs activos en bajo: "prender" = escribir 0.

Paso 1: Tabla `map[8]` = entrada asignada a cada salida (−1 = sin mapa) + `inv[8]` (not).

Paso 2: `IOMAP in out` → `map[out] = in; inv[out] = 0;` · con `not` → `inv[out] = 1;`
`IOMAP all` (o `IOMAP x x all`, aceptar ambas formas) → `map[i] = i` para los 8.

Paso 3: Tarea en `tasks_update()` cada ~20 ms (no bloquea)
- leer GPIOB de U2 (1 lectura I2C) → para cada out con mapa: `bit = (in_val >> map[out]) & 1; if(inv[out]) bit ^= 1;`
- armar el byte de LEDs y escribir OLATA (recordar activo en bajo → invertir el byte al final).
- Opcional mejor: usar IRQ1 (INTB de U2 en D2) para leer solo cuando cambia algo.

Paso 4: `IOUNMAP` → `map[i] = -1`, apagar LEDs.

Prueba: `IOMAP 0 0` → mover switch 0 prende/apaga LED 0. `IOMAP 1 2 not` → LED 2 al revés del switch 1.
`IOMAP all` → los 8. `IOUNMAP` → nada responde. Mientras tanto `MD` y demás comandos siguen funcionando.

---

## Fase 9 — POT2SEG {on|off} [{dec|hex}] (25 pts)

Reusa Fase 4 (ADC) + Fase 7 (display). En paralelo, no bloquea.

Paso 1: Bandera `pot2seg_on`, modo `pot2seg_hex`.
Paso 2: En `tasks_update()` cada ~50 ms: `adc_read(12)` → convertir a dígitos dec (0–4095) o hex (000–FFF) →
cargar `seg_digits` (el refresco de la Fase 7 ya los muestra).
Paso 3: `POT2SEG on [dec|hex]` (default dec) / `POT2SEG off` (apaga display).
Paso 4: Que SEGMENT y POT2SEG no se peleen: POT2SEG on desactiva SEGMENT y viceversa.

Prueba: `POT2SEG on` → girar pot, el número cambia en tiempo real; `POT2SEG on hex` → muestra 0–FFF;
escribir comandos mientras tanto → responden. `POT2SEG off` → se apaga.

---

## Fase 10 — LCDWATCH addr [{dec|hex}] [{b|hw|w}] (25 pts)

Reusa Fase 5 (LCD). Hasta 4 watches; el 5º sobrescribe el más viejo.

Paso 1: Estructura
```c
typedef struct { uint32_t addr; uint8_t hex; uint8_t size; } watch_t;
watch_t watches[4]; uint8_t n_watch, next_watch;
```
Paso 2: `LCDWATCH addr [dec|hex] [b|hw|w]` (default dec, b) → guardar en `watches[next_watch]`,
`next_watch = (next_watch + 1) % 4`.
Paso 3: En `tasks_update()` cada ~200 ms: por cada watch leer `*(volatile uint8_t/uint16_t/uint32_t *)addr`,
formatear `"%08lX:%lu"` (o hex) y escribir en la fila i de la LCD (14 caracteres por fila → formato corto,
ej. `"2424:  255"` usando los 4 dígitos bajos de la dirección, o 2 filas por watch).
Paso 4: Comando para quitar todos (opcional): `LCDWATCH off`.
⚠️ Una dirección inválida → HardFault. Validar que la dirección esté en RAM (0x20000000–0x20007FFF), flash
(0x08000000–0x0801FFFF) o periféricos (0x40000000–0x5FFFFFFF) antes de agregarla.

Prueba: `LCDWATCH 0x20000100 hex w` → `MM 0x20000100 0xAB` → la LCD cambia sola. Agregar 5 watches → el primero
desaparece. Comandos siguen respondiendo.

---

## Fase 11 — Comando naranja (OBLIGATORIO, sin él nota = 0)

Elegir uno con el usuario. Opciones con el hardware que ya existe:

| Opción | Pts | Usa | Dificultad |
|---|---|---|---|
| MULTITAP on/off | 5 | teclado | baja-media |
| ARCADE breakout | 10 | LCD + pot + teclado (* #) | media |
| GENERATOR sin/sqr/tri/swt | 10 | DAC + timer + pot + teclado + LCD | media |
| ACCEL | 5 | acelerómetro (**no está en el circuito**) | — |
| OSCILLOSCOPE | 10 | ADC 10 kHz + **UART del ST-Link** (choca con POT/KEY6) + programa en PC | alta |
| SERVO | 10 | motor DC + encoder (**no está en el circuito**) | alta |

### Si es MULTITAP (pasos)
- Teclado: filas KEY2/KEY7/KEY6/KEY4 (PA0/PA4/PA3/PA1) como salidas; columnas KEY3/KEY1/KEY5 (PB0/PB6/PF1) como entradas
  con pull-up (`PUPDR = 01`).
- Escaneo en `tasks_update()` cada 10 ms: bajar una fila a la vez, leer columnas (0 = presionada), antirrebote ~30 ms.
- MultiTap: misma tecla repetida antes de ~800 ms rota letras (2 → a b c 2), si pasa el tiempo o cambia de tecla
  se confirma la letra. `#` = Enter (simular `\r` → poner `cmd_ready`), `*` = borrar.
- Escribir las letras en `rx_data`/`i` (las mismas variables de la UART) para que el monitor las procese igual,
  y hacer eco por UART para verlas en CoolTerm.

### Si es ARCADE breakout (pasos)
- Pantalla LCD 84x48 con buffer de 504 bytes en RAM, dibujar pixel a pixel y mandar el buffer completo cada frame.
- Barra: posición = pot (ADC) escalado a 0–(84−ancho).
- Pelota: posición y velocidad (dx, dy), rebote en paredes, barra y ladrillos; ladrillos en matriz de bits.
- `*` o `#` (teclado) inicia cada jugada. Frame cada ~30 ms con `ms_ticks`.

### Si es GENERATOR (pasos)
- Tabla de 64–100 muestras por forma de onda; TIM6 dispara interrupción a (freq × N muestras) y escribe DAC.
- Offset 1.8 V, pico 1 V → valores entre 0.8 y 2.8 V.
- Pot ajusta frecuencia (10–100 Hz) o amplitud según modo; `*` = modo frecuencia, `#` = modo amplitud.
- LCD muestra modo, forma de onda, Vp y frecuencia.

---

## Fase 12 — Cierre

- `HELP` con todos los comandos nuevos.
- Probar todo junto (POT2SEG + LCDWATCH + IOMAP al mismo tiempo, jumpers en LCD).
- Documentación/reporte (lo pide el enunciado): esquemático + métodos usados. El usuario la pedirá al final; usar
  las secciones "Paso N" de este archivo como base.

---

## Bitácora (llenar conforme se avanza)

| Fecha | Fase | Resultado / notas |
|---|---|---|
| 2026-11-21 | Monitor básico | Funciona por FTDI (USART1 PA9/PA10). |
| 2026-11-21 | Pinout | Confirmado con UM2397 Tabla 11 + DS12589 Tabla 13. D5 = PA15 → I2C por hardware. TIMINGR = 0x30420F13 (RM0440 Tabla 384). |
| 2026-11-21 | Fase 0 | SB2 ON (A4–D4 continuidad) → **hay que quitarlo** (pendiente). SB3 ON (A5–D5) → se deja, PA6 sin usar. SB11 y SB14: pendiente (paso 2). |
| 2026-11-21 | Fase 0 | SB2 quitado. Verificado por SWD (pyocd commander, core en halt): PA5=0 → PB7 lee 1; PB7=0 → PA5 lee 1 → ya no hay corto. |
| 2026-11-21 | Fase 1 SOUND/MUTE | ✅ Funciona (`buzzer.c`). TIM16_CH1 PB4 AF1. PSC se calcula según freq (`ticks/65536`) → 1 Hz a 100 kHz. Silencio = OC1M "force inactive" (0100) para dejar el pin en 0. |
| 2026-11-21 | Fase 2 RGBOUT | ✅ Funciona (`rgb.c`). TIM1 CH1/CH4 + TIM3 CH2, PSC=15, **ARR=254** (así FF = 100 % y 00 = apagado). Acepta `FF0000` o `0xFF0000`. |
| 2026-11-21 | Orden | Módulos de comandos verdes movidos a `G4_Test1/mediums/` (grupo `Mediums` en el cproject). Compila igual (binario idéntico). |
| 2026-11-21 | Fase 3 VOUT | ✅ Funciona (`mediums/dac.c`). DAC1 canal 2 en PA5, espera `DAC2RDY`. res 1–12 (default 8): `code = v/3.3·(2^res−1)` y se escribe `code << (12−res)` en `DHR12R2`. |
| 2026-11-21 | Fase 4 VIN | ✅ Funciona (`mediums/adc.c`). ADC1_IN3 PA2, CKMODE=01, SMP=640.5 ciclos, res 6/8/10/12. Verificado por SWD con VREFINT (0x5E2 = 1.214 V). |
| 2026-11-21 | Fase 5 LCD | ✅ Funciona (`mediums/lcd.c`). SPI1 TX-only 2 MHz. **Lección 1:** SCE está a GND → un pulso falso en SCLK desfasa los bits para siempre (letras partidas, texto corrido). Solución: pull-down en PB3, SPE antes de pasar el pin a AF, MODER en una sola escritura. **Lección 2:** CoolTerm manda CR+LF → el LF quedaba al inicio del siguiente comando (salía "D " en la LCD). `USART.c` ignora `\n`. El texto se toma de `rx_line` (copia de la línea, porque `strtok` parte `rx_data`). RGB y LCD se roban los pines en cada comando (`rgb_pins()` / `lcd_pins()`). |
| 2026-11-22 | Fase 6 I2C | `mediums/i2c.c`: I2C1 PB7/PA15 AF4 open-drain, TIMINGR 0x30420F13, timeouts + NACK en todos los while, `i2c_reset()` apagando PE. Comandos de prueba I2CSCAN / I2CR / I2CW. Escaneo al arranque guardado en `i2c_devs` (leído por SWD): **0x20 y 0x24**. |
| 2026-11-22 | Fase 6 I2C | ✅ U2 = 0x24 (LEDs prenden con OLATA = 0). U4 = 0x20: IODIRB = 0xF0, OLATB = 0x0F, IODIRA = 0, OLATA = 0xFF → "8.8.8.8." y todos los registros se leen de vuelta bien. GPIOB de U4 sin presionar = 0x0F (botones GPB4–7 leen 0 en reposo). Los comandos son sensibles a mayúsculas. |
| 2026-11-22 | Base común + Fase 7 | `timebase.c` (SysTick 1 ms, `ms_ticks`, `delay_ms`), `tasks.c` (`tasks_update()` al final del `while(1)` del main), `seg.c` (multiplexado cada 3 ms: OLATB = 0 → OLATA = segmentos → OLATB = dígito; DIGIT1 = GPB3 izquierda; sin ceros a la izquierda; `seg_show_hex` listo para POT2SEG). Pendiente de prueba. |
| 2026-11-22 | Fase 7 SEGMENT | ✅ Funciona. |
| 2026-11-22 | Fase 8 IOMAP | `mediums/iomap.c`. Switches de U2 (GPB) manejan 0/1 solos: con GPPUB = 0xFF siguen leyendo 0x00 → **sin pull-ups**. Numeración: in n = GPBn, out n = GPAn. Tarea cada 20 ms: 1 lectura de GPIOB y escribe OLATA solo si cambió. Acepta `IOMAP i o`, `IOMAP i o not`, `IOMAP all`, `IOMAP all not`. Init verificado por SWD (IODIRA 0x00, IODIRB 0xFF, OLATA 0xFF). Pendiente de prueba. |
| 2026-11-22 | Fase 8 IOMAP | Funciona pero a veces los LEDs parpadeaban. Arreglo: antirrebote (3 lecturas iguales cada 10 ms) + refresco cada 250 ms de IODIRA/OLATA por si el MCP se reinicia o pierde una escritura. Pendiente confirmar. |
| 2026-11-22 | Fase 8 IOMAP | ✅ Funciona sin parpadeo, polaridad correcta. **Causa real:** el switch en OFF va a GND y en ON queda **abierto** → sin pull-up el pin flotaba (leía 0/1 al azar = parpadeo; con antirrebote nunca se estabilizaba). La prueba anterior "sin pull-ups" se hizo con todos en OFF y por eso engañó. **Arreglo: GPPUB = 0xFF** en U2 (y en el refresco). Se quedan antirrebote + refresco. |
| 2026-11-22 | Fase 9 POT2SEG | `mediums/pot2seg.c`: cada 50 ms promedio de 16 conversiones a 12 bits → `seg_show_dec` (0–4095) o `seg_show_hex` (0–FFF). SEGMENT llama `pot2seg_stop()`; POT2SEG on pisa a SEGMENT; `POT2SEG off` apaga el display. Verificado por SWD (flag en RAM → dígitos = 4095). Pendiente de prueba. |
| 2026-11-22 | Fase 9 POT2SEG | ✅ Funciona. |
| 2026-11-22 | Fase 10 LCDWATCH | `mediums/lcdwatch.c`: 4 watches en filas 0–3, formato de 14 caracteres `AAAA:valor` (4 dígitos bajos de la dirección, valor a la derecha; dec word de 10 dígitos sin ':'). El 5º sobrescribe el más viejo (misma fila). Refresco cada 100 ms. Valida RAM/CCM/flash/periféricos + alineación para hw/w (evita HardFault). `LCDWATCH off` los quita; `LCD texto` también los apaga. Verificado por SWD: watch inyectado sobre `ms_ticks`, sin faults (CFSR = HFSR = 0). Pendiente de prueba. |
| 2026-11-22 | Fase 10 LCDWATCH | ✅ Funciona. **Todos los verdes del plan terminados.** Falta el naranja (Fase 11). |
| 2026-11-22 | Limpieza | Quitados comandos I2CSCAN / I2CR / I2CW (y `i2c_scan`/`i2c_probe` + escaneo al arranque). Quedan `i2c_write`/`i2c_read` (los usan seg, iomap). |
| 2026-11-22 | Rúbrica | Tabla de puntos leída con `pdftotext -raw`: IOMAP 10, SEGMENT 10, LCD 5, AIN 5 (no vale con VIN), AOUT 5 (no vale con VOUT), VIN 10, VOUT 10, SOUND 10, RGB 10, POT2LED 10 (no vale con POT2SEG/POT2LCD), POT2SEG 25 (**no vale con POT2LCD**), POT2LCD 15, LEDWATCH 15 (no vale con SEGWATCH/LCDWATCH), SEGWATCH 20 (no vale con LCDWATCH), LCDWATCH 25. Naranjas: ACCEL 5, MULTITAP 5, ARCADE 10, GENERATOR 10, OSCILLOSCOPE 10, SERVO 10 (netos, uno obligatorio). Total actual = 35 + 115 = **150 (tope)**. AIN/AOUT/POT2LED/LEDWATCH/SEGWATCH suman 0; **POT2LCD resta 10** (anula POT2SEG). |
| 2026-11-22 | Decisión | El usuario decide quedarse con los verdes de más puntos (150 %). Naranja elegido: **ARCADE breakout**. |
| 2026-11-22 | Fase 11 ARCADE | `mediums/arcade.c` + `keypad.c`. Buffer de 504 bytes en RAM (6 bancos × 84), se manda completo cada 30 ms (`lcd_send_buffer`). HUD "P:puntos V:vidas" + línea; 4×12 ladrillos de 6×3 px como bits; barra 14 px con POT (posición absoluta, promedio 4 a 12 bits); bola 2×2 en 1/16 px, ángulo según dónde pega en la barra; 3 vidas; estados ESPERA/JUEGO/PERDIO/GANO. Teclado: solo fila 4 (KEY4 = PA1 en 0), `*` = PB0, `#` = PF1 con pull-up, por flanco. Corre en `tasks_update()`; `ARCADE off`, `LCD` y `LCDWATCH` lo apagan. Verificado por SWD: PB0/PF1 = 1 en reposo, PA1 = 0, buffer con "P:0" y ladrillos, sin faults. Pendiente de prueba. |
| 2026-11-22 | Fase 11 ARCADE | ✅ Funciona. Bola bajada a la mitad (`VEL_Y` 24 → 12, `VEL_X` 6, `VEL_GIRO` 2) porque la LCD es lenta y no se veía. |
| 2026-11-22 | Fase 12 Cierre | ✅ Prueba de todo junto OK (IOMAP + POT2SEG + LCDWATCH + ARCADE + comandos). HELP con los 22 comandos. Reporte en Word: `Reporte_Proyecto_Final.docx` (17 págs: esquemático en página horizontal, pines, adaptaciones G431KB, arquitectura, métodos por comando, ARCADE, problemas/soluciones, puntos, conclusiones). Pendiente opcional: borrar `G4_Test1/G4_Test1.lib` (ya no se usa). |
