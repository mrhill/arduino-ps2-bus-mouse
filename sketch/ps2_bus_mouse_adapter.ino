/*
 * Arduino sketch to PS2 mouse (or USB mouse operating in PS2 mode)
 * signals to quadrature-encoded bus mouse signals for older
 * computers.
 * 
 * Author: Stephen Pick
 * License: MIT
 * Based on an original sketch by ezContents (https://ezcontents.org/acorn-arduino-ps2-mouse-adapter)
 * 
 */

/* BEGIN CONFIG */

// #define DEBUG_PRINT

/* Arduino pins - bus mouse */
#define PIN_BUS_BUTTON1 2
#define PIN_BUS_BUTTON2 4
#define PIN_BUS_BUTTON3 3
#define PIN_BUS_XDIR 7
#define PIN_BUS_XREF 8
#define PIN_BUS_YDIR 9
#define PIN_BUS_YREF 10

/* Arduino pins - ps/2 mouse */
#define PIN_PS2_VCC 12   /* +5V   PS2: 4 USB: 1 (RED) */
#define PIN_PS2_CLOCK 6  /* Clock PS2: 5 USB: 3 (WHITE) */
#define PIN_PS2_DATA 5   /* Data  PS2: 1 USB: 2 (GREEN) */
                         /* GND   PS2: 3 USB: 4 (BLACK) */ 
/* END CONFIG */

#include <ps2.h>

/* PS2 commands and responses */
#define PS2_CMD_RESET 0xFF
#define PS2_CMD_MOUSE_MODE_REMOTE 0xF0
#define PS2_CMD_MOUSE_READ_DATA 0xEB
#define PS2_CMD_MOUSE_MODE_STREAM 0xEA
#define PS2_CMD_MOUSE_READ_DEVICETYPE 0xF2
#define PS2_CMD_MOUSE_SET_SAMPLERATE 0xF3
#define PS2_CMD_MOUSE_SET_RESOLUTION 0xE8
#define PS2_CMD_MOUSE_SET_SCALING1TO1 0xE6
#define PS2_CMD_MOUSE_ENABLE 0xF4
#define PS2_CMD_MOUSE_SET_DEFAULTS 0xF6

#define PS2_RSP_ACK 0xFA
#define PS2_RSP_MOUSE_TEST_PASS 0xAA

PS2 mouse(PIN_PS2_CLOCK, PIN_PS2_DATA);

int xrot = 0;
int yrot = 0;

bool send_ps2_cmd(const unsigned char request)
{
  mouse.write(request);
  if(!expect_ps2(PS2_RSP_ACK))
  {
    Serial.print("CMD ");
    Serial.print(request, HEX);
    Serial.println(" FAIL");
    return false;
  }
  return true;
}

bool expect_ps2(const unsigned char response)
{
  unsigned char result;
  result = mouse.read();  // ack byte
  if(result != response)
  {
    Serial.print("EXPECTED ");
    Serial.print(response, HEX);
    Serial.print(" GOT ");
    Serial.println(result, HEX);
    return false;
  }
  return true;
}

bool ps2_mouse_init()
{
  Serial.println("Initialising...");
  
  // power-cycle the mouse:
  digitalWrite(PIN_PS2_VCC, LOW);
  delay(250);
  digitalWrite(PIN_PS2_VCC, HIGH);
  delay(500);

  for (int i=0; i<3; i++)
  {
    if(!send_ps2_cmd(PS2_CMD_RESET))
      return false;
    Serial.println("Reset sent...");

    if(!expect_ps2(PS2_RSP_MOUSE_TEST_PASS))
      return false;
    Serial.println("Mouse test pass!");
    
    unsigned char mouseId = mouse.read();
    Serial.print("Mouse ID ");
    Serial.println(mouseId, HEX);
  }

  Serial.println("Set sample rate 10...");
  if (!send_ps2_cmd(PS2_CMD_MOUSE_SET_SAMPLERATE) || !send_ps2_cmd(10))
    return false;

  if (!send_ps2_cmd(PS2_CMD_MOUSE_READ_DEVICETYPE))
    return false;
  unsigned char deviceType = mouse.read();
  Serial.print("Device Type ");
  Serial.println(deviceType, HEX);

  Serial.println("Set resolution...");
  if (!send_ps2_cmd(PS2_CMD_MOUSE_SET_RESOLUTION) || !send_ps2_cmd(3))
    return false;

  Serial.println("Set scaling 1:1...");
  if (!send_ps2_cmd(PS2_CMD_MOUSE_SET_SCALING1TO1))
    return false;

  Serial.println("Set sample rate 100...");
  if (!send_ps2_cmd(PS2_CMD_MOUSE_SET_SAMPLERATE) || !send_ps2_cmd(100))
    return false;

  if (!send_ps2_cmd(PS2_CMD_MOUSE_ENABLE))
    return false;
  Serial.println("Enabled");

  // if (!send_ps2_cmd(PS2_CMD_MOUSE_MODE_REMOTE))
  //   return false;
  // Serial.println("Remote mode active");

  return true;
}

void setup()
{
  pinMode(PIN_PS2_VCC, OUTPUT);
  pinMode(PIN_BUS_BUTTON1, OUTPUT);
  pinMode(PIN_BUS_BUTTON2, OUTPUT);
  pinMode(PIN_BUS_BUTTON3, OUTPUT);
  pinMode(PIN_BUS_XDIR, OUTPUT);
  pinMode(PIN_BUS_XREF, OUTPUT);  
  pinMode(PIN_BUS_YDIR, OUTPUT);  
  pinMode(PIN_BUS_YREF, OUTPUT);

  Serial.begin(9600);
  
  while(!ps2_mouse_init())
  {
  }
}

void quad_inc(int& rot)
{
  switch(rot)
  {
    case 0b00:
      rot = 0b01;
      break;
    case 0b01:
      rot = 0b11;
      break;
    case 0b11:
      rot = 0b10;
      break;
    case 0b10:
      rot = 0b00;
      break;
  }
}

void quad_dec(int& rot)
{
  switch(rot)
  {
    case 0b00:
      rot = 0b10;
      break;
    case 0b10:
      rot = 0b11;
      break;
    case 0b11:
      rot = 0b01;
      break;
    case 0b01:
      rot = 0b00;
      break;
  }
}

void loop()
{
  unsigned char mstat;
  char mx;
  char my;

  //send_ps2_cmd(PS2_CMD_MOUSE_READ_DATA);
  
  mstat = mouse.read();
  mx = mouse.read();
  my = mouse.read();

  #ifdef DEBUG_PRINT
  if (mx|my) {
    Serial.print(mstat, HEX);
    Serial.print(' ');
    Serial.print(mx, DEC);
    Serial.print(' ');
    Serial.print(my, DEC);
    Serial.println();
  }
  #endif DEBUG_PRINT

  bool b1 = mstat & 0b001;
  bool b2 = mstat & 0b010;
  bool b3 = mstat & 0b100;

  digitalWrite(PIN_BUS_BUTTON1, !b1);
  digitalWrite(PIN_BUS_BUTTON2, !b2);
  digitalWrite(PIN_BUS_BUTTON3, !b3);

  while(mx != 0 || my != 0)
  {
    if (mx > 0) 
    {
      mx--;
      quad_inc(xrot);
    }
    else if (mx < 0) 
    {
      mx++;
      quad_dec(xrot);
    }
  
    digitalWrite(PIN_BUS_XDIR, xrot & 0b01);
    digitalWrite(PIN_BUS_XREF, xrot & 0b10);
  
    if (my > 0) 
    {
      my--;
      quad_inc(yrot);
    }
    else if (my < 0) 
    {
      my++;
      quad_dec(yrot);
    }
  
    digitalWrite(PIN_BUS_YDIR, yrot & 0b01);
    digitalWrite(PIN_BUS_YREF, yrot & 0b10);
    
    delayMicroseconds(200);
  }
}
