/* Copyright (c) 2015, Admar Schoonen
 * All rights reserved.
 *
 * License: New BSD License. See also license.txt
 */

#include "restouch.h"
#include "Arduino.h"
#include "configuration.h"

//#define RES_BAR_MAX 65536
#define RES_BAR_MAX 30000
#define RES_BAR_LENGTH 160
#if ((50 * RES_N_SENSORS + 2) < (RES_BAR_LENGTH + 1))
#define RES_BUF_LENGTH (RES_BAR_LENGTH + 5)
#else
#define RES_BUF_LENGTH (50 * RES_N_SENSORS + 6)
#endif

/* Arduino Mega does not have INTERNAL defined, but does have INTERNAL1V1
 * Define INTERNAL here for compatibility
 */
#ifndef INTERNAL
#define INTERNAL INTERNAL1V1
#endif

restouch_t restouch;

unsigned char restouch_scanorder[] = { \
  3, 2, 3, 2, 1, 1, 1, 3, 2, 0, 1, 1, 2, 2, 3, 2, \
  1, 3, 3, 0, 1, 0, 1, 3, 0, 2, 3, 3, 1, 2, 2, 1, \
  1, 3, 3, 2, 3, 2, 1, 0, 3, 3, 2, 2, 2, 2, 3, 2, \
  2, 2, 0, 2, 2, 1, 0, 1, 0, 3, 0, 2, 3, 2, 2, 1, \
  1, 0, 0, 0, 1, 3, 2, 2, 3, 0, 2, 1, 3, 0, 3, 2, \
  2, 0, 1, 3, 0, 0, 3, 1, 0, 3, 1, 2, 1, 1, 0, 3, \
  1, 1, 0, 2, 0, 0, 2, 3, 1, 1, 3, 3, 3, 1, 1, 0, \
  0, 1, 1, 0, 0, 3, 3, 3, 0, 0, 0, 2, 0, 2, 3, 3, \
  1, 2, 3, 2, 1, 0, 3, 1, 2, 2, 0, 0, 2, 1, 1, 2, \
  0, 0, 1, 1, 3, 2, 0, 0, 1, 2, 0, 0, 0, 2, 2, 0, \
  3, 3, 3, 3, 3, 3, 2, 0, 3, 1, 2, 3, 1, 2, 0, 0, \
  2, 0, 1, 3, 0, 3, 3, 3, 1, 2, 1, 3, 2, 1, 0, 1, \
  1, 3, 1, 1, 0, 3, 3, 2, 0, 0, 1, 3, 2, 2, 1, 0, \
  0, 0, 3, 2, 3, 0, 3, 1, 3, 3, 1, 0, 3, 2, 1, 0, \
  0, 1, 2, 2, 1, 0, 0, 3, 1, 2, 1, 1, 1, 0, 2, 0, \
  1, 3, 2, 0, 2, 3, 2, 2, 1, 2, 0, 0, 1, 2, 3, 1 \
};

void restouch_vcc(int enable)
{
  int n;

  if (enable)
    #if (SENSOR_TYPE == SENSOR_TYPE_VELOSTAT)
    //analogReference(INTERNAL);
    analogReference(DEFAULT);
    #else
    analogReference(DEFAULT);
    #endif
  else
    analogReference(DEFAULT);

  n = 0;
  while (n < RES_N_SENSORS) {
    if (enable) {
      // turn on pull up resistor on analog input
      digitalWrite(A0 + RES_SENSORS(n), HIGH);
      //pinMode(RES_VCCS(n), OUTPUT);
      //digitalWrite(RES_VCCS(n), HIGH);
    } else {
      // turn off pull up resistor on analog input
      digitalWrite(A0 + RES_SENSORS(n), LOW);
      //pinMode(RES_VCCS(n), INPUT);
      //digitalWrite(RES_VCCS(n), LOW);
    }
    
    n = n + 1;
  }
}

void restouch_gnd(int enable)
{
  int n;
  
  n = 0;
  while (n < RES_N_SENSORS) {
    if (enable) {
      // use IO pin as ground
      pinMode(RES_GNDS(n), OUTPUT);
      digitalWrite(RES_GNDS(n), LOW);
    } else {
      // set IO pin floating + disable pull up
      pinMode(RES_GNDS(n), INPUT);
      digitalWrite(RES_GNDS(n), LOW);
    }
    
    n = n + 1;
  }
}

void restouch_init(void)
{
}

void restouch_get_readings(void)
{
  int ch, ch_pin, index, i;
  
  for (ch = 0; ch < RES_N_SENSORS; ch++)
    restouch.readings[ch] = 0;
 
  for (index = 0; index < RES_N_SENSORS * RES_OVERSAMPLING; index++) {
    ch = restouch_scanorder[index];
    
    ch_pin = RES_SENSORS(ch);
    
    // turn on pull up resistor on analog input
    digitalWrite(A0 + ch_pin, HIGH);
    
    // connect other side to GND
    pinMode(RES_GNDS(ch), OUTPUT);
    digitalWrite(RES_GNDS(ch), LOW);
    
    // read sensor
    restouch.readings[ch] += analogRead(ch_pin);
  }
}

void restouch_process_readings(void)
{
  int n;
  
  for (n = 0; n < RES_N_SENSORS; n++) {
    switch (restouch.states[n]) {
    case res_state_calibrating:
      if (restouch.counters[n] < RES_N_CALIBRATION_SAMPLES) {
        restouch.avgs[n] = ((float) (restouch.counters[n] * restouch.avgs[n] + \
            restouch.readings[n])) / (restouch.counters[n] + 1);
        //restouch.avgs_sq[n] = ((float) (restouch.counters[n] * restouch.avgs_sq[n] + restouch.readings[n] * restouch.readings[n])) / (restouch.counters[n] + 1);
        restouch.counters[n]++;
      }
      if (restouch.counters[n] == RES_N_CALIBRATION_SAMPLES)
        restouch.states[n] = res_state_released;
      break;
    case res_state_released:
      if (restouch.readings[n] < restouch.avgs[n] - RES_THRESHOLD_RELEASED_TO_PRESSED(n) ) {
        restouch.counters[n] = 1;
        restouch.states[n] = res_state_released_to_pressed;
      } else {
        restouch.avgs[n] = ((float) ((RES_N_FILTER_SAMPLES - 1) * restouch.avgs[n] + \
            restouch.readings[n])) / RES_N_FILTER_SAMPLES;
        //restouch.avgs_sq[n] = ((float) (restouch.counters[n] * restouch.avgs_sq[n] + restouch.readings[n] * restouch.readings[n])) / (restouch.counters[n] + 1);
      }
      break;
    case res_state_released_to_pressed:
      // do not update average in this state
      if (restouch.readings[n] >= restouch.avgs[n] - RES_THRESHOLD_RELEASED_TO_PRESSED(n)) {
        // button is released; fall back to released
        restouch.counters[n] = 1;
        restouch.recal_counters[n] = 1;
        restouch.states[n] = res_state_released;
      } else {
        if (restouch.counters[n] + 1 < RES_HYSTERESIS_RELEASED_TO_PRESSED) {
          restouch.counters[n]++;
        } else {
          restouch.counters[n] = 1;
          restouch.states[n] = res_state_pressed;
        }
      }
      break;
    case res_state_pressed:
      if (restouch.readings[n] < restouch.avgs[n] - RES_THRESHOLD_PRESSED_TO_RELEASED(n)) {
        // button is still pressed
        if (restouch.recal_counters[n] + 1 > RES_N_RECALIBRATION_SAMPLES) {
           // button is held too long -> recalibrate
           restouch.counters[n] = 0;
           restouch.recal_counters[n] = 0;
           restouch.avgs[n] = 0;
           restouch.states[n] = res_state_calibrating;
        } else {
          restouch.recal_counters[n]++;
        }
      } else {
        restouch.counters[n] = 1;
        restouch.states[n] = res_state_pressed_to_released;
      }
      break;
    case res_state_pressed_to_released:
      if (restouch.readings[n] < restouch.avgs[n] - RES_THRESHOLD_PRESSED_TO_RELEASED(n)) {
        // button is still pressed
        restouch.states[n] = res_state_pressed;
        restouch.recal_counters[n]++;
      } else {
        if (restouch.counters[n] + 1 < RES_HYSTERESIS_PRESSED_TO_RELEASED) {
          restouch.counters[n]++;
        } else {
          restouch.counters[n] = 1;
          restouch.states[n] = res_state_released;
        }
      }
      break;
    }
  }
}

void restouch_serial_write(char *s)
{
  unsigned int n, N;
  
  N = strlen(s);
  
  for (n = 0; n < N; n++)
    Serial.print(s[n]);
}


void restouch_print_debug()
{
  int n, buf_index;
  char buf[RES_BUF_LENGTH];
  
  n = 0;
  buf_index = 0;
  while (n < RES_N_SENSORS) {
   buf_index += snprintf(&(buf[buf_index]), RES_BUF_LENGTH - buf_index, \
       "   raw: % 5u; avg: % 5u; delta: % 5d; state: %1d", \
       restouch.readings[n], (int) restouch.avgs[n], ((int) restouch.avgs[n]) \
       - restouch.readings[n], restouch.states[n]);
     n = n + 1;
  }
  // new line
  //Serial.println();
  snprintf(&(buf[buf_index]), RES_BUF_LENGTH - buf_index, "\t");
  restouch_serial_write(buf);
}

void restouch_process_bar(int n)
{
  int K;
  
  if (restouch.avgs[n] - restouch.readings[n] < 0)
    K = 0;
  else if (restouch.avgs[n] - restouch.readings[n] > RES_BAR_MAX)
    K = RES_BAR_LENGTH;
  else
    K = (restouch.avgs[n] - restouch.readings[n]) * (RES_BAR_LENGTH) / RES_BAR_MAX;
    
  restouch.bar[n] = K;
}

void restouch_print_bar(int n)
{
  int k, buf_index = 0;
  char buf[RES_BUF_LENGTH];
  
  restouch_process_bar(n);
  
//  for (k = 0; k < 4; k++)
//    buf[buf_index++] = ' ';

  for (k = 0; k < restouch.bar[n]; k++)
    buf[buf_index++] = '=';
  
  for (; k < RES_BAR_LENGTH; k++)
    buf[buf_index++] = '.';
  
  buf[buf_index++] = '\0';
  
  restouch_serial_write(buf);
}
