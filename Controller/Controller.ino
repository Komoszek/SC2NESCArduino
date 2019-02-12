//mapper.py - kliknięcia

byte MODE = 0;

/*
  0 - Setup modec
  1 - Reconnect mode
  2 - Discovery mode
  3 - Connecting/setup
  4 - Connected

  16 - fail mode
*/


#include "Wire.h" // This seems redundant, but we need to declare this
// dependency in the pde file or else it won't be included
// in the build.

#include "wiimote.h"
#include <math.h>

#include "scbt_hm10.h"



SCByBtC SCData;

char buffer[64];
int buffer_iterator = 0;

byte DEVICE_ID = 255;
int TEMP_ID = -1;
bool CONTROLLER_CANDIDATE = false;

int STEP = 0;

const int ZONE_CONST = 10922;

char CONNECTION_COMMAND[] = "AT+CONN0";
const char DISABLE_LIZARD_MODE[] = "AT+SEND_DATAWR002C\xc0\x87\x0f\x18\x00\x00\x31\x02\x00\x08\x07\x00\x07\x07\x00\x30\x00\x00x00";


int bdl = 0; // D-Pad Left state
int bdr = 0; // D-Pad Right state
int bdu = 0; // D-Pad Up state
int bdd = 0; // D-Pad Down state
int ba = 0; // A button state
int bb = 0; // B button state
int bx = 0; // X button state
int by = 0; // Y button state
int bl = 0; // L button state
int br = 0; // R button state
int bm = 0; // MINUS button state
int bp = 0; // PLUS button state
int bhome = 0; // HOME button state
int bzl = 0; // ZL button state
int bzr = 0; // ZR button state


/*
   Analog Buttons.
   They are initialized with center values from the calibration buffer.
*/

byte lx = 0x7F;
byte ly = 0x7F;
byte rx = 0x7F;
byte ry = 0x7F;

/*byte lx = calbuf[2] >> 2;
byte ly = calbuf[5] >> 2;
byte rx = calbuf[8] >> 3;
byte ry = calbuf[11] >> 3;
*/
byte lt = 0;
byte rt = 0;

int pinRight = 2;

// Wiimote button data stream
byte *stream_callback(byte *buffer) {

  wiimote_write_buffer(buffer, bdl, bdr, bdu, bdd, ba, bb, bx, by, bl, br,
                       bm, bp, bhome, lx, ly, rx, ry, bzl, bzr, lt, rt);

  return buffer;
}

int sign;

int alfa(int x, int y) {
  if (x > ZONE_CONST) {

    if (y > ZONE_CONST) {
      return 2;
    } else if (y < -ZONE_CONST) {
      return 8;
    } else {
      return 1;
    }

  } else if (x < -ZONE_CONST) {
    if (y > ZONE_CONST) {
      return 4;
    } else if (y < -ZONE_CONST) {
      return 6;
    } else {
      return 5;
    }
  } else {

    if (y > ZONE_CONST) {
      return 3;
    } else if (y < -ZONE_CONST) {
      return 7;
    } else {
      return 0;
    }
  }
}

void setup() {
  SCData.it = 0;
  wiimote_stream = stream_callback;
  wiimote_init();


  Serial.begin(115200); //This pipes to the serial monitor
  while (!Serial);
  Serial.setTimeout(10);
  Serial1.setTimeout(10);
  Serial1.begin(115200); //This pipes to the serial monitor


  delay(1000);
  Serial1.write("AT");
  delay(10);
  if (Serial1.available()) {

    buffer_iterator = Serial1.readBytes(buffer, 2);

    if (strncmp(buffer, "OK", 2) != 0)
      MODE = 16;

    if (MODE != 16) {
      Serial1.write("AT+ROLE1");
      delay(10);
      Serial1.write("AT+IMME1");
      delay(10);
      Serial1.write("AT+SHOW1");
      delay(10);

      while(Serial1.available())
        Serial1.read();
    }

  } else {
    MODE = 16;
  }

}

void loop() {
  if (MODE == 0) {
    delay(1000);
    Serial1.write("AT+CONNL");
    MODE = 1;
  }
  if (Serial.available()) {
    buffer_iterator = Serial.readBytes(buffer, 64);

    Serial1.write(buffer, buffer_iterator);

    if (buffer_iterator == 8) { //AT+DISC?
      if (strncmp(buffer, "AT+DISC?", 8) == 0) {
        Serial.write("DISCOVER\n");
        DEVICE_ID = 255;
        TEMP_ID = -1;
        MODE = 2;
      }

    }
    buffer_iterator = 0;
  }

  if (Serial1.available()) {
    switch (MODE) {
      case 1:
        SCData.it = Serial1.readBytesUntil('+', SCData.buffer, 64);

        if (SCData.it == 7) { //CONNE\r\n AND  CONNF\r\n
          if (strncmp(SCData.buffer, "CONNE\r\n", 7) == 0 || strncmp(SCData.buffer, "CONNF\r\n", 7) == 0) {
            Serial.write("CONNECTION FAILED - RECONNECTING\n");
            delay(10);
            Serial1.write("AT+CONNL");
          } else if (strncmp(SCData.buffer, "CONNN\r\n", 7) == 0) {
            Serial.write("NO MAC AVAILABLE - ENTERING DISCOVERY MODE\n");
            Serial1.write("AT+DISC?");
            MODE = 2;

          }

          SCData.it = 0;

        } else if (SCData.it == 6) {
          if (strncmp(SCData.buffer, "CONN\r\n", 6) == 0) {
            Serial.write("CONNECTION SUCCESS\n");

            Serial1.write(DISABLE_LIZARD_MODE, 37);
            delay(100);
            Serial1.write("AT+NOTIFY_ON0029");
            MODE = 4;
            SCData.it = 0;
          }
        }
        break;
      case 2:
        SCData.it = Serial1.readBytesUntil('+', SCData.buffer, 64);

        if (SCData.it > 5) {
          if (DEVICE_ID == 255 && SCData.buffer[4] == ':') {
            if (strncmp(SCData.buffer, "DIS", 3) == 0) {
              TEMP_ID++;
              if (SCData.buffer[5] == 'C' && SCData.buffer[6] == 'F')
                CONTROLLER_CANDIDATE = true;
              else
                CONTROLLER_CANDIDATE = false;

            } else if (strncmp(SCData.buffer, "NAME", 4) == 0 && CONTROLLER_CANDIDATE) {
              if (SCData.it == 22 && strncmp(SCData.buffer + 5, "SteamController\r\n", 17) == 0)
                DEVICE_ID = TEMP_ID;
            }
          }
        } else if (SCData.it == 5) { //CONNE\n\r AND  CONNF\n\r
          if (strncmp(SCData.buffer, "DISCE", 5) == 0) {
            if (DEVICE_ID == 255) {
              Serial.write("SteamController not found - repeating discovery process\n");
              Serial1.write("AT+DISC?");
              DEVICE_ID = 255;
              TEMP_ID = -1;
            } else {
              Serial.write("SteamController found - Establishing connection\n");
              CONNECTION_COMMAND[7] = '0' + DEVICE_ID;
              Serial1.write(CONNECTION_COMMAND, 8);
              MODE = 3;

            }
          }
        }

        SCData.it = 0;

        break;
      case 3:
        SCData.it = Serial1.readBytesUntil('+', SCData.buffer, 64);

        if (SCData.it == 7) { //CONNF\r\n
          if (strncmp(SCData.buffer, "CONNF\r\n", 7) == 0 || strncmp(SCData.buffer, "CONNN\r\n", 7) == 0 || strncmp(SCData.buffer, "CONNE\r\n", 7) == 0) {
            Serial.write("CONNECTION FAILED - falling back to discovery mode\n");
            Serial1.write("AT+DISC?");
            MODE = 2;
          }

        } else if (SCData.it == 6) {
          if (strncmp(SCData.buffer, "CONN\r\n", 6) == 0) {
            Serial.write("CONNECTION SUCCESS\n");
            Serial1.write(DISABLE_LIZARD_MODE, 37);
            delay(100);
            Serial1.write("AT+NOTIFY_ON0029");
            MODE = 4;
            SCData.it = 0;
          }
        }
        SCData.it = 0;
        break;
      case 4:
        SCData.buffer[SCData.it] = Serial1.read();
        /*
           if(SCData.dbuffer[SCData.it] < 0x10)
           Serial.write('0');
           Serial.print(SCData.buffer[SCData.it] , HEX);
           Serial.write(" ");
        */
        if (!(SCData.it == 0 && !(SCData.buffer[SCData.it] == 0xC0 || SCData.buffer[SCData.it] == 'O')))
          SCData.it++;


        if (SCData.it == 19) {
          if (SCData.buffer[0] == 0xC0 && read_input( & SCData) == 1) {

            ba = (SCData.state.buttons & SCB_B) != 0; // A button state
            bb = (SCData.state.buttons & SCB_A) != 0; // B button state
            bx = (SCData.state.buttons & SCB_Y) != 0; // X button state
            by = (SCData.state.buttons & SCB_X) != 0; // Y button state

            bm = (SCData.state.buttons & SCB_BACK) != 0; // MINUS button state
            bhome = (SCData.state.buttons & SCB_C) != 0; // HOME button state
            bp = (SCData.state.buttons & SCB_START) != 0; // PLUS button state

            bzl = (SCData.state.buttons & SCB_LB) != 0; // ZL button state
            bzr = (SCData.state.buttons & SCB_RB) != 0; // ZR button state

            bl = (SCData.state.buttons & SCB_LT) != 0; // L button state
            br = (SCData.state.buttons & SCB_RT) != 0; // R button state


            lx = map(SCData.state.stick_x, -32767, 32767, 0, 255);
            ly = map(SCData.state.stick_y, -32767, 32767, 0, 255);

            rx = map(SCData.state.rpad_x, -32767, 32767, 0, 255);
            ry = map(SCData.state.rpad_y, -32767, 32767, 0, 255);

            lt = SCData.state.ltrig;
            rt = SCData.state.rtrig;
/*
            lx = map(SCData.state.stick_x, -32767, 32767, 0, 63);
            ly = map(SCData.state.stick_y, -32767, 32767, 0, 63);

            rx = map(SCData.state.rpad_x, -32767, 32767, 0, 31);
            ry = map(SCData.state.rpad_y, -32767, 32767, 0, 31);

            lt = map(SCData.state.ltrig, 0, 255, 0, 31);
            rt = map(SCData.state.rtrig, 0, 255, 0, 31);*/

                      sign = alfa(SCData.state.lpad_x,SCData.state.lpad_y);
                      
                      bdl = (sign == 4 || sign == 5 || sign == 6) ? 1 : 0; // D-Pad Left state
                      bdr = (sign == 1 || sign == 2 || sign == 8) ? 1 : 0; // D-Pad Right state
                      bdu = (sign == 4 || sign == 3 || sign == 2) ? 1 : 0; // D-Pad Up state
                      bdd = (sign == 6 || sign == 7 || sign == 8) ? 1 : 0; // D-Pad Down state
            /*
                     ba = 1;
                     bb = 1;
                     bx = 1;
                     by = 1;


            Serial.print(SCData.state.lpad_x);
            
            Serial.write(" ");
            Serial.print(SCData.state.lpad_y);
                        Serial.write(" ");
                      Serial.print(sign);
                        Serial.write("\r\n");
                     */
          }
          SCData.it = 0;
        } else if (SCData.it == 9 && strncmp(SCData.buffer, "OK+LOST\r\n", 9) == 0) {
          Serial.write("CONNECTION LOST - reconnecting");
          MODE = 0;
          SCData.it = 0;

        }

        break;
      default:
        Serial.write(Serial1.read());

    }
  }
}
