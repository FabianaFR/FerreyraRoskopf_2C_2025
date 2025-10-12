/*! @mainpage Guia 2 Actividad 1
 *
 * @section genDesc General Description
 *
 * Se diseñó el firmware de manera que cumple con las siguientes funcionalidades:
 * <ol>
 * 		<li>Mostrar distancia medida utilizando los leds de la siguiente manera: </li>
 * - Si la distancia es menor a 10 cm, apagar todos los LEDs.
 * - Si la distancia está entre 10 y 20 cm, encender el LED_1.
 * - Si la distancia está entre 20 y 30 cm, encender el LED_2 y LED_1.
 * - Si la distancia es mayor a 30 cm, encender el LED_3, LED_2 y LED_1.
 * 		<li>Mostrar el valor de distancia en cm utilizando el display LCD. </li>
 * 		<li>Usar TEC1 para activar y detener la medición. </li>
 * 		<li>Usar TEC2 para mantener el resultado (“HOLD”): sin pausar la medición. </li>
 * 		<li>Refresco de medición: 1 s (1000 ms). </li>
 * </ol>
 *
 * <a href="https://drive.google.com/file/d/1yIPn12GYl-s8fiDQC3_fr2C4CjTvfixg/view">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  | 	ESP32-C6	|
 * |:--------------:|:--------------|
 * | 	ECHO	 	| 	GPIO_3		|
 * | 	TRIGGER	 	| 	GPIO_2		|
 * | 	Vcc		 	| 	+5V			|
 * | 	GND		 	| 	GND			|
 * | 	LED_1	 	| 	GPIO_11		|
 * | 	LED_2	 	| 	GPIO_10		|
 * | 	LED_3	 	| 	GPIO_5		|
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 13/04/2025 | Se agregó la funcionalidad de HOLD.            |
 *
 *  @author Fabiana F Roskopf (fabianafroskopf@gmail.com)
 *
 */
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
/**
 * @def CONFIG_MEASURE_PERIOD
 * @brief Define el periodo de medición
 * @details 500 ms
 */
#define CONFIG_MEASURE_PERIOD 500
/**
 * @def CONFIG_READING_PERIOD
 * @brief Define el periodo de lectura de teclas
 * @details 20 ms
 */
#define CONFIG_READING_PERIOD 20

/**
 * @def MIN_DIST
 * @brief Define la distancia mínima
 * @details 10 cm
 */
#define MIN_DIST 10
/**
 * @def MED_DIST
 * @brief Define la distancia media
 * @details 20 cm
 */
#define MED_DIST 20
/**
 * @def MAX_DIST
 * @brief Define la distancia máxima
 * @details 30 cm
 */
#define MAX_DIST 30

/*==================[internal data definition]===============================*/
/**
 * @brief identificador de tipo TaskHandle_t de la tarea "Medir_MostrarPantalla".
 * @details Se utiliza para la creación de la tarea y para su control. */
TaskHandle_t Medir_MostrarPantalla_task_handle = NULL;
/**
 * @brief identificador de tipo TaskHandle_t de la tarea "teclas".
 * @details Se utiliza para la creación de la tarea y para su control. */
TaskHandle_t teclas_task_handle = NULL;

/**
 * @brief Flags que indica si se debe medir o no.
 * @details Se utiliza para habilitar o deshabilitar la medición. */
bool medir = true;
/**
 * @brief Flags que indica si se debe mantener la última medición en pantalla.
 * @details Se utiliza para habilitar o deshabilitar la modificación de los LEDs y valores mostrados en el LCD. */
bool hold = false;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Tarea muestra el resultado del valor medido de _distancia_.
 * @details Muestra el resultado en el LCD y lo representa en los LEDs según el diseño planteado.
 */
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
/**
 * @brief Tarea que mide la distancia, muestra el resultado en el LCD y enciende los LEDs.
 */
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
/**
 * @brief Tarea que lee las teclas y cambia el estado de MEDIR y HOLD.
 * @details Cambia el estado de MEDIR y HOLD según la tecla presionada.
 */
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
