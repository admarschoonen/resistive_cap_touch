/* Copyright (c) 2015, Admar Schoonen
 * All rights reserved.
 *
 * License: New BSD License. See also license.txt
 */

#include "captouch.h"
#include "Arduino.h"
#include <math.h>

//#define CAP_BAR_MIN 500
//#define CAP_BAR_MAX 950
#define CAP_BAR_MIN 400
#define CAP_BAR_MAX 850
#define CAP_BAR_LENGTH 160
#if ((50 * CAP_N_SENSORS + 2) < (CAP_BAR_LENGTH + 1))
#define CAP_BUF_LENGTH (CAP_BAR_LENGTH + 5)
#else
#define CAP_BUF_LENGTH (50 * CAP_N_SENSORS + 6)
#endif

// captouch_scanorder is of sizeCAP_N_SENSORS * CAP_OVERSAMPLING
/*unsigned char captouch_scanorder[] = { \
  0, 1, 2, 3 \
};*/

/*unsigned char captouch_scanorder[] = { \
  1, 1, 0, 0, 0, 3, 2, 0, 3, 3, 0, 2, 2, 1, 2, 2, \
  3, 0, 1, 0, 0, 0, 1, 1, 1, 2, 3, 1, 1, 3, 1, 2, \
  2, 3, 2, 3, 0, 3, 1, 3, 3, 3, 0, 1, 2, 2, 2, 2, \
  1, 0, 3, 1, 0, 3, 2, 2, 1, 1, 0, 2, 0, 0, 3, 3 \
};*/

unsigned char captouch_scanorder[] = { \
  1, 3, 3, 0, 1, 0, 1, 3, 0, 2, 3, 3, 1, 2, 2, 1, \
  1, 3, 3, 2, 3, 2, 1, 0, 3, 3, 2, 2, 2, 2, 3, 2, \
  2, 2, 0, 2, 2, 1, 0, 1, 0, 3, 0, 2, 3, 2, 2, 1, \
  1, 0, 0, 0, 1, 3, 2, 2, 3, 0, 2, 1, 3, 0, 3, 2, \
  3, 2, 3, 2, 1, 1, 1, 3, 2, 0, 1, 1, 2, 2, 3, 2, \
  1, 1, 0, 2, 0, 0, 2, 3, 1, 1, 3, 3, 3, 1, 1, 0, \
  0, 1, 1, 0, 0, 3, 3, 3, 0, 0, 0, 2, 0, 2, 3, 3, \
  1, 2, 3, 2, 1, 0, 3, 1, 2, 2, 0, 0, 2, 1, 1, 2, \
  0, 0, 1, 1, 3, 2, 0, 0, 1, 2, 0, 0, 0, 2, 2, 0, \
  3, 3, 3, 3, 3, 3, 2, 0, 3, 1, 2, 3, 1, 2, 0, 0, \
  2, 0, 1, 3, 0, 3, 3, 3, 1, 2, 1, 3, 2, 1, 0, 1, \
  2, 0, 1, 3, 0, 0, 3, 1, 0, 3, 1, 2, 1, 1, 0, 3, \
  1, 3, 1, 1, 0, 3, 3, 2, 0, 0, 1, 3, 2, 2, 1, 0, \
  0, 0, 3, 2, 3, 0, 3, 1, 3, 3, 1, 0, 3, 2, 1, 0, \
  0, 1, 2, 2, 1, 0, 0, 3, 1, 2, 1, 1, 1, 0, 2, 0, \
  1, 3, 2, 0, 2, 3, 2, 2, 1, 2, 0, 0, 1, 2, 3, 1 \
};

captouch_t captouch;

void captouch_disable_pullup(void)
{
  int n;
  
  analogReference(DEFAULT);

  n = 0;
  while (n < CAP_N_SENSORS) {
    // turn off pull up resistor on analog input
    digitalWrite(A0 + CAP_SENSORS(n), LOW);
    
    n = n + 1;
  }
}

void captouch_init(void)
{
  int n;

  n = 0;
  while (n < CAP_N_SENSORS) {
    // turn off pull up resistor on analog input
    pinMode(A0 + CAP_SENSORS(n), INPUT);
    
    captouch.readings[n] = 0;
    captouch.avgs[n] = 0;
    captouch.states[n] = cap_state_calibrating;
    captouch.counters[n] = 0;
    captouch.recal_counters[n] = 0;
    captouch.prev_keys_mask = 0;
    captouch.prev_keys_time = 0;
    
    n = n + 1;
  }
  captouch_disable_pullup();  

}

int captouch_ch_to_ref(int ch)
{
  int ref;
  
  if (ch == CAP_N_SENSORS - 1)
    ref = 0;
  else
    ref = ch + 1;

  return ref;
}

void captouch_set_adc_reference_pin(unsigned int n)
{
  unsigned char mux;
  
  if (n < 8) {
    mux = n;
  } else {
    mux = 0x20 + (n - 8);
  }
  
  ADMUX &= ~0x1F;
  ADMUX |= (mux & 0x1F);
  if (mux > 0x1F) {
    ADCSRB |= 0x08;
  } else {
    ADCSRB &= ~0x08;
  }
}

void captouch_process_readings(void)
{
  int n;
  
  for (n = 0; n < CAP_N_SENSORS; n++) {
    switch (captouch.states[n]) {
    case cap_state_calibrating:
      if (captouch.counters[n] < CAP_N_CALIBRATION_SAMPLES) {
        captouch.avgs[n] = ((float) (captouch.counters[n] * captouch.avgs[n] + \
            captouch.readings[n])) / (captouch.counters[n] + 1);
        //captouch.avgs_sq[n] = ((float) (captouch.counters[n] * captouch.avgs_sq[n] + captouch.readings[n] * captouch.readings[n])) / (captouch.counters[n] + 1);
        captouch.counters[n]++;
      }
      if (captouch.counters[n] == CAP_N_CALIBRATION_SAMPLES)
        captouch.states[n] = cap_state_released;
      break;
    case cap_state_released:
      if (captouch.readings[n] < captouch.avgs[n] - CAP_THRESHOLD_RELEASED_TO_PRESSED(n) ) {
        captouch.counters[n] = 1;
        captouch.states[n] = cap_state_released_to_pressed;
      } else {
        captouch.avgs[n] = ((float) ((CAP_N_FILTER_SAMPLES - 1) * captouch.avgs[n] + \
            captouch.readings[n])) / CAP_N_FILTER_SAMPLES;
        //captouch.avgs_sq[n] = ((float) (captouch.counters[n] * captouch.avgs_sq[n] + captouch.readings[n] * captouch.readings[n])) / (captouch.counters[n] + 1);
      }
      break;
    case cap_state_released_to_pressed:
      // do not update average in this state
      if (captouch.readings[n] >= captouch.avgs[n] - CAP_THRESHOLD_RELEASED_TO_PRESSED(n)) {
        // button is released; fall back to released
        captouch.counters[n] = 1;
        captouch.recal_counters[n] = 1;
        captouch.states[n] = cap_state_released;
      } else {
        if (captouch.counters[n] + 1 < CAP_HYSTERESIS_RELEASED_TO_PRESSED) {
          captouch.counters[n]++;
        } else {
          captouch.counters[n] = 1;
          captouch.states[n] = cap_state_pressed;
        }
      }
      break;
    case cap_state_pressed:
      if (captouch.readings[n] < captouch.avgs[n] - CAP_THRESHOLD_PRESSED_TO_RELEASED(n)) {
        // button is still pressed
        if (captouch.recal_counters[n] + 1 > CAP_N_RECALIBRATION_SAMPLES) {
           // button is held too long -> recalibrate
           captouch.counters[n] = 0;
           captouch.recal_counters[n] = 0;
           captouch.avgs[n] = 0;
           captouch.states[n] = cap_state_calibrating;
        } else {
          captouch.recal_counters[n]++;
        }
      } else {
        captouch.counters[n] = 1;
        captouch.states[n] = cap_state_pressed_to_released;
      }
      break;
    case cap_state_pressed_to_released:
      if (captouch.readings[n] < captouch.avgs[n] - CAP_THRESHOLD_PRESSED_TO_RELEASED(n)) {
        // button is still pressed
        captouch.states[n] = cap_state_pressed;
        captouch.recal_counters[n]++;
      } else {
        if (captouch.counters[n] + 1 < CAP_HYSTERESIS_PRESSED_TO_RELEASED) {
          captouch.counters[n]++;
        } else {
          captouch.counters[n] = 1;
          captouch.states[n] = cap_state_released;
        }
      }
      break;
    }
  }
}

void captouch_get_readings(void)
{
  int ch, ref, ch_pin, ref_pin, index, i;
  
  for (ch = 0; ch < CAP_N_SENSORS; ch++)
    captouch.readings[ch] = 0;
 
  for (index = 0; index < CAP_N_SENSORS * CAP_OVERSAMPLING; index++) {
    ch = captouch_scanorder[index];
    ref = captouch_ch_to_ref(ch);
    
    ch_pin = CAP_SENSORS(ch);
    ref_pin = CAP_SENSORS(ref);
    
    // set reference pin as output and high
    pinMode(A0 + ref_pin, OUTPUT);
    digitalWrite(A0 + ref_pin, HIGH);
	
    // set sensor pin as output and low (discharge sensor)
    pinMode(A0 + ch_pin, OUTPUT);
    digitalWrite(A0 + ch_pin, LOW);

    // set sensor pin as analog input
    pinMode(A0 + ch_pin, INPUT);
    
    for (i = 0; i < CAP_ANALOG_INTEGRATE; i++) {
      // set ADC to reference pin (charge internal capacitor)
      captouch_set_adc_reference_pin(ref_pin);
      // set ADC to sensor pin (transfer charge from Chold to Csense)
      captouch_set_adc_reference_pin(ch_pin);
    }
    
    // set ADC to reference pin (charge internal capacitor)
    captouch_set_adc_reference_pin(ref_pin);
    
    // read sensor
    
    #ifndef CS_ENABLE_SLEW_RATE_LIMITER
    captouch.readings[ch] += analogRead(ch_pin);
    #else
    if (captouch.readings[ch] == 0)
      captouch.readings[ch] = analogRead(ch_pin);
    else
      captouch.readings[ch] += (analogRead(ch_pin) > captouch.readings[ch]) ? 1: -1;
    #endif  

    // set sensor pin as output and low (discharge sensor)
    pinMode(A0 + ch_pin, OUTPUT);
    digitalWrite(A0 + ch_pin, LOW);
    
    pinMode(A0 + ref_pin, INPUT);
  }
}

void captouch_serial_write(char *s)
{
  unsigned int n, N;
  
  N = strlen(s);
  
  for (n = 0; n < N; n++)
    Serial.print(s[n]);
}

void captouch_print_debug(void)
{
  int n, buf_index;
  char buf[CAP_BUF_LENGTH];
  
  n = 0;
  buf_index = 0;
  while (n < CAP_N_SENSORS) {
    buf_index += snprintf(&(buf[buf_index]), CAP_BUF_LENGTH - buf_index, \
       "   raw: % 5u; avg: % 5u; delta: % 5d; state: %1d", \
       captouch.readings[n], (int) captouch.avgs[n], ((int) captouch.avgs[n]) \
       - captouch.readings[n], captouch.states[n]);
    n = n + 1;
  }
  // new line
  //Serial.println();
  snprintf(&(buf[buf_index]), CAP_BUF_LENGTH - buf_index, "\t");
  captouch_serial_write(buf);
}

void captouch_process_bar(int n)
{
  int K;
  float f;
  
  f = 100 * log((float) (captouch.avgs[n] - captouch.readings[n]));
  
//  Serial.println(f);
  
  if (f < CAP_BAR_MIN)
    K = 0;
  else if (f > CAP_BAR_MAX)
    K = CAP_BAR_LENGTH;
  else
    K = (int) ((f - CAP_BAR_MIN) * (CAP_BAR_LENGTH) / (CAP_BAR_MAX - CAP_BAR_MIN));

  captouch.bar[n] = K;
}

void captouch_print_bar(int n)
{
  int k, buf_index;
  char buf[CAP_BUF_LENGTH];
  
  buf_index = 0;
 
  captouch_process_bar(n);
  
  for (k = 0; k < captouch.bar[n]; k++)
    buf[buf_index++] = '=';
  
  for (; k < CAP_BAR_LENGTH; k++)
    buf[buf_index++] = '.';
  
  buf[buf_index++] = '\0';
  
  captouch_serial_write(buf);
}
