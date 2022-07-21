
/* CHRYSALIS RECEIVER PROGRAM */

#include <Arduino.h>
#include <SPI.h>
#include <Servo.h>
#include <nRF24L01.h>
#include <RF24.h>

#define X A0            // RF24
#define Y A1            // RF24
#define Z 10            // RF24

Servo LEFT_SERVO;
Servo RIGHT_SERVO;

// 2.4 GHz radio
RF24 Radio(5,6); // CE, CSN
const byte address[6] = "37412";

// timekeeping
unsigned long time_curr = 0;
unsigned long time_prev = 0;
const int FLAP_DELAY    = 110;
const int DROP_DELAY    = 200;
const int SWITCH_DELAY  = 500;

// analog readings
int xyz_val[3] = {0,0,0};
int x_val = 0;
int y_val = 0;
int z_val = 0;
unsigned long z_vals[2][2] = {{0,0}, 
                              {0,0}};
bool is_on = false;

// analog thresholds
const int LEFT_THRES_ANLG  = 400;
const int DOWN_THRES_ANLG  = 400;
const int RIGHT_THRES_ANLG = 1022 - LEFT_THRES_ANLG;
const int UP_THRES_ANLG    = 1022 - DOWN_THRES_ANLG;

////////////////////////////////////////////////////////////////////

void setup() {

    Serial.begin(9600);
    LEFT_SERVO.attach(3);
    RIGHT_SERVO.attach(4);
    LEFT_SERVO.write(get_angle(0));
    RIGHT_SERVO.write(get_angle(0));

    Radio.begin();
    Radio.openReadingPipe(0,address);
    Radio.setPALevel(RF24_PA_MAX); // max transceiving distance
    Radio.startListening();

}

void loop() {
    
    time_curr = millis();
    while (is_on) {

        if (Radio.available()) {
            char msg_in[32] = "";
            Radio.read(&msg_in, sizeof(msg_in));
            Radio.read(&xyz_val, sizeof(&xyz_val)); // ?

            x_val = (int)xyz_val[0];
            y_val = (int)xyz_val[1];
            z_val = (int)xyz_val[2];

            if (z_val == 1) {
                for (byte c = 0; c <= 1; ++c) {
                    if (z_vals[0][c] == 0) {
                        z_vals[0][c] = z_val;
                    }
                    if (z_vals[1][c] == 0) {
                        z_vals[1][c] = millis();    
                    }
                    if (abs(z_vals[1][0] - z_vals[1][1]) <= 10) { // 10 ms minimum delay
                        z_vals[0][1] = 0;
                        z_vals[1][1] = 0;
                    }
                }
            }
            if (is_dbl_pressed(10, SWITCH_DELAY) || 
                abs(z_vals[1][0] - z_vals[1][1]) > SWITCH_DELAY) {
                is_on = !is_on;
                for (byte r = 0; r <= 1; ++r) {
                    for (byte c = 0; c <= 1; ++c) {
                        z_vals[r][d] = 0;
                    }
                }
            }
        }

        // x_val = analogRead(X); // joystick X
        // y_val = analogRead(Y); // joystick Y

        int x_uint8 = map(x_val, 0, 1023, 0, 255); // maps 0-1023 to 0-255
        int y_uint8 = map(y_val, 0, 1023, 0, 255);

        // stationary or forward
        LEFT_SERVO.write(get_angle(30));
        RIGHT_SERVO.write(get_angle(30));
        LEFT_SERVO.write(get_angle(-30));
        RIGHT_SERVO.write(get_angle(-30));

        // turn left
        while (x_val < LEFT_THRES_ANLG && y_val > UP_THRES_ANLG) {
            for (int i = 0; i < 3; i++) {
                LEFT_SERVO.write(get_angle(20));
                RIGHT_SERVO.write(get_angle(40));
                LEFT_SERVO.write(get_angle(-40));
                RIGHT_SERVO.write(get_angle(-20));
            }
        }

        // turn right
        while (x_val > RIGHT_THRES_ANLG && y_val > UP_THRES_ANLG) {
            for (int i = 0; i < 3; i++) {
                LEFT_SERVO.write(get_angle(40));
                RIGHT_SERVO.write(get_angle(20));
                LEFT_SERVO.write(get_angle(-20));
                RIGHT_SERVO.write(get_angle(-40));
            }
        }   

        // drop altitude
        while (y_val < DOWN_THRES_ANLG) {
            for (int i = 0; i < 3; i++) {
                if (time_curr - time_prev >= DROP_DELAY) {
                    LEFT_SERVO.write(get_angle(30));
                    RIGHT_SERVO.write(get_angle(30));
                    time_prev = time_curr;
                }
                if (time_curr - time_prev >= DROP_DELAY) {
                    LEFT_SERVO.write(get_angle(-30));
                    RIGHT_SERVO.write(get_angle(-30));
                    time_prev = time_curr;
                }
            }
        }
    } /* while (is_on) {} */
}

int get_angle (int displacement) {
    if (displacement < -90) {
        displacement = -90;
    }
    else if (displacement > 90) {
        displacement = 90;
    }

    return 90 + displacement;
}

bool is_dbl_pressed (int min_thres, int max_thres) {
    
    for (byte c = 0; c < sizeof(z_vals[0]); ++c) {
        if (z_vals[0][c] == 0) {
            return false;
        }
    }
    if (abs(z_vals[1][0] - z_vals[1][1]) > max_thres || 
        abs(z_vals[1][0] - z_vals[1][1]) < min_thres) {
        return false;
    }

    return true;
}

void servo_transmit(Servo WING, int angle, int delay) {
    if (time_curr - time_prev >= delay) {
    WING.write(get_angle(angle));
    }
    time_prev = time_curr;
}

////////////////////////////////////////////////////////////////////

/* CURRENTLY UNNEEDED */

/*
int get_uint8 (float degrees) {
    if (degrees > 90.0) {degrees = 90.0;}
    else if (degrees < -90.0) {degrees = -90.0;}

    int uint8_val = degrees/180.0 * 256.0 + 127.0;

    if (uint8_val > 255) {uint8_val = 255;}
    else if (uint8_val < 0) {uint8_val = 0;}

    return uint8_val;
}

int get_thres_deg (int thres_anlg) {
    if (thres_anlg < 0) {thres_anlg = 0;}
    else if (thres_anlg > 1023) {thres_anlg = 1023;}

    return (int)((511 - thres_anlg) / 511 * 180);
}
*/