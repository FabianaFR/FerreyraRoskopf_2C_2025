/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "hc_sr04.h"
#include "lcditse0803.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/

#define CONFIG_MEASURE_PERIOD 500 // ms entre mediciones
#define CONFIG_READING_PERIOD 20  // ms entre lecturas de teclas

#define MIN_DIST 10 // cm
#define MED_DIST 20
#define MAX_DIST 30

/*==================[internal data definition]===============================*/
TaskHandle_t Medir_MostrarPantalla_task_handle = NULL;
TaskHandle_t teclas_task_handle = NULL;

bool medir = true; // bandera de “medir”
bool hold = false; // bandera de “hold”

/*==================[internal functions declaration]=========================*/
void encenderLedSegunDistancia(uint32_t distance)
{
    if (distance < MIN_DIST)
    { // <10 cm
        LedsOffAll();
    }
    else if (distance >= MIN_DIST && distance < MED_DIST)
    { // 10–20 cm
        LedOn(LED_1);
        LedOff(LED_2);
        LedOff(LED_3);
    }
    else if (distance >= MED_DIST && distance < MAX_DIST)
    { // 20–30 cm
        LedOn(LED_1);
        LedOn(LED_2);
        LedOff(LED_3);
    }
    else if (distance >= MAX_DIST)
    { // >30 cm
        LedOn(LED_1);
        LedOn(LED_2);
        LedOn(LED_3);
    }
}

/*==================[tasks definition]=======================================*/
static void Medir_MostrarPantalla_Task(void *pvParameter)
{
    while (true)
    {
        uint16_t distancia = 0;
        if (medir == true)
        {
            distancia = HcSr04ReadDistanceInCentimeters(); // medir distancia
        }

        if (hold == false && medir == true)
        {
            encenderLedSegunDistancia(distancia);
            LcdItsE0803Write(distancia);
        }
        else if (hold == true && medir == true)
        {
            // mantener la última medición, no hacer nada
        }

        vTaskDelay(CONFIG_MEASURE_PERIOD / portTICK_PERIOD_MS);
    }
}

static void Teclas_Task(void *pvParameter)
{
    while (1)
    {
        uint8_t switchOn = SwitchesRead();

        if (switchOn == SWITCH_1)
        { // tecla 1
            medir = !medir;
            if (medir == false)
            {
                LedsOffAll();     // apagar LEDs si dejo de medir
                LcdItsE0803Off(); // apagar LCD si dejo de medir
            }
        }
        else if (switchOn == SWITCH_2)
        {                 // tecla 2
            hold = !hold; // toggle hold
        }

        vTaskDelay(CONFIG_READING_PERIOD / portTICK_PERIOD_MS);
    }
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
    LedsInit();
    LcdItsE0803Init();
    HcSr04Init(GPIO_3, GPIO_2);
    SwitchesInit();

    xTaskCreate(&Medir_MostrarPantalla_Task, "MEDIR_Y_MOSTRAR", 2048, NULL, 5, &Medir_MostrarPantalla_task_handle);
    xTaskCreate(&Teclas_Task, "TECLAS", 2048, NULL, 5, &teclas_task_handle);
}

/*==================[end of file]============================================*/
