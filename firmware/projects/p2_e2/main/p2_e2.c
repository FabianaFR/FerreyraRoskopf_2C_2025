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
#include "timer_mcu.h"
/*==================[macros and definitions]=================================*/

#define CONFIG_MEASURE_PERIOD 300 // ms entre mediciones

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

void Medir_MostrarPantalla(void *pvParameter)
{
    vTaskNotifyGiveFromISR(Medir_MostrarPantalla_task_handle, pdFALSE);
}

/*==================[tasks definition]=======================================*/
static void Medir_MostrarPantalla_Task(void *pvParameter)
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
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
    }
}

void tecla_1()
{
    medir = !medir; // toggle medir
    if (medir == false)
    {
        LedsOffAll();
        LcdItsE0803Off(); // limpiar pantalla
    }
}

void tecla_2()
{
    hold = !hold; // toggle hold
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
    LedsInit();
    LcdItsE0803Init();
    HcSr04Init(GPIO_3, GPIO_2);
    SwitchesInit();

    timer_config_t timer_medir = {
        .timer = TIMER_A,
        .period = CONFIG_MEASURE_PERIOD * 1000, // convertir a us
        .func_p = Medir_MostrarPantalla,
        .param_p = NULL};
    TimerInit(&timer_medir);

    SwitchActivInt(SWITCH_1, tecla_1, NULL);
    SwitchActivInt(SWITCH_2, tecla_2, NULL);

    xTaskCreate(&Medir_MostrarPantalla_Task, "MEDIR_Y_MOSTRAR", 2048, NULL, 5, &Medir_MostrarPantalla_task_handle);
    TimerStart(timer_medir.timer);
}

/*==================[end of file]============================================*/
