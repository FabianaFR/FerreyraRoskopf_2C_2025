/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD 100
// defino valores para myleds.mode
#define ON 1
#define OFF 0
#define TOGGLE 2

/*==================[internal data definition]===============================*/
// struct Tleds
// {
//     uint8_t mode;      // ON, OFF, TOGGLE
// 	uint8_t n_led;        //indica el número de led a controlar
// 	uint8_t n_ciclos;   //indica la cantidad de ciclos de encendido/apagado
// 	uint16_t periodo;    //indica el tiempo de cada ciclo
// }my_leds;

// CON ESTO DEFINO LA ESTRUCTURA
struct Tleds // T de tipo -- es una colección de variables agrupadas bajo un mismo nombre // ¿una clase?
{
	uint8_t mode;	  // ON, OFF, TOGGLE
	uint8_t n_led;	  // indica el número de led a controlar
	uint8_t n_ciclos; // indica la cantidad de ciclos de encendido/apagado
	uint16_t periodo; // indica el tiempo de cada ciclo
};

// CON ESTO GUARDO LA ESTRUCTURA EN MY_LEDS Y OCUPA LUGAR EN LA MEMORIA
struct Tleds my_leds; // declaro una variable llamada my_leds, y esa variable es de tipo struct Tleds

/*==================[internal functions declaration]=========================*/
void function_leds(struct Tleds *ptr); // defino el nombre de la función y lo que está entre paréntesis define un paramretro de la función
// el parámetro de la función es un puntero que se dirige hacia la dirección de memoria donde se encuentra el struct Tleds guardado en la variable my_leds

/*==================[external functions definition]==========================*/

void app_main(void)
{
	uint8_t teclas;
	LedsInit();
	SwitchesInit();

	my_leds.mode = TOGGLE;
	my_leds.n_led = LED_2;
	my_leds.n_ciclos = 10;
	my_leds.periodo = 5;

	// function_leds(&my_leds) -- llama a la función function_leds y le pasa la dirección donde está guardada la estructura my_leds
	// struct Tleds * ptrstruct = &my_leds

	struct Tleds *ptrstruct;  // creo una variable  puntero a una estructura Tleds o de tipo Tleds. La variable se llama ptrstruct
	ptrstruct = &my_leds;	  // esta línea hace que el puntero apunte a la dirección de memoria de my_leds
	function_leds(ptrstruct); // Llama a la función usando el puntero
}

void function_leds(struct Tleds *ptr)
{
	switch (ptr->mode)
	{
	case ON:
		switch (ptr->n_led)
		{
		case LED_1:
			LedOn(LED_1);
			break;

		case LED_2:
			LedOn(LED_2);
			break;

		case LED_3:
			LedOn(LED_3);
			break;
		}

	case OFF:
		switch (ptr->n_led)
		{
		case LED_1:
			LedOff(LED_1);
			break;

		case LED_2:
			LedOff(LED_2);
			break;

		case LED_3:
			LedOff(LED_3);
			break;
		}

	case TOGGLE:
		for (int i = 0; i < ptr->n_ciclos; i++)
		{
			switch (ptr->n_led)
			{
			case LED_1:
				LedToggle(LED_1);
				break;

			case LED_2:
				LedToggle(LED_2);
				break;

			case LED_3:
				LedToggle(LED_3);
				break;

			default:
			}
			for (uint8_t i = 0; i < ptr->periodo; i++)
				vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
		}
	}
}
/*==================[end of file]============================================*/