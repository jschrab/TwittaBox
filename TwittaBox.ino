// ----------------------------------------------------------------------------
// TwittaBox 9001
// ----------------------------------------------------------------------------
// 30-MAY-2011 - JDS - Incept date
// 14-JUL-2012 - JDS - Wow. Actually DOES something now!
// ----------------------------------------------------------------------------

#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>
#include <StackListPlus.h>
#include <Bounce.h>
#include <SoftwareSerial.h>

// ----------------------------------------------------------------------------

#define GREEN_BUTTON         7
#define RED_BUTTON           6

#define ATTENTION_LED        8
#define MORE_ATTENTION_LED   9

#define BOFF                 0
#define BON                  1500
#define BFAST                750

#define txPin 2
#define rxPin 5

// ----------------------------------------------------------------------------
// IP / MAC address config - Make sure unique on the network!
// ----------------------------------------------------------------------------

static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static uint8_t ip[]  = { 10, 60, 70, 242 };

// ----------------------------------------------------------------------------

P(INDEX_PREFIX) = "<!DOCTYPE html><head>";
P(INDEX_REDIRECT) = "<meta http-equiv=\"refresh\" content=\"2;url=index.html\" />";
P(INDEX_PAGE_PT1) = "<meta charset=\"utf-8\" /><title>TwittaBox 9001 - Version 0.9.2</title><style type=\"text/css\"> body{margin: 50px 0px;font-family: Consolas, \"Andale Mono WT\", \"Andale Mono\", \"Lucida Console\", \"Lucida Sans Typewriter\", \"DejaVu Sans Mono\", \"Bitstream Vera Sans Mono\", \"Liberation Mono\", \"Nimbus Mono L\", Monaco, \"Courier New\", Courier, monospace;color:#fff;background: #333}#container{box-shadow: inset 0 0 20px rgba(0,0,0,.3), 4px 4px 12px rgba(0,0,0,.4);width: 620px;margin: 0px auto;text-align: left;padding: 15px 30px 30px;border: 4px solid #999;border-radius: 6px;background:#666}h1{margin:15px 0;color:#fff;text-shadow: -2px -2px 1px rgba(0,0,0,.5), 2px 2px 1px rgba(255,255,255,.2);}#main{margin: 0 0 0 20px}#msg{border:1px solid #999;font: bold 16px Consolas, \"Andale Mono WT\", \"Andale Mono\", \"Lucida Console\", \"Lucida Sans Typewriter\", \"DejaVu Sans Mono\", \"Bitstream Vera Sans Mono\", \"Liberation Mono\", \"Nimbus Mono L\", Monaco, \"Courier New\", Courier, monospace;width: 300px;color:#f00;background: #000;padding: 4px 8px;}#send{text-align:center;margin-top: 10px;}#send input{font: bold 16px/16px Consolas, \"Andale Mono WT\", \"Andale Mono\", \"Lucida Console\", \"Lucida Sans Typewriter\", \"DejaVu Sans Mono\", \"Bitstream Vera Sans Mono\", \"Liberation Mono\", \"Nimbus Mono L\", Monaco, \"Courier New\", Courier, monospace;color: #fff;background: #f00;border:1px solid #900;border-radius:60px;padding: 24px 14px;box-shadow: inset 1px 1px 2px rgba(255,255,255,.6), inset -1px -1px 2px rgba(0,0,0,.2);text-shadow: -1px -1px 1px rgba(0,0,0,.5), 1px 1px 1px rgba(255,255,255,.2);cursor:pointer;}#send input:hover{box-shadow: inset 1px 1px 2px rgba(255,255,255,.6), inset -1px -1px 2px rgba(0,0,0,.2), 0px 0px 14px #900;}#msg-area{text-align: center;padding-top: 12px;}</style></head><body><div id=\"container\"><h1>TwittaBox 9001</h1><div id=\"main\"><form action=\"go\" method=\"post\"><label for=\"msg\">Message </label><input type=\"text\" name=\"msg\" id=\"msg\" maxlength=\"30\" /><input type=\"checkbox\" name=\"is_imp\" id=\"is_imp\" />It's important!<div id=\"send\"><input type=\"submit\" value=\"Send\" /></div></form><div id=\"msg-area\">";
P(INDEX_PAGE_MSG_1) = "30 character messages are anonymous";
P(INDEX_PAGE_MSG_2a) = "Message Sent! ";
P(INDEX_PAGE_MSG_2b) = " of 10 messages in the queue.";
P(INDEX_PAGE_MSG_3) = "Message queue full! Try again later.";
P(INDEX_PAGE_PT2) = "</div></div></div><script>document.getElementById('msg').focus();</script></body></html>";

// ----------------------------------------------------------------------------
// Set up software serial
// ----------------------------------------------------------------------------

SoftwareSerial LCD =  SoftwareSerial(0, txPin);

// ----------------------------------------------------------------------------

unsigned  loopCounter = 0;

const int  ledPins[]          = { ATTENTION_LED, MORE_ATTENTION_LED };
const int  buttons[]          = { GREEN_BUTTON, RED_BUTTON };

#define numButtons (sizeof(buttons)/sizeof(int)) //array size
#define numLeds (sizeof(ledPins)/sizeof(int)) //array size

const long DEBOUNCE_DELAY = 5;

Bounce bouncer[]        = { Bounce( GREEN_BUTTON, DEBOUNCE_DELAY ), Bounce( RED_BUTTON, DEBOUNCE_DELAY ) };

int  ledBlinkState[]    = { BOFF,   BOFF };
int  ledBlinkToggle[]   = { 0,      0    };

StackListPlus <String> msgStack[1];
    
// ----------------------------------------------------------------------------

#define PREFIX ""
WebServer webserver(PREFIX, 80);

#define NAMELEN 32
#define VALUELEN 32

// ----------------------------------------------------------------------------
// SparkFun SerialLCD display utility functions
// ----------------------------------------------------------------------------

void clearLCD(){
    LCD.write(0xFE);   //command flag
    LCD.write(0x01);   //clear command.
    delay(50);
} // function..clearLCD

void selectLineOne(){  //puts the cursor at line 0 char 0.
    LCD.write(0xFE);   //command flag
    LCD.write(0x80+0);
    delay(50);
} // function..selectLineOne

void selectLineTwo(){  //puts the cursor at line 1 char 0.
    LCD.write(0xFE);   //command flag
    LCD.write(0x80+64+16);
    delay(50);
} // function..selectLineTwo

// ----------------------------------------------------------------------------

void indexCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
    server.httpSuccess();
    
    if (type == WebServer::HEAD)
        return;    

    if(msgStack[0].count() < 10) {
        server.printP(INDEX_PREFIX);
        server.printP(INDEX_PAGE_PT1);
        server.printP(INDEX_PAGE_MSG_1);
        server.printP(INDEX_PAGE_PT2);
    } else {
        server.printP(INDEX_PREFIX);
        server.printP(INDEX_PAGE_PT1);
        server.printP(INDEX_PAGE_MSG_3);
        server.printP(INDEX_PAGE_PT2);
    } // if..else
} // function..indexCmd

// ----------------------------------------------------------------------------

void goCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
    bool repeat;
    char name[NAMELEN], value[VALUELEN];
  
    switch (type) {
        case WebServer::HEAD:
            server.httpSuccess();
            return;
        case WebServer::POST:
            do {
                repeat = server.readPOSTparam(name, NAMELEN, value, VALUELEN);
                if (strcmp(name, "msg") == 0) {
                    ledBlinkState[0] = BON;
                    digitalWrite(ledPins[0],HIGH);
                    msgStack[0].push(value);
                    clearLCD();
                    selectLineOne();
                    LCD.print(msgStack[0].count());
                    LCD.print(" New Messages");
                } // if
                if (strcmp(name, "is_imp") == 0) {
                    ledBlinkState[1] = BON;
                    digitalWrite(ledPins[1],HIGH);
                } // if
            } while (repeat);

            server.printP(INDEX_PREFIX);
            server.printP(INDEX_REDIRECT);
            server.printP(INDEX_PAGE_PT1);
            server.printP(INDEX_PAGE_MSG_2a);
            server.print(msgStack[0].count());
            server.printP(INDEX_PAGE_MSG_2b);
            server.printP(INDEX_PAGE_PT2);

            break;
    } // switch..case
} // function..indexCmd

// ----------------------------------------------------------------------------

void setup() {
    pinMode(ATTENTION_LED,      OUTPUT);
    pinMode(MORE_ATTENTION_LED, OUTPUT);

    pinMode(GREEN_BUTTON,       INPUT);
    pinMode(RED_BUTTON,         INPUT);

    pinMode(txPin, OUTPUT);
    LCD.begin(9600);

    clearLCD();
    selectLineOne();
    LCD.print("No New Messages");

    Ethernet.begin(mac, ip);

    webserver.setDefaultCommand(&indexCmd);
    webserver.addCommand("index.html", &indexCmd);
    webserver.addCommand("go", &goCmd);
    webserver.begin();
} // function..setup

// ----------------------------------------------------------------------------
// *** MAIN LOOP ***
// ----------------------------------------------------------------------------

void loop() {
    char buff[256];
    int len = 256;
    
    for(int i=0;i<numButtons;i++) {
        bouncer[i].update();

        //-------------------------------------------------------------------------
        // Set blink states of lights
        //-------------------------------------------------------------------------
        if(ledBlinkState[i]!=BOFF) {
            if((loopCounter % ledBlinkState[i])==0) {
                ledBlinkToggle[i] = !ledBlinkToggle[i];
            } // if
            if(ledBlinkToggle[i]) {
                digitalWrite(ledPins[i],HIGH);
            } else {
                digitalWrite(ledPins[i],LOW);
            } // if..else
        } // if
    } // for..i

    //-------------------------------------------------------------------------
    //- Event: GREEN_BUTTON - advance to next message
    //-------------------------------------------------------------------------
    if(bouncer[1].risingEdge()) {
        // Turn blink states off - if button pushed
        ledBlinkState[0] = BOFF;
        digitalWrite(ledPins[0],LOW);

        ledBlinkState[1] = BOFF;
        digitalWrite(ledPins[1],LOW);
 
        clearLCD();
        selectLineOne();
        if(msgStack[0].count()==0) {
            LCD.print("No New Messages");
        } else {
            LCD.print(msgStack[0].count());
            LCD.print(":"+msgStack[0].peek());
            msgStack[0].tack(msgStack[0].pop());
        } // if..else
    } // if

    //-------------------------------------------------------------------------
    //- Event: RED_BUTTON - delete message
    //-------------------------------------------------------------------------
    if(bouncer[0].risingEdge()) {
        if(msgStack[0].count()!=0) {
            msgStack[0].pop();

            clearLCD();
            selectLineOne();
            LCD.print("Message Deleted");

            delay(750);

            clearLCD();
            selectLineOne();
            if(msgStack[0].count()==0) {
                LCD.print("No New Messages");
            } else {
                LCD.print(msgStack[0].count());
                LCD.print(":"+msgStack[0].peek());
            } // if..else
        } // if
    } // if
    
    //-------------------------------------------------------------------------

    webserver.processConnection(buff, &len);

    loopCounter++; //-- don't care about overflow - rap around is fine
} // function..loop

// ----------------------------------------------------------------------------

