# ZKS - the 'Zugkraftsensor' project

## Libraries

1. esp32 by Espressif Systems 2.0.14 -> 3.0.0 - alpha 1 will not work
2. Adafruit NeoPixel 1.11.0
3. ESP32Time by fbiego 2.0.4
4. ESP32TimerInterrupt by Khoi Hoang 2.3.0

## Arduino Board type

Use the 'ESP32 Dev Module'

## Source Files

1. ZKS.ino
2. ADC.h
3. cli.h
4. ZKSConfig.h

## Algorithm

(under construction)
The voltage output of the KM30z for 50kN can be found in the manufactorer's data sheet as characteristic value (K).
The first prototype board uses a sensor with K=0.99268 mV/V.
The voltage at 50kN with our circuit is: Uf = Us * G * K 
G: gain (923)
Us: the votage at the sensor (2.5V)
We assume a voltage of 0V with a force of 0.
Here with 2.5V and a gain of 923:
U(50kN) = 2.2906091 [V] = 2.3075 *K [V]
U(F) =  (2.3075* K/50000) * F [V] with F is the force in Newton.
That means:
F(U) = 50000/(K*2.3075) U [N] = 2166.85/K * U[N] or
F(U) = 

## Wireless

Currently the connection to the ZKS is done using Bluetooth. For maximizing the Bluetooth 
range the power is switched to the maximum value:

**esp_bredr_tx_power_set(ESP_PWR_LVL_P9,ESP_PWR_LVL_P9);**

The devicename is a combination of devicename and serialnumber. (DYNAFORCE_ZKS_2345)

## Hardware

[Data sheet used MCP3564 24 bit - 4 channel ADC](/https://www.mouser.de/datasheet/2/268/MCP3561_2_4_Family_Data_Sheet_DS20006181C-2257924.pdf)

MCP3561_2_4_Family_Data_Sheet_DS20006181C-2257924.pdf

The code for handling the ADC is in ADC.h

## Power Management

The observation of the current accu state is under construction

When no communication occured for 3600 seconds, the ZKS will shut down. This idle time shutdown duration is a default value and can be overwritten. The shutdown occurs with setting the SHUTDOWNPIN (18) to low.
