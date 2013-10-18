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
#include "sine_table.h"
//#include "cos_table.h"
#include "atan_table.h"

float fast_sine(float angle);
float fast_cos(float angle); 
float fast_atan(float tan_value);
//float fast_vector_angle(float quadrature, float direct);
float fast_sine(float angle)
{
  int rounded_angle; 
 
  while (angle>=360.0f) { angle=angle-360.0f; }
  while (angle<   0.0f) { angle=angle+360.0f; }
  
  rounded_angle=(( int )(angle*SIN_TABLE_SIZE_F/360.0f));
  
  return sine_table[rounded_angle];

}
/*
float fast_cos(float angle)
{
  int rounded_angle; 


  while (angle>=360.0f) { angle=angle-360.0f; }
  while (angle<   0.0f) { angle=angle+360.0f; }

 
  rounded_angle=(( int )(angle*COS_TABLE_SIZE_F/360.0f));

  

  return cos_table[rounded_angle];
}
*/

float fast_cos(float angle)
{
  //int rounded_angle; 

 
  //rounded_angle=(( int )( (angle+90.0f)*SIN_TABLE_SIZE_F/360.0f));

  

  return fast_sine(angle+90.0f);//sine_table[rounded_angle];
}




float fast_atan(float tan_value)
{
  int rounded_tan;
  rounded_tan=(( int )(tan_value*ATAN_TABLE_SIZE_F));

  
  if  (rounded_tan<0.0f || rounded_tan>ATAN_TABLE_SIZE) { return 0.0f; }
  else                                                  { return atan_table[rounded_tan]; }
}


