/*
 * Copyright (C) 2013 ARCOS-Lab Universidad de Costa Rica
 * Author: Federico Ruiz Ugalde <memeruiz@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/f4/timer.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3-plus/newlib/syscall.h>
#include <cdcacm_example.h>
#include <libopencm3-plus/cdcacm_one_serial/cdcacm.h>
#include <stdio.h>
#include <libopencm3-plus/utils/misc.h>
#include <libopencm3-plus/stm32f4discovery/leds.h>
#include <limits.h>
#include <stdbool.h>
#include "motor.h"

//pwm-related timer configuration
#define SYSFREQ     168000000 //168MHz
#define PWMFREQ        32000  //32000
#define PWMFREQ_F       ((float )(PWMFREQ)) //32000.0f
#define PRESCALE        1                                       //freq_CK_CNT=freq_CK_PSC/(PSC[15:0]+1)
#define PWM_PERIOD_ARR  SYSFREQ/( PWMFREQ*(PRESCALE+1) )
#define INIT_DUTY 0.5f
#define PI 3.1416f
#define TICK_PERIOD 1.0f/PWMFREQ_F
#define MYUINT_MAX 536870912
#define t ticks/TICK_PERIOD
#define CUR_FREQ 1.0f/(period/TICK_PERIOD)

int hall_a;
uint ticks;
uint period;
uint temp_period;
float est_angle=0;
float duty_a=0.0f;
float duty_b=0;
float duty_c=0;
float ref_freq=1;
float cur_angle=0;
float final_ref_freq=40;
float error, p_error;
float i_error=0;
float cmd_angle;
float pi_control;
int close_loop=false;
int first_closed=false;
int motor_off;

void leds_init(void) {
  rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPDEN);
  //gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
  //gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO14);
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO15);
}



void tim4_init(void) {
	/* Enable TIM4 clock. and Port D clock */
	rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM4EN);
	rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPDEN);

	//Set TIM4 channel (and complementary) output to alternate function push-pull'.
	//f4 TIM4=> GIO12: CH1, GPIO13: CH2
	gpio_mode_setup(GPIOD, GPIO_MODE_AF,GPIO_PUPD_NONE,GPIO12 | GPIO13);
	gpio_set_af(GPIOD, GPIO_AF2, GPIO12 | GPIO13);


	/* Reset TIM4 peripheral. */
	timer_reset(TIM4);


	timer_set_mode(TIM4, TIM_CR1_CKD_CK_INT, //For dead time and filter sampling, not important for now.
		       TIM_CR1_CMS_EDGE,	//TIM_CR1_CMS_EDGE
						//TIM_CR1_CMS_CENTER_1
						//TIM_CR1_CMS_CENTER_2
						//TIM_CR1_CMS_CENTER_3 la frequencia del pwm se divide a la mitad.
			 TIM_CR1_DIR_UP);

	timer_set_prescaler(TIM4, 1); //1 = disabled (max speed)
	timer_set_repetition_counter(TIM4, 0); //disabled
	timer_enable_preload(TIM4);
	timer_continuous_mode(TIM4);
	timer_slave_set_mode(TIM4, TIM_SMCR_SMS_EM3);
	timer_set_oc_polarity_high(TIM4, TIM_OC1);
	timer_set_oc_polarity_high(TIM4, TIM_OC2);
	timer_ic_enable(TIM4, TIM_IC1);
	timer_ic_enable(TIM4, TIM_IC2);
	timer_ic_set_input(TIM4, TIM_IC1, TIM_IC_IN_TI1);
	timer_ic_set_input(TIM4, TIM_IC2, TIM_IC_IN_TI1);

	/* Period (32kHz). */
	timer_set_period(TIM4, 1000); //ARR (value compared against main counter to reload counter aka: period of counter)

	/* Configure break and deadtime. */
	//timer_set_deadtime(TIM1, deadtime_percentage*pwm_period_ARR);
	//timer_set_enabled_off_state_in_idle_mode(TIM1);
	//timer_set_enabled_off_state_in_run_mode(TIM1);
	//timer_disable_break(TIM1);
	//timer_set_break_polarity_high(TIM1);
	//timer_disable_break_automatic_output(TIM1);
	//timer_set_break_lock(TIM1, TIM_BDTR_LOCK_OFF);

	/* Disable outputs. */
	timer_disable_oc_output(TIM4, TIM_OC1);
	//timer_disable_oc_output(TIM1, TIM_OC1N);
	timer_disable_oc_output(TIM4, TIM_OC2);
	//timer_disable_oc_output(TIM1, TIM_OC2N);
	timer_disable_oc_output(TIM4, TIM_OC3);
	//timer_disable_oc_output(TIM1, TIM_OC3N);

	/* -- OC1 and OC1N configuration -- */
	/* Configure global mode of line 1. */
	//timer_enable_oc_preload(TIM1, TIM_OC1);
	//timer_set_oc_mode(TIM1, TIM_OC1, TIM_OCM_PWM1);
	/* Configure OC1. */
	//timer_set_oc_polarity_high(TIM1, TIM_OC1);
	//timer_set_oc_idle_state_unset(TIM1, TIM_OC1); //When idle (braked) put 0 on output
	/* Configure OC1N. */
	//timer_set_oc_polarity_high(TIM1, TIM_OC1N);
	//timer_set_oc_idle_state_unset(TIM1, TIM_OC1N);
	/* Set the capture compare value for OC1. */
	//timer_set_oc_value(TIM1, TIM_OC1, INIT_DUTY*PWM_PERIOD_ARR);//initial_duty_cycle*pwm_period_ARR);

	/* -- OC2 and OC2N configuration -- */
	/* Configure global mode of line 2. */
	//timer_enable_oc_preload(TIM1, TIM_OC2);
	//timer_set_oc_mode(TIM1, TIM_OC2, TIM_OCM_PWM1);
	/* Configure OC2. */
	//timer_set_oc_polarity_high(TIM1, TIM_OC2);
	//timer_set_oc_idle_state_unset(TIM1, TIM_OC2);
	/* Configure OC2N. */
	//timer_set_oc_polarity_high(TIM1, TIM_OC2N);
	//timer_set_oc_idle_state_unset(TIM1, TIM_OC2N);
	/* Set the capture compare value for OC2. */
	//timer_set_oc_value(TIM1, TIM_OC2, INIT_DUTY*PWM_PERIOD_ARR);//initial_duty_cycle*pwm_period_ARR);

	/* -- OC3 and OC3N configuration -- */
	/* Configure global mode of line 3. */
	//timer_enable_oc_preload(TIM1, TIM_OC3);
	//timer_set_oc_mode(TIM1, TIM_OC3, TIM_OCM_PWM1);
	/* Configure OC3. */
	//timer_set_oc_polarity_high(TIM1, TIM_OC3);
	//timer_set_oc_idle_state_unset(TIM1, TIM_OC3);
	/* Configure OC3N. */
	//timer_set_oc_polarity_high(TIM1, TIM_OC3N);
	//timer_set_oc_idle_state_unset(TIM1, TIM_OC3N);
	/* Set the capture compare value for OC3. */
	//timer_set_oc_value(TIM1, TIM_OC3, INIT_DUTY*PWM_PERIOD_ARR);//initial_duty_cycle*pwm_period_ARR);//100);

	/* Reenable outputs. */
	/* timer_enable_oc_output(TIM1, TIM_OC1); */
	/* timer_enable_oc_output(TIM1, TIM_OC1N); */
	/* timer_enable_oc_output(TIM1, TIM_OC2); */
	/* timer_enable_oc_output(TIM1, TIM_OC2N); */
	/* timer_enable_oc_output(TIM1, TIM_OC3); */
	/* timer_enable_oc_output(TIM1, TIM_OC3N); */

	/* ---- */

	/* ARR reload enable. */
	//timer_enable_preload(TIM1);

	/*
	 * Enable preload of complementary channel configurations and
	 * update on COM event.
	 */
	//timer_enable_preload_complementry_enable_bits(TIM1);
	timer_disable_preload_complementry_enable_bits(TIM4);

	/* Enable outputs in the break subsystem. */
	//timer_enable_break_main_output(TIM1);

	/* Generate update event to reload all registers before starting*/
	//timer_generate_event(TIM1, TIM_EGR_UG);

	/* Counter enable. */
	timer_enable_counter(TIM4);

	/* Enable commutation interrupt. */
	//timer_enable_irq(TIM1, TIM_DIER_COMIE);

	/*********/
	/*Capture compare interrupt*/

	//enable capture compare interrupt
	timer_enable_update_event(TIM4);

	/* Enable commutation interrupt. */
	//timer_enable_irq(TIM1, TIM_DIER_CC1IE);	//Capture/compare 1 interrupt enable
	/* Enable commutation interrupt. */
	//timer_enable_irq(TIM1, TIM_DIER_CC1IE);
	timer_enable_irq(TIM4, TIM_DIER_UIE);
	nvic_enable_irq(NVIC_TIM4_IRQ);




}

void tim1_init(void)
{
	/* Enable TIM1 clock. and Port E clock (for outputs) */
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_TIM1EN);
	rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPEEN);

	//Set TIM1 channel (and complementary) output to alternate function push-pull'.
	//f4 TIM1=> GIO9: CH1, GPIO11: CH2, GPIO13: CH3
	//f4 TIM1=> GIO8: CH1N, GPIO10: CH2N, GPIO12: CH3N
	gpio_mode_setup(GPIOE, GPIO_MODE_AF,GPIO_PUPD_NONE,GPIO9 | GPIO11 | GPIO13);
	gpio_set_af(GPIOE, GPIO_AF1, GPIO9 | GPIO11 | GPIO13);
	gpio_mode_setup(GPIOE, GPIO_MODE_AF,GPIO_PUPD_NONE,GPIO8 | GPIO10 | GPIO12);
	gpio_set_af(GPIOE, GPIO_AF1, GPIO8 | GPIO10 | GPIO12);

	/* Enable TIM1 commutation interrupt. */
	//nvic_enable_irq(NVIC_TIM1_TRG_COM_TIM11_IRQ);	//f4

	/* Reset TIM1 peripheral. */
	timer_reset(TIM1);

	/* Timer global mode:
	 * - No divider
	 * - Alignment edge
	 * - Direction up
	 */
	timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, //For dead time and filter sampling, not important for now.
		       TIM_CR1_CMS_CENTER_3,	//TIM_CR1_CMS_EDGE
						//TIM_CR1_CMS_CENTER_1
						//TIM_CR1_CMS_CENTER_2
						//TIM_CR1_CMS_CENTER_3 la frequencia del pwm se divide a la mitad.
			 TIM_CR1_DIR_UP);

	timer_set_prescaler(TIM1, PRESCALE); //1 = disabled (max speed)
	timer_set_repetition_counter(TIM1, 0); //disabled
	timer_enable_preload(TIM1);
	timer_continuous_mode(TIM1);

	/* Period (32kHz). */
	timer_set_period(TIM1, PWM_PERIOD_ARR); //ARR (value compared against main counter to reload counter aka: period of counter)

	/* Configure break and deadtime. */
	//timer_set_deadtime(TIM1, deadtime_percentage*pwm_period_ARR);
	timer_set_enabled_off_state_in_idle_mode(TIM1);
	timer_set_enabled_off_state_in_run_mode(TIM1);
	timer_disable_break(TIM1);
	timer_set_break_polarity_high(TIM1);
	timer_disable_break_automatic_output(TIM1);
	timer_set_break_lock(TIM1, TIM_BDTR_LOCK_OFF);

	/* Disable outputs. */
	timer_disable_oc_output(TIM1, TIM_OC1);
	timer_disable_oc_output(TIM1, TIM_OC1N);
	timer_disable_oc_output(TIM1, TIM_OC2);
	timer_disable_oc_output(TIM1, TIM_OC2N);
	timer_disable_oc_output(TIM1, TIM_OC3);
	timer_disable_oc_output(TIM1, TIM_OC3N);

	/* -- OC1 and OC1N configuration -- */
	/* Configure global mode of line 1. */
	timer_enable_oc_preload(TIM1, TIM_OC1);
	timer_set_oc_mode(TIM1, TIM_OC1, TIM_OCM_PWM1);
	/* Configure OC1. */
	timer_set_oc_polarity_high(TIM1, TIM_OC1);
	timer_set_oc_idle_state_unset(TIM1, TIM_OC1); //When idle (braked) put 0 on output
	/* Configure OC1N. */
	timer_set_oc_polarity_high(TIM1, TIM_OC1N);
	timer_set_oc_idle_state_unset(TIM1, TIM_OC1N);
	/* Set the capture compare value for OC1. */
	timer_set_oc_value(TIM1, TIM_OC1, INIT_DUTY*PWM_PERIOD_ARR);//initial_duty_cycle*pwm_period_ARR);

	/* -- OC2 and OC2N configuration -- */
	/* Configure global mode of line 2. */
	timer_enable_oc_preload(TIM1, TIM_OC2);
	timer_set_oc_mode(TIM1, TIM_OC2, TIM_OCM_PWM1);
	/* Configure OC2. */
	timer_set_oc_polarity_high(TIM1, TIM_OC2);
	timer_set_oc_idle_state_unset(TIM1, TIM_OC2);
	/* Configure OC2N. */
	timer_set_oc_polarity_high(TIM1, TIM_OC2N);
	timer_set_oc_idle_state_unset(TIM1, TIM_OC2N);
	/* Set the capture compare value for OC2. */
	timer_set_oc_value(TIM1, TIM_OC2, INIT_DUTY*PWM_PERIOD_ARR);//initial_duty_cycle*pwm_period_ARR);

	/* -- OC3 and OC3N configuration -- */
	/* Configure global mode of line 3. */
	timer_enable_oc_preload(TIM1, TIM_OC3);
	timer_set_oc_mode(TIM1, TIM_OC3, TIM_OCM_PWM1);
	/* Configure OC3. */
	timer_set_oc_polarity_high(TIM1, TIM_OC3);
	timer_set_oc_idle_state_unset(TIM1, TIM_OC3);
	/* Configure OC3N. */
	timer_set_oc_polarity_high(TIM1, TIM_OC3N);
	timer_set_oc_idle_state_unset(TIM1, TIM_OC3N);
	/* Set the capture compare value for OC3. */
	timer_set_oc_value(TIM1, TIM_OC3, INIT_DUTY*PWM_PERIOD_ARR);//initial_duty_cycle*pwm_period_ARR);//100);

	/* Reenable outputs. */
	timer_enable_oc_output(TIM1, TIM_OC1);
	timer_enable_oc_output(TIM1, TIM_OC1N);
	timer_enable_oc_output(TIM1, TIM_OC2);
	timer_enable_oc_output(TIM1, TIM_OC2N);
	timer_enable_oc_output(TIM1, TIM_OC3);
	timer_enable_oc_output(TIM1, TIM_OC3N);

	/* ---- */

	/* ARR reload enable. */
	timer_enable_preload(TIM1);

	/*
	 * Enable preload of complementary channel configurations and
	 * update on COM event.
	 */
	//timer_enable_preload_complementry_enable_bits(TIM1);
	timer_disable_preload_complementry_enable_bits(TIM1);

	/* Enable outputs in the break subsystem. */
	timer_enable_break_main_output(TIM1);

	/* Generate update event to reload all registers before starting*/
	timer_generate_event(TIM1, TIM_EGR_UG);

	/* Counter enable. */
	timer_enable_counter(TIM1);

	/* Enable commutation interrupt. */
	//timer_enable_irq(TIM1, TIM_DIER_COMIE);

	/*********/
	/*Capture compare interrupt*/

	//enable capture compare interrupt
	timer_enable_update_event(TIM1);

	/* Enable commutation interrupt. */
	//timer_enable_irq(TIM1, TIM_DIER_CC1IE);	//Capture/compare 1 interrupt enable
	/* Enable commutation interrupt. */
	//timer_enable_irq(TIM1, TIM_DIER_CC1IE);
	timer_enable_irq(TIM1, TIM_DIER_UIE);
	nvic_enable_irq(NVIC_TIM1_UP_TIM10_IRQ);
}

void system_init(void) {
  rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);
  leds_init();
  cdcacm_init();
  printled(4, LRED);
  tim1_init();
  tim4_init();
}

/* void calc_freq(void) { */
/*   static first=true; */
/*   static int hall_a_last=0; */
/*   static uint last_fall_hall_a_ticks=0; */
/*   hall_a=HALL_A(); */
/*   if (first) { */
/*     hall_a_last=hall_a; */
/*     ticks=0; */
/*     period=UINT_MAX; */
/*     last_fall_hall_a_ticks=ticks; */
/*     first=false; */
/*   } else { */
/*     ticks++; */
/*     if (ticks == UINT_MAX) { */
/*       ticks=0; */
/*     } */
/*     if ((hall_a_last > 0) && (hall_a == 0)) { */
/*       //hall falling edge: new cycle */
/*       //new rotor position measurement */
/*       est_angle=0; */
/*       //gpio_toggle(LGREEN); */
/*       if (ticks > last_fall_hall_a_ticks) { //updating period */
/* 	period=ticks-last_fall_hall_a_ticks; */
/*       } else { */
/* 	period=UINT_MAX-last_fall_hall_a_ticks+ticks; */
/*       } */
/*       last_fall_hall_a_ticks=ticks; */
/*     } else { */
/*       //we update period only if is bigger than before */
/*       if (ticks > last_fall_hall_a_ticks) { */
/* 	temp_period=ticks-last_fall_hall_a_ticks; */
/*       } else { */
/* 	temp_period=UINT_MAX-last_fall_hall_a_ticks+ticks; */
/*       } */
/*       if (temp_period > period) { */
/* 	period=temp_period; */
/*       } */
/*       //update estimated current angle */
/*       est_angle+=2.0f*PI*TICK_PERIOD/(period/TICK_PERIOD); */
/*       if (est_angle > 2.0f*PI) { */
/* 	est_angle=0; */
/*       } */
/*     } */
/*   } */
/*   hall_a_last=hall_a; */
/* } */

void pi_controller(void) {
  error=ref_freq-CUR_FREQ; // ref_freq-cur_freq
  if (error > 0.0f) {
    p_error=P*error;
  } else {
    p_error=P_DOWN*error;
  }
  if (error > 0.0f) {
    i_error+=I*error;
  } else {
    i_error+=I_DOWN*error;
  }
  if (i_error > I_MAX){
    i_error=I_MAX;
  }
  if (i_error < -I_MAX) {
    i_error=-I_MAX;
  }
  if (p_error > P_MAX) {
    p_error=P_MAX;
  }
  if (p_error < -P_MAX) {
    p_error= -P_MAX;
  }
  pi_control=p_error+i_error;
  if (pi_control > PI_MAX) {
    pi_control = PI_MAX;
  }
  if (pi_control < PI_MIN) {
    pi_control = PI_MIN;
  }
  cmd_angle+=pi_control;
}

void gen_pwm(void) {
  //calc_attenuation();

  static float pi_times;

  //pi_controller();
  duty_a=ref_freq/10.0f;
  duty_b=ref_freq/10.0f;
  duty_c=ref_freq/10.0f;

  if (duty_a < 0.0f)
    {
      timer_set_oc_mode(TIM1, TIM_OC1, TIM_OCM_PWM1);
      timer_disable_oc_output(TIM1,TIM_OC1);
      timer_enable_oc_output (TIM1, TIM_OC1N);
      duty_a=-duty_a;
    }
  else
    {
      timer_set_oc_mode(TIM1, TIM_OC1, TIM_OCM_PWM1);
      timer_enable_oc_output(TIM1, TIM_OC1 );
      timer_disable_oc_output (TIM1, TIM_OC1N);
    }
  if (duty_b < 0.0f)
    {
      timer_set_oc_mode(TIM1, TIM_OC2, TIM_OCM_PWM1);
      timer_disable_oc_output(TIM1, TIM_OC2 );
      timer_enable_oc_output (TIM1, TIM_OC2N);
      duty_b=-duty_b;
    }
  else
    {
      timer_set_oc_mode(TIM1, TIM_OC2, TIM_OCM_PWM1);
      timer_enable_oc_output(TIM1, TIM_OC2 );
      timer_disable_oc_output (TIM1, TIM_OC2N);
    }
  if (duty_c < 0.0f)
    {
      timer_set_oc_mode(TIM1, TIM_OC3, TIM_OCM_PWM1);
      timer_disable_oc_output(TIM1, TIM_OC3 );
      timer_enable_oc_output (TIM1, TIM_OC3N);
      duty_c=-duty_c;
    }
  else
    {
      timer_set_oc_mode(TIM1, TIM_OC3, TIM_OCM_PWM1);
      timer_enable_oc_output(TIM1, TIM_OC3 );
      timer_disable_oc_output (TIM1, TIM_OC3N);
    }

  /* Set the capture compare value for OC1. */
  timer_set_oc_value(TIM1, TIM_OC1, duty_a*PWM_PERIOD_ARR);
  /* Set the capture compare value for OC1. */
  timer_set_oc_value(TIM1, TIM_OC2, duty_b*PWM_PERIOD_ARR);
  /* Set the capture compare value for OC1. */
  timer_set_oc_value(TIM1, TIM_OC3, duty_c*PWM_PERIOD_ARR);
  //tim_force_update_event(TIM1);
}

void tim1_up_tim10_isr(void) {
  // Clear the update interrupt flag
  timer_clear_flag(TIM1,  TIM_SR_UIF);
  gen_pwm();
}

void tim4_isr(void) {
  // Clear the update interrupt flag
  timer_clear_flag(TIM4,  TIM_SR_UIF);
}

int main(void)
{
  system_init();
  char cmd_s[50]="";
  char cmd[10]="";
  int i;
  int c=0;
  uint32_t encoder;
  setvbuf(stdin,NULL,_IONBF,0); // Sets stdin in unbuffered mode (normal for usart com)
  setvbuf(stdout,NULL,_IONBF,0); // Sets stdin in unbuffered mode (normal for usart com)
  //printled(3, LRED);
  int counter=0;
  int new_freq=0;
  float value;
  while (poll(stdin) > 0) {
    printf("Cleaning stdin\n");
    getc(stdin);
  }
  int motor_stop=false;
  motor_off=false;
  while (1){
    counter++;
    //printled(1, LRED);
    if ((poll(stdin) > 0)) {
      i=0;
      if (poll(stdin) > 0) {
	c=0;
	while (c!='\r') {
	  c=getc(stdin);
	  cmd_s[i]=c;
	  i++;
	  putc(c, stdout);
	//fflush(stdout);
	}
	cmd_s[i]='\0';
      }
      printf("%s", cmd_s);
      sscanf(cmd_s, "%s %f", cmd, &value);
      if (strcmp(cmd, "f") == 0){ //set ref freq
	printf("New reference frequency: %f. Confirm? (Press \"y\")\n", value);
	ref_freq=value;
	//printled(2, LRED);
      }
    }
    encoder=timer_get_counter(TIM4);
    /* if (!close_loop) { */
    /*   while (poll(stdin) > 0) { */
    /* 	getc(stdin); */
    /*   } */
    /* } else { */
    /*   ref_freq=value; */
    /*   //printf("Close loop\n"); */
    /* } */
    //printf(" e: %7.2f, e_p %6.2f, e_i: %6.2f, adv: %6.2f, c_f: %6.2f, r_f: %6.2f, att: %6.2f, counter %d, eof %d, buf: %s, v %f\n", error, p_error, i_error, pi_control*180.0f/PI, 1.0f/(period/TICK_PERIOD), ref_freq, attenuation, counter, eof, cmd, value);
    printf("ref_freq %6.2f, duty_a: %6.2f, encoder: %d\n", ref_freq, duty_a, encoder);

  }
}