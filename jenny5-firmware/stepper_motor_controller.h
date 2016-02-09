#ifndef stepper_motor_controller_H
#define stepper_motor_controller_H

#include "Arduino.h"
#include "AccelStepper.h"
#include "potentiometers_controller.h"
#include "jenny5_types.h"

//---------------------------------------------------
class t_stepper_motor_controller
{
public:
  //Declare pin functions on Arduino
  
  byte dir_pin;
  byte step_pin;
  byte enable_pin;
  
  t_sensor_info  *sensors;
  byte sensors_count;
  
  AccelStepper  *stepper;
  byte motor_running;

  //int motor_speed;
  //int motor_acceleration;
  
public:
  t_stepper_motor_controller(void);
  ~t_stepper_motor_controller(void);
  
  void create_init(byte _dir, byte _step, byte _enable, float default_motor_speed, float default_motor_acceleration);
  
  void move_motor(int num_steps);
  void move_motor_to(int _position);
  
  void set_motor_speed(float motor_speed);
  
  void set_motor_acceleration(float motor_acceleration);
// speed and acceleration are float, but we read and write them as int in order to reduce serial traffic
  void set_motor_speed_and_acceleration(float motor_speed, float motor_acceleration);
  void get_motor_speed_and_acceleration(float *motor_speed, float *motor_acceleration);
  
  void disable_motor(void);
  
  void lock_motor(void);

  void set_num_attached_sensors(byte num_sensors);
  void get_num_attached_sensors(byte *num_sensors);

  void add_sensor(byte sensor_type, byte sensor_index);

  void get_sensor(byte sensor_index_in_motor_list, byte *sensor_type, byte *sensor_index);

  void set_motor_running(byte is_running);
  void go_home(t_potentiometers_controller *potentiometers_control);
  byte is_motor_running(void);

  void reset_pins(void);
  
  int run_motor(t_potentiometers_controller *potentiometers_control);
};
//---------------------------------------------------

#endif
