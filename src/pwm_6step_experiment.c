/*
 *This file is part of the open-coroco project.
 *
 *  Copyright (C) 2013  Sebastian Chinchilla Gutierrez <tumacher@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */



#include "pwm_6step_experiment.h"
//impedance control libraries
#include "set_up.c"
#include "usart_functions.c"
#include "timer_1.c"
#include "timer_2.c"
#include "hall_sensors.c"
#include "control_loops.c"
#include "interrupt_handler.c"


int main(void)
{
	stm32_setup();
	
	// hall data initialization
	hall1_data.hall_state=HALL_INITIAL;
	hall1_data.hall_counter=0;
	hall1_data.hall_update=false;
	hall1_data.hall_corrected=LOW;

	hall2_data.hall_state=HALL_INITIAL;
	hall2_data.hall_counter=0;
	hall2_data.hall_update=false;
	hall2_data.hall_corrected=LOW;

	hall3_data.hall_state=HALL_INITIAL;
	hall3_data.hall_counter=0;
	hall3_data.hall_update=false;
	hall3_data.hall_corrected=LOW;	

	float captured_A,captured_B,captured_C,captured_D,captured_E,captured_F,captured_G,captured_H;


	while (1)
	{



		captured_A=A;
		captured_B=B;
		captured_C=C;
		captured_D=D;
		captured_E=E;
		captured_F=F;
		captured_G=G;
		captured_H=H;

		//sine frequency
		usart_transmit_a_tagged_floating_number("freq", captured_A);		
		//rotor frequency
		usart_transmit_a_tagged_floating_number("speed", captured_B);	
		//offset
		usart_transmit_a_tagged_floating_number("off", captured_C*1000.0f);		
		//attenuation
		usart_transmit_a_tagged_floating_number("att", captured_D*100.0f);	
		//phase U
		usart_transmit_a_tagged_floating_number("U", captured_E);	
		//phase advance
		usart_transmit_a_tagged_floating_number("Ad", captured_F);	
		//close loop error
		usart_transmit_a_tagged_floating_number("er",captured_G);		
		//previous_hall_1
		usart_transmit_a_tagged_floating_number("h1",captured_H);	

		
		desired_rotor_frequency=usart_receive_a_floating_number();
		usart_transmit_a_tagged_floating_number("des_speed:",desired_rotor_frequency);


		usart_transmit_new_line();



	


	}	
		return 0;
}
