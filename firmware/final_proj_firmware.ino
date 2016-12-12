// Analog switch control pins (active-HIGH)
const byte pin_switch_en_a = 5; // D5: Connects integrator input to input current
const byte pin_switch_en_b = 12; // D12: Connects integrator input to reference current
const byte pin_switch_en_c = 9; // D9: Resets integrator capacitor
// Comparator input pin
const byte pin_comp_in = 2; // D2: INT0
// Start-up pin
const byte pin_start = 7; //D7: Connects to a node in the PTAT via a cap
const uint16_t ramp_up_delta_t_us = 5000;
unsigned long ramp_down_start_time_us = 0;
unsigned long ramp_down_end_time_us = 0;
unsigned long ramp_down_delta_t_us = 0;
// Temperature calibration for given t_down/t_up
// T = a*(t_down/t_up) - b
const float a = 185.2;
const float b = - 0.8687;

void setup() {
// Configure IO pins
pinMode( pin_switch_en_a, OUTPUT ); // Switch A enable
pinMode( pin_switch_en_b, OUTPUT ); // Switch B enable
pinMode( pin_switch_en_c, OUTPUT ); // Switch C enable
pinMode( pin_start, OUTPUT ); // Start up pin
pinMode( pin_comp_in, INPUT );   // Output from comparator
// Configure an interrupt to call isr_comp_in() on a rising edge at pin_comp_in
attachInterrupt( digitalPinToInterrupt( pin_comp_in ), isr_comp_in, RISING );
// Configure the USB serial port for reporting conversion results at 9600 baud
Serial.begin( 9600 );
// Ramp the integrator all the way up so that the ADC can start
// even if the integrator initial condition is negative
digitalWrite( pin_start, HIGH ); // Pulse start up pin high
digitalWrite( pin_start, LOW );
digitalWrite( pin_switch_en_c, HIGH ); // drain integrator capacitor
delay( 1 );
digitalWrite( pin_switch_en_c, LOW );
digitalWrite( pin_switch_en_b, LOW );
digitalWrite( pin_switch_en_a, HIGH );
delayMicroseconds( ramp_up_delta_t_us );
// Now start the first ramp-down
digitalWrite( pin_switch_en_a, LOW );
digitalWrite( pin_switch_en_b, HIGH );
}

void loop() {
// Sample most-recent ramp times while preventing the comparator interrupt from firing
cli(); // Disable interrupts
unsigned long ramp_down_delta_t_us_sample = ramp_down_delta_t_us;
unsigned long ramp_down_start_time_us_sample = ramp_down_start_time_us;
unsigned long ramp_down_end_time_us_sample = ramp_down_end_time_us;
sei(); // Enable interrupts
// Print the latest result roughly once per second
Serial.print( "Ramp-down delta t (us): " );
Serial.println( ramp_down_delta_t_us_sample );
Serial.print( "Ramp down start: " );
Serial.println( ramp_down_start_time_us_sample );
Serial.print( "Ramp down end: " );
Serial.println( ramp_down_end_time_us_sample );
Serial.print( "Temperature: " );
Serial.println( calculate_temperature( ramp_down_delta_t_us_sample ), 4 );
delay( 1000 ); // Do nothing for 1000 milliseconds
}
// Comparator interrupt:
//   Fires whenever the comparator output transistions from LOW to HIGH.
//   This occurs when the integrator ramp-down crosses the reference voltage.
void isr_comp_in() {
cli(); // Disable interrupts
// Stop the integrator ramp-down
digitalWrite( pin_switch_en_b, LOW );
digitalWrite( pin_switch_en_c, HIGH ); // drain integrator capacitor
delay( 1 );
digitalWrite( pin_switch_en_c, LOW );

// Figure out the ramp-down time
ramp_down_end_time_us = micros();
ramp_down_delta_t_us = ramp_down_end_time_us - ramp_down_start_time_us;
// Start the integrator ramp-up
digitalWrite( pin_switch_en_a, HIGH );
// Integrate the unknown current for a known amount of time
delayMicroseconds( ramp_up_delta_t_us );
// Stop the integrator ramp-up
digitalWrite( pin_switch_en_a, LOW );
// Start the integrator ramp-down;
digitalWrite( pin_switch_en_b, HIGH );
ramp_down_start_time_us = micros();
EIFR = 0x01; // Clear any pending interrupts on INT0 (useful when you implement en_c)
sei(); // Enable interrupts
}
// Uses the calibration curve to calculate the temperature
float calculate_temperature( unsigned long ramp_down_delta_t_us ) {
return (ramp_down_delta_t_us/float(ramp_up_delta_t_us) + b)* a;
}
