PINS_AF = (
  ('D0_RX', (1, 'TMR4_TIMER0'), (2, 'FLEXPWM2_PWMB1'), (3, 'LPUART3_RX'), (5, 'GPIO2_PIN9'), ),
  ('D1_TX', (1, 'TMR3_TIMER2'), (2, 'FLEXPWM2_PWMA1'), (3, 'LPUART3_TX'), (5, 'GPIO2_PIN8'), ),
  ('D2_INT0', (1, 'FLEXPWM1_PWMB3'), (5, 'GPIO1_PIN11'), ),
  ('D3_INT1', (1, 'FLEXPWM4_PWMA0'), (5, 'GPIO1_PIN24'), ),
  ('D4', (1, 'FLEXPWM2_PWMA3'), (3, 'SAI2_TX_DATA'), (5, 'GPIO1_PIN9'), (7, 'GPT2_CLK'), ),
  ('D5_PWM1', (1, 'TMR3_TIMER0'), (2, 'FLEXPWM2_PWMA0'), (5, 'GPIO2_PIN6'), ),
  ('D6_PWM2', (1, 'TMR3_TIMER1'), (2, 'FLEXPWM2_PWMB0'), (5, 'GPIO2_PIN7'), ),
  ('D7_PWM3', (1, 'TMR3_TIMER3'), (2, 'LPUART2_RX'), (5, 'GPIO1_PIN19'), ),
  ('D8', (2, 'LPUART6_RX'), (4, 'FLEXPWM1_PWMX1'), (5, 'GPIO1_PIN3'), (7, 'LPSPI3_PCS0'), ),
  ('D9_PWM4', (2, 'LPUART6_TX'), (4, 'FLEXPWM1_PWMX0'), (5, 'GPIO1_PIN2'), (7, 'LPSPI3_SDI'), ),
  ('D10_SS', (2, 'LPUART6_RX'), (4, 'FLEXPWM1_PWMX1'), (5, 'GPIO1_PIN3'), (7, 'LPSPI3_PCS0'), ),
  ('D11_MO', (0, 'FLEXPWM2_PWMB3'), (5, 'GPIO1_PIN1'), (7, 'LPSPI3_SDO'), ),
  ('D12_MI', (2, 'LPUART6_TX'), (4, 'FLEXPWM1_PWMX0'), (5, 'GPIO1_PIN2'), (7, 'LPSPI3_SDI'), ),
  ('D13_CK', (0, 'FLEXPWM2_PWMA3'), (5, 'GPIO1_PIN0'), (7, 'LPSPI3_SCK'), ),
  ('D14_SDA', (0, 'LPI2C4_SDA'), (1, 'GPT1_CLK'), (2, 'LPUART1_RX'), (4, 'FLEXPWM1_PWMX3'), (5, 'GPIO1_PIN13'), ),
  ('D15_SCL', (0, 'LPI2C4_SCL'), (2, 'LPUART1_TX'), (4, 'FLEXPWM1_PWMX2'), (5, 'GPIO1_PIN12'), ),
  ('A0', (1, 'TMR3_TIMER0'), (2, 'LPUART2_CTS_B'), (3, 'LPI2C1_SCL'), (5, 'GPIO1_PIN16'), ),
  ('A1', (2, 'LPUART8_RX'), (3, 'SAI1_RX_BCLK'), (5, 'GPIO1_PIN27'), ),
  ('A2', (2, 'LPUART3_CTS_B'), (5, 'GPIO1_PIN20'), ),
  ('A3', (2, 'LPUART3_RTS_B'), (5, 'GPIO1_PIN21'), ),
  ('A4', (1, 'TMR3_TIMER1'), (2, 'LPUART2_RTS_B'), (3, 'LPI2C1_SDA'), (5, 'GPIO1_PIN17'), ),
  ('A5', (1, 'TMR3_TIMER0'), (2, 'LPUART2_CTS_B'), (3, 'LPI2C1_SCL'), (5, 'GPIO1_PIN16'), ),
  ('LED', (1, 'FLEXPWM2_PWMA3'), (3, 'SAI2_TX_DATA'), (5, 'GPIO1_PIN9'), (7, 'GPT2_CLK'), ),
  ('LED', (1, 'FLEXPWM1_PWMA3'), (3, 'SAI2_MCLK'), (5, 'GPIO1_PIN10'), ),
  ('LED', (1, 'FLEXPWM1_PWMB3'), (5, 'GPIO1_PIN11'), ),
  ('LED', (1, 'TMR4_TIMER2'), (2, 'FLEXPWM2_PWMB2'), (3, 'SAI1_TX_DATA2'), (5, 'GPIO2_PIN11'), ),
  ('KEY', (5, 'GPIO5_PIN0'), ),
  ('DBG_RXD', (0, 'LPI2C4_SDA'), (1, 'GPT1_CLK'), (2, 'LPUART1_RX'), (4, 'FLEXPWM1_PWMX3'), (5, 'GPIO1_PIN13'), ),
  ('DBG_TXD', (0, 'LPI2C4_SCL'), (2, 'LPUART1_TX'), (4, 'FLEXPWM1_PWMX2'), (5, 'GPIO1_PIN12'), ),
)
