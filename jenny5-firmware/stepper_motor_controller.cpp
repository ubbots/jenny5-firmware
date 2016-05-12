#include "stepper_motor_controller.h"
#include "jenny5_types.h"

//-------------------------------------------------------------------------------
t_stepper_motor_controller::t_stepper_motor_controller(void)
{
  dir_pin = 2;
  step_pin = 3;
  enable_pin = 4;

  sensors_count = 0;
  sensors = NULL;

  motor_running = 0;
  stepper = NULL;

  going_home = false;
}
//-------------------------------------------------------------------------------
t_stepper_motor_controller::~t_stepper_motor_controller(void)
{
  if (stepper)
    delete stepper;

  if (sensors)
    delete[] sensors;

  sensors_count = 0;

  motor_running = 0;
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::create_init(byte _dir, byte _step, byte _enable, float default_motor_speed, float default_motor_acceleration)
{
  dir_pin = _dir;
  step_pin = _step;
  enable_pin = _enable;

  if (stepper)
    delete stepper;

  if (sensors)
    delete[] sensors;

  sensors_count = 0;

  motor_running = 0;
  going_home = false;

  stepper = new AccelStepper(AccelStepper::DRIVER, step_pin, dir_pin);
  stepper->setMaxSpeed(default_motor_speed);
  stepper->setSpeed(default_motor_speed);
  stepper->setAcceleration(default_motor_acceleration);

  reset_pins();
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::move_motor(int num_steps)
{
  digitalWrite(enable_pin, LOW); // turn motor on
  stepper->move(num_steps); //move num_steps
  motor_running = 1;
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::move_motor_to(int _position)
{
  digitalWrite(enable_pin, LOW); // turn motor on
  stepper->moveTo(_position); //move num_steps
  motor_running = 1;
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::set_motor_speed(float _motor_speed)
{
  stepper->setMaxSpeed(_motor_speed);
  stepper->setSpeed(_motor_speed);
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::set_motor_acceleration(float _motor_acceleration)
{
  stepper->setAcceleration(_motor_acceleration);
  // motor_acceleration = _motor_acceleration;
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::set_motor_speed_and_acceleration(float _motor_speed, float _motor_acceleration)
{
  stepper->setMaxSpeed(_motor_speed);
  stepper->setSpeed(_motor_speed);
  stepper->setAcceleration(_motor_acceleration);
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::disable_motor(void)
{
  digitalWrite(enable_pin, HIGH); // disable motor
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::lock_motor(void)
{
  digitalWrite(enable_pin, LOW); // enable motor
}
//-------------------------------------------------------------------------------
//Reset pins to default states
void t_stepper_motor_controller::reset_pins(void)
{
  digitalWrite(step_pin, LOW);
  digitalWrite(dir_pin, LOW);
  digitalWrite(enable_pin, HIGH); // all motors are disabled now
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::add_sensor(byte sensor_type, byte sensor_index)
{
  sensors[sensors_count].type = sensor_type;
  sensors[sensors_count].index = sensor_index;
  sensors_count++;
}
//-------------------------------------------------------------------------------
byte t_stepper_motor_controller::get_num_attached_sensors(void)
{
  return sensors_count;
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::set_num_attached_sensors(byte num_sensors)
{
  if (sensors_count != num_sensors) {
    // clear memory if the array has a different size
    if (sensors) {
      delete[] sensors;
      sensors = NULL;
    }

    if (num_sensors > 0)
      sensors = new t_sensor_info[num_sensors]; // allocate memory for them
  }
  sensors_count = 0; // actual number of sensors
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::set_motor_running(byte is_running)
{
  motor_running = is_running;
}
//-------------------------------------------------------------------------------
byte t_stepper_motor_controller::is_motor_running(void)
{
  return motor_running;
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::get_sensor(byte sensor_index_in_motor_list, byte *sensor_type, byte *sensor_index)
{
  *sensor_type = sensors[sensor_index_in_motor_list].type;
  *sensor_index = sensors[sensor_index_in_motor_list].index;
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::get_motor_speed_and_acceleration(float *_motor_speed, float *_motor_acceleration)
{
  if (stepper) {
    *_motor_acceleration = stepper->getAcceleration();
    *_motor_speed = stepper->maxSpeed();
  }
  else {
    *_motor_speed = 0;
    *_motor_acceleration = 0;
  }
}
//-------------------------------------------------------------------------------
byte t_stepper_motor_controller::run_motor(t_potentiometers_controller *potentiometers_control, t_infrared_sensors_controller *infrared_controller, int& dist_to_go)
{
  // returns 1 if is still running
  // returns 2 if it does nothing
  // returns 0 if it has just stopped; in dist_to_go we have what is left to run (or 0)

  bool limit_reached = false;
  int distance_to_go = stepper->distanceToGo();

  if (distance_to_go)
  {
    for (byte j = 0 ; j < sensors_count ; ++j)
    {
      byte sensor_index = sensors[j].index;
      byte type = sensors[j].type;

      if (POTENTIOMETER == type) {
        int potentiometer_direction = potentiometers_control->get_direction(sensor_index);
        if (potentiometers_control->is_lower_bound_reached(sensor_index)) {
          if (distance_to_go * potentiometer_direction < 0) {
            limit_reached = true;
          }
        }
        if (potentiometers_control->is_upper_bound_reached(sensor_index)) {
          if (distance_to_go * potentiometer_direction > 0) {
            limit_reached = true;
          }
        }
      }
      else if (ULTRASOUND == type) {
        // deal with ultrasound sensor
      }
      else if (INFRARED == type) {
        int infrared_direction = infrared_controller->get_direction(sensor_index);
        int infrared_signal_strength = infrared_controller->get_signal_strength(sensor_index);
        int infrared_home_pos = infrared_controller->get_home_position(sensor_index);

        /*
          Serial.print(distance_to_go);
          Serial.write(' ');
          Serial.print(infrared_signal_strength);
          Serial.write(' ');
          Serial.println(infrared_home_pos);
        */
        if (infrared_controller->is_lower_bound_reached(sensor_index)) {
          if (distance_to_go * infrared_direction < 0) {
            limit_reached = true;
          }
        }
        if (infrared_controller->is_upper_bound_reached(sensor_index)) {
          if (distance_to_go * infrared_direction > 0) {
            limit_reached = true;
          }
        }
        if (going_home) {
          // must stop to home
          if (distance_to_go > 0)
            if (infrared_signal_strength > infrared_home_pos)
              limit_reached = true;
            else;
          else if (infrared_signal_strength < infrared_home_pos)
            limit_reached = true;
        }
      }

    }

    if (!limit_reached)
    {
      stepper->run();
      return MOTOR_STILL_RUNNING; // still running
    }
    else {
      if (is_motor_running()) {
        int to_go = stepper->distanceToGo();
        stepper->setCurrentPosition(0);
        stepper->move(0);
        dist_to_go = to_go;
        going_home = false;
        //Serial.println("left to go  > 0");
        set_motor_running(0);
        return MOTOR_JUST_STOPPED;
      }
    }
  }
  else {
    // the motor has just finished the move, so we output that event
    going_home = false;
    if (is_motor_running()) {
      set_motor_running(0);
      dist_to_go = 0;
      //Serial.println("left to go == 0");
      return MOTOR_JUST_STOPPED; // distance to go
    }
    else
      return MOTOR_DOES_NOTHING;
  }
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::go_home(t_potentiometers_controller *potentiometers_control)
{
  int sensor_index = -1;
  for (byte j = 0 ; j < sensors_count ; ++j) {
    byte type = sensors[j].type;
    if (POTENTIOMETER == type) {
      sensor_index = sensors[j].index;
      break;
    }
  }
  if (sensor_index != -1) {
    //calculate the remaining distance from the current position to home position, relative to the direction and position of the potentiometer
    int pot_dir = potentiometers_control->get_direction(sensor_index);
    int pot_home = potentiometers_control->get_home_position(sensor_index);
    int pot_pos = potentiometers_control->get_position(sensor_index);
    int distance_to_home = pot_dir * (pot_home - pot_pos);
    move_motor(distance_to_home);
  }
}
//-------------------------------------------------------------------------------
void t_stepper_motor_controller::go_home(t_infrared_sensors_controller *infrareds_control)
{
  int sensor_index = -1;
  for (byte j = 0 ; j < sensors_count ; ++j) {
    byte type = sensors[j].type;
    if (INFRARED == type) {
      sensor_index = sensors[j].index;
      break;
    }
  }
  if (sensor_index != -1) {
    //calculate the remaining distance from the current position to home position, relative to the direction and position of the potentiometer
    //int i_dir = infrareds_control->get_direction(sensor_index);
    int i_home = infrareds_control->get_home_position(sensor_index);
    int i_pos = infrareds_control->get_signal_strength(sensor_index);
    int distance_to_home;
    if (i_home < i_pos)
      distance_to_home = -32000;
    else
      distance_to_home = 32000;
    going_home = true;
    move_motor(distance_to_home);
  }
}
//-------------------------------------------------------------------------------

