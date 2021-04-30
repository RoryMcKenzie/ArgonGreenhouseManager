// This #include statement was automatically added by the Particle IDE.
#include <LiquidCrystal.h>

// This #include statement was automatically added by the Particle IDE.
#include <Keypad_Particle.h>

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_DHT.h>

// This #include statement was automatically added by the Particle IDE.
#include "MQTT.h"

void callback(char* topic, byte* payload, unsigned int length);

// Create an MQTT client
MQTT client("test.mosquitto.org", 1883, callback);

//Set amount of time to wait after being locked out
const int LOCKOUTTIME = 30;
//Set number of attempts before being locked out
const int ATTEMPTS = 3;

//Set number of rows and columns for keypad
const int ROWS = 4;
const int COLUMNS = 3;


//Create temperature sensor object
DHT dht(D1, DHT22);

//Variables for temperature and humidity
double temperature;
double humidity;

//Create variables for each line on the LCD
char line0[17] = "Enter password: ";
char line1[17];

//Set key layout of keypad
char keys[ROWS][COLUMNS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

//Connect rows and columns to the pins on the Argon
byte pin_rows[ROWS] = {D8, D7, D6, D5};
byte pin_column[COLUMNS] = {D4, D3, D2};

//Make keymap from 2d array
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROWS, COLUMNS);

//Setup LCD with connected Argon pins
LiquidCrystal lcd(A0, A1, A2, A3, A4, A5);

//Function to update display of LCD with correct text
void updateDisplay() {
   lcd.setCursor(0,0);
   lcd.print(line0);
   lcd.setCursor(0,1);
   lcd.print(line1);
}

//Create variable to store user-inputted password
char password[4];
//Create variable to store correct password
char initialPassword[4] = {'1','2','3','4'};
//Create variable to store which digit of the password the user is inputting (1st, 2nd etc)
int passwordPosition=0;
//Create variable to store how many attempts user has left
int attemptsLeft = ATTEMPTS;

//Function to get a reading from the DHT sensor
void readSensor() {
    humidity = dht.getHumidity();
    temperature = dht.getTempCelcius();
}


//Function called when Argon receives an MQTT message
void callback(char* topic, byte* payload, unsigned int length) 
{
    //Only runs if client is connected to MQTT broker
     if (client.isConnected())
    {
        readSensor();
        // Check if any reads failed 
        if (isnan(humidity) || isnan(temperature)) {
            client.publish("readingerror", "could not read value");
            return;
        }
        
        //Publish MQTT message with temperature and humidity data
        client.publish("tempandhum", String(temperature) + "," + String(humidity));

    } else{
        Particle.publish("connection", "not connected", PRIVATE);
    } 
}


// Setup the Argon
void setup() 
{
    //setup lcd with 16 columns, 2 rows
    lcd.begin(16,2);

    //set lcd to initial values
    updateDisplay();

    // Connect to the server
    client.connect("argon");
    
    if (client.isConnected()) {
        client.subscribe("tempandhumrequest");
    }
    
    //Start DHT sensor
    dht.begin();
}


// Main loop
void loop() 
{
    char key = keypad.getKey();

    //If a key has been pressed
    if (key){
        //If the key is the hash symbol, the password being entered is undone
        if (key == '#'){
            strcpy(line1, "                ");
            updateDisplay();
            passwordPosition = 0;
        } else {
            password[passwordPosition++]=key;
            //Switch statement to update the display with an asterisk 
            //to represent the digit if the user has typed 3 or fewer digits
            switch(passwordPosition){
                case 1:
                    line1[0] = '*';
                    updateDisplay();
                    break;
                case 2:
                    line1[1] = '*';
                    updateDisplay();
                    break;
                case 3:
                    line1[2] = '*';
                    updateDisplay();
                    break;
                case 4:
                    line1[3] = '*';
                    updateDisplay();
                    delay(500);
                    //If the user has entered the password correctly, the text will change to tell them this,
                    //attempts left and passwordPosition are both reset.
                    if (!strncmp(password, initialPassword, 4)){
                        strcpy(line0, "  Door opened   ");
                        strcpy(line1, "Password correct");
                        updateDisplay();
                        delay(10000);
                        passwordPosition = 0;
                        attemptsLeft = ATTEMPTS;
                        strcpy(line0, "Enter password: ");
                        strcpy(line1, "                ");
                        updateDisplay();
                        break;
                    } else {
                        //If the password is incorrect and not run out of attempts,
                        //changes text to tell user how many attempts left
                        if (attemptsLeft > 1){
                            attemptsLeft--;
                            if (attemptsLeft == 1){
                                strcpy(line1, "  attempt left ");
                            } else {
                                strcpy(line1, "  attempts left");
                            }
                            line1[0] = attemptsLeft+'0';
                            updateDisplay();
                            delay(2000);
                            strcpy(line1, "                ");
                            updateDisplay();
                            passwordPosition = 0;
                        } else {
                            //If user has run out of attempts, text changed to say they're locked out
                            client.publish("notification", "locked out");
                            strcpy(line0, " You are locked ");
                            //Countdown timer until the user isn't locked out
                            for (int timer = LOCKOUTTIME; timer > 0; timer--){
                                char textTimer= timer+'0';
                                if(timer > 9){
                                    char textTimerArray[ATTEMPTS];
                                    itoa(timer, textTimerArray, 10);
                                    strcpy(line1, "   out for   s    ");
                                    line1[11] = textTimerArray[0];
                                    line1[12] = textTimerArray[1]; 
                                    updateDisplay();
                                    unsigned long currentMillis = millis();
                                    delay(1000);
                                } else {
                                    char textTimer= timer+'0';
                                    strcpy(line1, "   out for  s   ");
                                    line1[11] = textTimer;
                                    updateDisplay();
                                    delay(1000);
                                }
                            }
                            //Resets LCD display back to how it is at the start, resets passwordPosition and attempts left
                            passwordPosition = 0;
                            attemptsLeft = ATTEMPTS;
                            strcpy(line0, "Enter password: ");
                            strcpy(line1, "                ");
                            updateDisplay();
                        } 
                    }
                    break;
            } 
        }
    } 
    client.loop();
}
