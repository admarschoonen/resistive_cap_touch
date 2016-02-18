/* Copyright (c) 2015, Admar Schoonen
 * All rights reserved.
 *
 * License: New BSD License. See also license.txt
 */

#ifndef _RESTOUCH_H_
#define _RESTOUCH_H_

#define RES_SENSOR_0 0
#define RES_SENSOR_1 1
#define RES_SENSOR_2 2
#define RES_SENSOR_3 3
#define RES_GND_0 2
#define RES_GND_1 3
#define RES_GND_2 4
#define RES_GND_3 5
#define RES_VCC_0 6
#define RES_VCC_1 7
#define RES_VCC_2 8
#define RES_VCC_3 9

#define RES_SENSORS(x) (x == 0 ? RES_SENSOR_0 : (x == 1 ? RES_SENSOR_1 : (x == 2 ? RES_SENSOR_2 : RES_SENSOR_3)))
#define RES_GNDS(x) (x == 0 ? RES_GND_0 : (x == 1 ? RES_GND_1 : (x == 2 ? RES_GND_2 : RES_GND_3)))
#define RES_VCCS(x) (x == 0 ? RES_VCC_0 : (x == 1 ? RES_VCC_1 : (x == 2 ? RES_VCC_2 : RES_VCC_3)))

#define RES_N_SENSORS 4

#define RES_OVERSAMPLING 64
//#define RES_OVERSAMPLING 1
#define RES_THRESHOLD_RELEASED_TO_PRESSED(x) (x == 0 ? 1000 : (x == 1 ? 1000 : (x == 2 ? 1000 : 1000)))
#define RES_THRESHOLD_PRESSED_TO_RELEASED(x) (x == 0 ? 1000 : (x == 1 ? 1000 : (x == 2 ? 1000 : 1000)))
//#define RES_THRESHOLD_RELEASED_TO_PRESSED(x) (x == 0 ? 100 : (x == 1 ? 100 : (x == 2 ? 100 : 100)))
//#define RES_THRESHOLD_PRESSED_TO_RELEASED(x) (x == 0 ? 100 : (x == 1 ? 100 : (x == 2 ? 100 : 100)))

#define RES_HYSTERESIS_RELEASED_TO_PRESSED 4
#define RES_HYSTERESIS_PRESSED_TO_RELEASED 4
#define RES_N_CALIBRATION_SAMPLES 16
#define RES_N_FILTER_SAMPLES 256
#define RES_N_RECALIBRATION_SAMPLES 10000
#define RES_TIME_WAIT_AFTER_KEY_RELEASE 500 /* milliseconds */


typedef enum res_button_state_t {
  res_state_calibrating,
  res_state_released,
  res_state_released_to_pressed,
  res_state_pressed,
  res_state_pressed_to_released
} res_button_state_t;

typedef struct restouch_t {
  unsigned int readings[RES_N_SENSORS];
  float avgs[RES_N_SENSORS];
  res_button_state_t states[RES_N_SENSORS];
  unsigned int counters[RES_N_SENSORS];
  unsigned int recal_counters[RES_N_SENSORS];
  char prev_keys_mask;
  unsigned long prev_keys_time;
  int bar[RES_N_SENSORS];
} restouch_t;

extern restouch_t restouch;

extern void restouch_vcc(int enable);
extern void restouch_gnd(int enable);
extern void restouch_init(void);
extern void restouch_get_readings(void);
extern void restouch_process_readings(void);
extern void restouch_print_debug(void);
extern void restouch_process_bar(int n);
extern void restouch_print_bar(int n);

#endif

