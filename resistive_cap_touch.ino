/* Capacitive and resistive touch demo
 * Sensor 1: top layer (capacitive touch): pin A0, bottom pin 2
 * Sensor 2: top layer (capacitive touch): pin A1, bottom pin 3
 * Sensor 3: top layer (capacitive touch): pin A2, bottom pin 4
 * Sensor 4: top layer (capacitive touch): pin A3, bottom pin 5
 * 
 * Select correct sensor type in configuration.h
 *
 * Warning! There is (probably) a bug in the code causing the resistive
 * touch of sensor 4 to be much less responsive.
 *
 * Copyright (c) 2015, Admar Schoonen
 * All rights reserved.
 *
 * License: New BSD License. See also license.txt
 */

#include "captouch.h"
#include "restouch.h"
#include "configuration.h"

#define BAR_N_SENSORS 4
#define BAR_LENGTH (40 * 4 / BAR_N_SENSORS)

void serial_write(char *s)
{
  unsigned int n, N;
  
  N = strlen(s);
  
  for (n = 0; n < N; n++)
    Serial.print(s[n]);
}

void print_bar(void)
{
  int n, N, r, c, k, K, index = 0;
  char buf[BAR_LENGTH + 7];
  
  if (CAP_N_SENSORS < RES_N_SENSORS)
    N = RES_N_SENSORS;
  else
    N = CAP_N_SENSORS;
   
  if (N > BAR_N_SENSORS)
    N = BAR_N_SENSORS;
  
  for (n = 0; n < N; n++) {
    captouch_process_bar(n);
    restouch_process_bar(n);
    
    if (n >= CAP_N_SENSORS)
      c = 0;
    else
      c = captouch.bar[n] / BAR_N_SENSORS;
    
    if (n >= RES_N_SENSORS)
      r = 0;
    else
      r = restouch.bar[n] / BAR_N_SENSORS;
    
    for (k = 0; k < BAR_LENGTH; k++) {
      if (k < r) {
        buf[k] = '#';
      } else if (k < c - 1) {
        buf[k] = '-';
      } else if (k == c - 1) {
        buf[k] = '*';
      } else {
        buf[k] = ' ';
      }
    }
    buf[k++] = ' ';
    buf[k++] = ' ';
    buf[k++] = ' ';
    buf[k++] = '|';
    buf[k++] = ' ';
    buf[k++] = ' ';
    buf[k++] = ' ';
    buf[k++] = '\0';
    
    serial_write(buf);
  }
}

void setup()
{
  captouch_init();
  restouch_init();
  Serial.begin(115200);
  Serial.println("resistive_cap_touch");
}

void loop()
{
  int n;
  
  #if (SENSOR_TYPE == SENSOR_TYPE_KNITTED)
    /* Knitted sensor has a mesh at the back. This seems to be just close enough to provide some shielding
    while still open enough to not affect captouch signal too much --> can be worn on body */
    restouch_gnd(1);
  #endif

  captouch_get_readings();
  captouch_process_readings();
//  captouch_print_debug();
  
  restouch_vcc(1);
  restouch_gnd(1);

  restouch_get_readings();
  restouch_process_readings();
//  restouch_print_debug();

  print_bar();
  
  Serial.println();

  restouch_gnd(0);
  restouch_vcc(0);

}

