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


float w_r=0.0f;


#define I_MAX_SENSORLESS            45.0f//  0.0005f//(90.0f*frequency/interrupt_frequency) 
#define P_MAX_SENSORLESS            45.0f//  0.0005f//(90.0f*frequency/interrupt_frequency) 
#define PI_MAX_SENSORLESS           45.0f//  0.0005f//(90.0f*frequency/interrupt_frequency) 
#define PI_MIN_SENSORLESS          -45.0f// -0.0005f//-(90.0f*frequency/interrupt_frequency) 

float SVM_pi_control=0.0f;
float psi_rotating_angle_SVM=0.0f;
float phase_advance_SVM=0.0f;
float pi_max=0.0f;





void sensorless_speed_pi_controller(
                           float reference_frequency, float frequency, float* rotating_angle) 
{
  float        sensorless_error         = 0.0f;
  float        p_sensorless_error       = 0.0f;
  static float i_sensorless_error       = 0.0f;
  float        pi_control_sensorless    = 0.0f;

  sensorless_error=reference_frequency-frequency;

  if (sensorless_error > 0.0f) {  p_sensorless_error  = P_SENSORLESS      * sensorless_error;
                                  i_sensorless_error += (I_SENSORLESS      * sensorless_error); } 
  else                         {  p_sensorless_error  = P_DOWN_SENSORLESS * sensorless_error;
                                  i_sensorless_error += (I_DOWN_SENSORLESS * sensorless_error); }

  if      (i_sensorless_error >  I_MAX) { i_sensorless_error =  I_MAX; }
  else if (i_sensorless_error < -I_MAX) { i_sensorless_error = -I_MAX; }

  if      (p_sensorless_error >  P_MAX_SENSORLESS) { p_sensorless_error =  P_MAX_SENSORLESS; }
  else if (p_sensorless_error < -P_MAX_SENSORLESS) { p_sensorless_error = -P_MAX_SENSORLESS; }


  static float nan_counter=0;
  if (i_sensorless_error!=i_sensorless_error)
    {
      i_sensorless_error=0.0f;
      nan_counter+=1;
    }
    
  pi_control_sensorless=p_sensorless_error+i_sensorless_error;

  if      (pi_control_sensorless > PI_MAX_SENSORLESS) { pi_control_sensorless = PI_MAX_SENSORLESS; }
  else if (pi_control_sensorless < PI_MIN_SENSORLESS) { pi_control_sensorless = PI_MIN_SENSORLESS; }



  if (reference_frequency!=0.0f)
    *rotating_angle=pi_control_sensorless;
  else 
    *rotating_angle=0.0f;  


  SVM_pi_control=*rotating_angle;           //pi_control_sensorless
  phase_advance_SVM=pi_control_sensorless;  //*rotating_angle;//pi_control_sensorless;
  //pi_max=*rotating_angle;                 //P_MAX_SENSORLESS;//*interrupt_frequency;
  pi_max=P_MAX_SENSORLESS;
}


void sensorless_open_loop(
     float *reference_frequency, float* sensorless_attenuation,float interrupt_frequency,float max_frequency,float frequency_increment)
{
  static int cicle_counter=0.0f;

  *sensorless_attenuation=MIN_ATTENUATION;


  if (cicle_counter >= interrupt_frequency/10.0f && *reference_frequency <= max_frequency)
  {
    cicle_counter=0;
    *reference_frequency=*reference_frequency+frequency_increment;
  } 

   cicle_counter=cicle_counter+1;

}


float psi_advance_calculator(float reference_frequency, float interrupt_frequency)
{
  return 360.0f*reference_frequency/interrupt_frequency;
}



#define I_MAX_SENSORLESS_TORQUE            45.0f//  0.005f//(90.0f*frequency/interrupt_frequency) 
#define P_MAX_SENSORLESS_TORQUE            45.0f//  0.005f//(90.0f*frequency/interrupt_frequency) 
#define PI_MAX_SENSORLESS_TORQUE           45.0f//  0.005f//(90.0f*frequency/interrupt_frequency) 
#define PI_MIN_SENSORLESS_TORQUE          -45.0f//  -0.005f//-(90.0f*frequency/interrupt_frequency) 


void sensorless_torque_pi_controller(
                           float reference_torque, float torque, float* rotating_angle) 
{
  float        sensorless_error         = 0.0f;
  float        p_sensorless_error       = 0.0f;
  static float i_sensorless_error       = 0.0f;
  float        pi_control_sensorless    = 0.0f;

  sensorless_error=reference_torque-torque;

  if (sensorless_error > 0.0f) {  p_sensorless_error  = P_SENSORLESS_TORQUE      * sensorless_error;
                                  i_sensorless_error += I_SENSORLESS_TORQUE      * sensorless_error; } 
  else                         {  p_sensorless_error  = P_DOWN_SENSORLESS_TORQUE * sensorless_error;
                                  i_sensorless_error += I_DOWN_SENSORLESS_TORQUE * sensorless_error; }

  if      (i_sensorless_error >  I_MAX_SENSORLESS_TORQUE) { i_sensorless_error =  I_MAX_SENSORLESS_TORQUE; }
  else if (i_sensorless_error < -I_MAX_SENSORLESS_TORQUE) { i_sensorless_error = -I_MAX_SENSORLESS_TORQUE; }

  if      (p_sensorless_error >  P_MAX_SENSORLESS_TORQUE) { p_sensorless_error =  P_MAX_SENSORLESS_TORQUE; }
  else if (p_sensorless_error < -P_MAX_SENSORLESS_TORQUE) { p_sensorless_error = -P_MAX_SENSORLESS_TORQUE; }

  pi_control_sensorless=p_sensorless_error+i_sensorless_error;

  if      (pi_control_sensorless > PI_MAX_SENSORLESS_TORQUE) { pi_control_sensorless = PI_MAX_SENSORLESS_TORQUE; }
  else if (pi_control_sensorless < PI_MIN_SENSORLESS_TORQUE) { pi_control_sensorless = PI_MIN_SENSORLESS_TORQUE; }


  if (reference_torque!=0)
    *rotating_angle=pi_control_sensorless;
  
  else 
    *rotating_angle=0.0f;


  SVM_pi_control=pi_control_sensorless;
  phase_advance_SVM=pi_control_sensorless;
  pi_max=P_MAX_SENSORLESS_TORQUE;
}


/*
void sensorless_torque_pi_controller_from_speed(
                           float reference_torque, float torque,float switching_frequency, float* rotating_angle,float frequency,float  *reference_frequency) 
{
  float        sensorless_error         = 0.0f;
  float        p_sensorless_error       = 0.0f;
  static float i_sensorless_error       = 0.0f;
  float        pi_control_sensorless    = 0.0f;

  sensorless_error=reference_torque-torque;

  if (sensorless_error > 0.0f) {  p_sensorless_error  = P_SENSORLESS_TORQUE      * sensorless_error;
                                  i_sensorless_error += I_SENSORLESS_TORQUE      * sensorless_error; } 
  else                         {  p_sensorless_error  = P_DOWN_SENSORLESS_TORQUE * sensorless_error;
                                  i_sensorless_error += I_DOWN_SENSORLESS_TORQUE * sensorless_error; }

  if      (i_sensorless_error >  I_MAX_SENSORLESS_TORQUE) { i_sensorless_error =  I_MAX_SENSORLESS_TORQUE; }
  else if (i_sensorless_error < -I_MAX_SENSORLESS_TORQUE) { i_sensorless_error = -I_MAX_SENSORLESS_TORQUE; }

  if      (p_sensorless_error >  P_MAX_SENSORLESS_TORQUE) { p_sensorless_error =  P_MAX_SENSORLESS_TORQUE; }
  else if (p_sensorless_error < -P_MAX_SENSORLESS_TORQUE) { p_sensorless_error = -P_MAX_SENSORLESS_TORQUE; }

  pi_control_sensorless=p_sensorless_error+i_sensorless_error;

  if      (pi_control_sensorless > PI_MAX_SENSORLESS_TORQUE) { pi_control_sensorless = PI_MAX_SENSORLESS_TORQUE; }
  else if (pi_control_sensorless < PI_MIN_SENSORLESS_TORQUE) { pi_control_sensorless = PI_MIN_SENSORLESS_TORQUE; }


  if (w_r<300.0f && w_r>-300)
  {
        
  }
static float  increment= 0.0f;
if (reference_torque != 0.0f && w_r<400.0f)
   {
    
    increment = increment +0.001f;
    *reference_frequency=*reference_frequency+increment;//0.04160f;//pi_control_sensorless;
   }
else 
    {
        increment=0.0f;
    }

if (reference_torque == 0)
    *reference_frequency=0.0f;
    increment=0.0f;
    

//#define W_CUTOFF_TORQUE 100.0f
    // *rotating_angle=(*rotating_angle+pi_control_sensorless)/(1.0f+switching_frequency*W_CUTOFF_TORQUE);
    



  SVM_pi_control=pi_control_sensorless;
  phase_advance_SVM=pi_control_sensorless;
  pi_max=P_MAX_SENSORLESS_TORQUE;



}
*/

