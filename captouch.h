/* Copyright (c) 2015, Admar Schoonen
 * All rights reserved.
 *
 * License: New BSD License. See also license.txt
 */

#ifndef _CAPTOUCH_H_
#define _CAPTOUCH_H_

//#include "configuration.h"

#define CAP_SENSOR_0 0
#define CAP_SENSOR_1 1
#define CAP_SENSOR_2 2
#define CAP_SENSOR_3 3
#define CAP_SENSORS(x) (x == 0 ? CAP_SENSOR_0 : (x == 1 ? CAP_SENSOR_1 : (x == 2 ? CAP_SENSOR_2 : CAP_SENSOR_3)))

#define CAP_N_SENSORS 4

#define CAP_ANALOG_INTEGRATE 2
//#define CAP_OVERSAMPLING 16
#define CAP_OVERSAMPLING 64
//#define CAP_THRESHOLD_RELEASED_TO_PRESSED 75
//#define CAP_THRESHOLD_PRESSED_TO_RELEASED 75
/*#define CAP_THRESHOLD_RELEASED_TO_PRESSED 4
#define CAP_THRESHOLD_PRESSED_TO_RELEASED 4*/
#define CAP_THRESHOLD_RELEASED_TO_PRESSED(x) (x == 0 ? 500 : (x == 1 ? 500 : (x == 2 ? 500 : 500)))
#define CAP_THRESHOLD_PRESSED_TO_RELEASED(x) (x == 0 ? 500 : (x == 1 ? 500 : (x == 2 ? 500 : 500)))
//#define CAP_THRESHOLD_RELEASED_TO_PRESSED(x) (x == 0 ? 100 : (x == 1 ? 100 : (x == 2 ? 100 : 100)))
//#define CAP_THRESHOLD_PRESSED_TO_RELEASED(x) (x == 0 ? 100 : (x == 1 ? 100 : (x == 2 ? 100 : 100)))

#define CAP_HYSTERESIS_RELEASED_TO_PRESSED 4
#define CAP_HYSTERESIS_PRESSED_TO_RELEASED 4
#define CAP_N_CALIBRATION_SAMPLES 16
#define CAP_N_FILTER_SAMPLES 4096 //1024
#define CAP_N_RECALIBRATION_SAMPLES 10000
#define CAP_TIME_WAIT_AFTER_KEY_RELEASE 500 /* milliseconds */
//#define CS_ENABLE_SLEW_RATE_LIMITER

//#define CAP_REFERENCE DEFAULT
#define CAP_REFERENCE INTERNAL

typedef enum cap_button_state_t {
  cap_state_calibrating,
  cap_state_released,
  cap_state_released_to_pressed,
  cap_state_pressed,
  cap_state_pressed_to_released
} button_state_t;

const char * const state_labels[] = {"calibrating", "released", "released_to_pressed", "pressed", "pressed_to_released"};

typedef struct captouch_t {
  unsigned int readings[CAP_N_SENSORS];
  float avgs[CAP_N_SENSORS];
  button_state_t states[CAP_N_SENSORS];
  unsigned int counters[CAP_N_SENSORS];
  unsigned int recal_counters[CAP_N_SENSORS];
  char prev_keys_mask;
  unsigned long prev_keys_time;
  int bar[CAP_N_SENSORS];
} captouch_t;

extern captouch_t captouch;

extern void captouch_disable_pullup();
extern void captouch_init(void);
extern void captouch_get_readings(void);
extern void captouch_process_readings(void);
extern void captouch_print_debug(void);
extern void captouch_process_bar(int n);
extern void captouch_print_bar(int n);
#endif // _CAPTOUCH_H_
