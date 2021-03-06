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


void exti0_isr(void)
{
	//gpio_toggle(GPIOD, GPIO12);
	exti_reset_request(EXTI0);

	button=1;

/*	

	exti_reset_request(EXTI0);

	if (exti_direction == FALLING) 
	{
		//gpio_toggle(GPIOD, GPIO13);
		exti_direction = RISING;
		exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);

		V_hall_1_V1=0.0f;	
	} 

	else 
	{
		 //gpio_toggle(GPIOD, GPIO14);
		
		exti_direction = FALLING;
		exti_set_trigger(EXTI0, EXTI_TRIGGER_FALLING);
		
		V_hall_1_V1=3.0f;

	}
*/

}





/*
void exti1_isr(void)
{
	exti_reset_request(EXTI1);

	if (exti_direction == FALLING) 
	{
		gpio_toggle(GPIOD, GPIO13);
		exti_direction = RISING;
		exti_set_trigger(EXTI1, EXTI_TRIGGER_RISING);

		V_hall_1_V1=0.0f;	
	} 

	else 
	{
		 gpio_toggle(GPIOD, GPIO14);
		
		exti_direction = FALLING;
		exti_set_trigger(EXTI1, EXTI_TRIGGER_FALLING);
		
		V_hall_1_V1=3.0f;

	}
	button=1;
}
*/


void tim1_cc_isr (void)
{
	static float attenuation=0.0f;

	// Clear the update interrupt flag
	timer_clear_flag(TIM1,  TIM_SR_CC1IF);	

	//pwm duty cycle calculation
	pwm(phase_A_stator_angle,attenuation);
		
	if (button==1)
	{
		PID_control_loop(&attenuation);
	}

}



