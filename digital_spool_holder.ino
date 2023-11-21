  Serial.print("-17.");
#include <U8g2lib.h>
#include <assert.h>
#include <Wire.h>
// #include <U8glib.h>

/*
Digital_Spool_Holder

This version is a heavily refactored version of the original.
The original was made by InterlinkKnight, I am not aware of a github or other repo for it. 
The originalsketch is listed from the instructables page (see below).

Bugs and issues with this are likely to be as a result of my changes and should be reported through my github
repo found here: https://github.com/nhorlock/digital_spool_holder

This sketch is for a scale made to measure the weight (g) of the
remaining material in a spool. It was made mainly for 3D printers but it can be
used for any kind of spool or container.

The main difference between this and a common scale is that this device has the
possibility to add multiple profiles to store different Tare values
so that way we can measure only the weight of the remaining filament excluding
the weight of the empty spool, because each brand of filaments has a different
weight and material for the spool itself.

It uses a load cell connected to a HX711 module.

We also use an 1.3" OLED display 128x64 I2C with SSD130/SH1106 driver using the
U8GLIB library. It also works on a 0.96" OLD display of the same characteristics
but everything would look smaller so I don't recommend it.

Features:
 * This sketch can measure weights from 10g to 9999g.
 * There are 3 buttons (LEFT, ENTER, RIGHT) that can be used for scrolling 
   through different profiles and navigate through the menu system.
 * You can add, edit and delete profiles using the menu system.
   It can store up to 20 profiles, with their own name and Tare value.
   Maximum 21 characters for the name of each profile.
 * You can do a full calibration for the load cell using the menu system, 
   so there's no need to edit this sketch in any way.
 * Configurable deadzone with real-time reading on the display. The deadzone
   helps ignoring the fluctuations of the constant pulling/pushing of the
   filament in the 3D printer.
 * All the settings are stored in the EEPROM so it's going to remember all the 
   settings after powering the device off.
 * There is an option in the menu to FACTORY RESET arduino for clearing the EEPROM (user data).
 * Auto-center the tittle text in the main/normal screen ignoring spaces to the right.
 * When adding a new profile, it's going to save the new profile after the current 
   profile so the order can be arranged in a predictable way. To make this work it
   required a lot of effort so even though you might take this for granted, it's not
   a trivial feature.
 * In the menu you can set the amount of deadzone to ignore the peaks when the 3D printer 
   pulls the filament, so we show the most realistic estimate of the actual weight left in the spool.
 * When powering ON, changing profiles or returning from the menu, it shows the weight 
   starting at the lower edge of the deadzone so the device is ready to measure the decrease of weight
   of the filament immediately.
 * No timeout for being on the menu. Yes, that's right, this is a feature. Why? I hate timeouts
   that will exit the menu if no operation is done after a few seconds, but I think that is
   a bad design that causes stress and frustration because you feel like you need to rush things
   with the threat of having to start all over from the beginning. You are not going to find anything
   like that in here.


More details about the project in here: https://www.instructables.com/Digital-Spool-Holder-with-Scale/

Pins for HX711 module to arduino:
 * GND         = GND
 * VCC         = 5V
 * DT (DATA)   = 2
 * SCK (CLOCK) = 3

Load cell connection to the HX711 module:
 * RED   = E+
 * BLACK = E-
 * WHITE = A-
 * GREEN = A+

To control everything we use 3 buttons:
 * LEFT ARROW  = Pin 5: Scroll to the left.
 * ENTER       = Pin 6: This button is to enter the menu and select options.
 * RIGHT ARROW = Pin 7: Scroll to the right.
Note: When you press the button, the pin should receive GROUND, and when you
release the button the pin should receive 5V. We use the internal pullup
resistor for this but I recommend adding an aditional 10K resistor to each pin
connected to each button pin and 5V. This way is going to be more reliable.
I say this because I've seen the internal pullup resistor fail after some time.

Pins for OLED Display:
 * GND = GND
 * VCC = 5V
 * SCL = A5
 * SDA = A4

It's a good idea to put a resistor (3.3K) between A4-5V and A5-5V to help stabilize
the connection. What that does is to pull-up the I2C pins to make it more reliable
and prevents lock-ups (yes, this can happen without adding resistors).


HX711 maximum raw value allowed: 8388607

Load cell actual maximum weight reached:
 * 2Kg = 8.4Kg


Reference of weight for empty spools:
 * Plastic = 244g
 * Cardboard = 177g


Note: When exposing the cell to very heavy weights, more than the rating,
the cell permanently bends a little so may require Tare. Also, with time this
can happen but only very little (around 3g).


U8glib library: https://github.com/olikraus/u8g2
HX711 library: https://github.com/bogde/HX711


User Reference Manual: https://github.com/olikraus/u8g2/wiki
List of fonts: https://github.com/olikraus/u8g2/wiki/fntgrp

Original Sketch made by: InterlinkKnight - Last modification: 2023/10/16
Forked by nhorlock (Zyxt) latest updates on github: https://github.com/nhorlock/digital_spool_holder
*/
// #define DEBUG
#ifdef DEBUG
#define DEBUGS(Str) Serial.println(F( Str ))
#define DEBUGV(Val) Serial.print( F(#Val ": ") ); Serial.println(Val)
#else
#define DEBUGS(Str) (void)(F(Str))
#define DEBUGV(Val) (void)(F(#Val ": ") ); (void)(Val)
#endif
// Pins:
#define LEFT_BUTTON_PIN 5  // Pin for LEFT button
#define ENTER_BUTTON_PIN 6  // Pin for ENTER button
#define RIGHT_BUTTON_PIN 7  // Pin for RIGHT button

#define PROGRESS_BAR_FULL 123
// Create display and set pins:
// U8G2_SSD1306_128X64_NONAME_2_SW_I2C u8g(U8G2_R0, SCL, SDA);
U8G2_SH1106_128X64_NONAME_2_HW_I2C u8g(U8G2_R0, SCL, SDA);
// U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_DEV_0|U8G_I2C_OPT_FAST);  // Dev 0, Fast I2C / TWI

#include "HX711.h"  // Include HX711 library for the load cell

// Set pins for HX711 module:
const byte LOADCELL_DOUT_PIN = 2;
const byte LOADCELL_SCK_PIN = 3;

HX711 scale;  // Create object for the load cell and give it a name


#include "EEPROM.h"  // Include EEPROM library to be able to store settings

///////////////
// ADJUSTMENTS:
///////////////
// The following items are the values that you could want to change:

// Smoothing the weight:
const byte numReadings1 = 20;  // Number of samples for smoothing. The more samples, the smoother the value
                               // although that will delay refreshing the value. 

const byte MinimumWeightAllowed = 10;  // When the weight is below this value, we are going to consider that the weight is actually 0 so we show a 0


// Debouncing buttons:
const byte ButtonDebouncingAmount = 1;  // Debouncing amount (cycles) for how long to wait. The higher, the more
                                        // it will wait to see if button remain release to really consider is released.
                                        // It adds a lag to make sure we really release the button, ignoring micro pulses.

// Holding buttons:
const byte RepeatWhenWeHoldAmount = 5;  // For how long (cycles) to wait when holding the button, before considering is holded
const byte RepeatWhenWeHoldFrequencyDelay = 1;  // How often (cycles) repeat pulses when we hold the button.
                                // 0 = one cycle pulse and one cycle no (101010101...)
                                // 1 = pulse every 2 cycles (1001001001...)
                                // 2 = pulse every 3 cycles (1000100010001...)

const byte WeightYPosition = 50;  // The Y position of the weight when in normal screen
const byte SelectBoxWigth = 55;  // This is how wide the selection box is when is in the center
const byte SelectBoxHeight = 12;  // This is how high the selection box is

const byte SelectBoxXPositionRight = 73;  // X position of the selection box that goes to the right
const byte SelectBoxYPosition = 52;  // Y position of the selection box
const byte SelectBoxXPositionCenter = 36;  // X position of the selection box that goes to the center

const byte ProgressBarSpeedAmount = 5;  // How many cycles to wait to increase the progress bar by 1 pixel.
                                        // The higher, the slower the bar will increase.

// Text Cursor:
const byte BlinkingCursorAmount = 5;  // How many cycles go between blincking the cursor when editing text.
                                      // The higher, the slower is going to do the blinking.
#define NUM_PROFILE_SLOTS 20

const int EEPROM_AddressStart = 0;  // Address where the settings are stored in the EEPROM

/////////////
// Variables:
/////////////

#define MAX_PROFILE_NAME_LEN 21
// Define a struct for profile data
struct UserProfile {
  int emptySpoolMeasuredWeight;
  float fullSpoolFilamentWeight;
  char name[MAX_PROFILE_NAME_LEN+1];
};

// Define a struct for the configuration data
struct Configuration {
  byte calibrationDone; // was offset 0
                        // 0 = Full Calibration never doned
                        // 1 = Full Calibration has been done before
                        // Uses 1 byte
  float emptyHolderRaw; // empty holder (4 byte float)
  float fullSpoolRaw; // full spool weight as a raw HX711 value (used in full calibration)
  float fullSpoolWeight; // full spool calibration value (externally measured)
  byte deadzoneAmount; // deadzone value
  byte sequenceOfCurrentProfileSlot; // active profile
                          // This is so every power cycle, it loads the profile that was open last. 
                          // Uses 1 bytes
                          // Next Address available is 15 to 19
  UserProfile profiles[NUM_PROFILE_SLOTS]; // profile data
  int userProfileSlotOrderSequence[NUM_PROFILE_SLOTS]; // EEPROM sequence (to allow non-linear arrangement of profile - recent next to current)
};

Configuration config;
UserProfile* currentProfilePtr = nullptr;

void drawStr(int x, int y, const char* text)
{
  u8g.setCursor(x,y);
  u8g.print((class __FlashStringHelper *)text);
}

// Function to read configuration data from EEPROM
void readConfigurationFromEEPROM() {
  EEPROM.get(EEPROM_AddressStart, config);
}

// Function to write configuration data to EEPROM
void writeConfigurationToEEPROM() {
  EEPROM.put(EEPROM_AddressStart, config);
}

// End of EEPROM Config handling

byte ProgressBarCounter;  // Counter for progress bar
byte ProgressBarSpeedCounter;  // The counter for the speed of the progress bar


byte WeightXPosition;  // X position to display the weight. It depends on the amount of digits so I later change this depending on that

long FullCalibrationRawValueEmptyHolder;  // EEPROM with calibrated zero raw value

byte BlinkingCursorCounter;  // Counter for blincking the cursor when editing text
byte BlinkingCursorState = 1;  // State for blincking the cursor when editing text

long RawLoadCellReading;  // Raw reading from load cell
long HolderWeight;  // Current weight converted to grams, based on calibration
long CurrentProfileWeightShown;  // This variable stores the weight minus the weight of the empty spool on the current profile
long SmoothedWeight;  // Weight with smooth so is more stable
long SmoothedWeightToShowAsInputInDeadzone;  // When we are in deadzone selection, this is the input is going to show

char tempProfileTitleBuffer[22] = "PROFILE 1";  // Character array for profile name before checking the amount of characters.
                                              // The current profile name is stored here, readed from EEPROM

long WeightWithDeadzone;  // Weight with deadzone
long FinalWeightToShow;

// Variables for smoothing weight:
long readings1[numReadings1];  // The input
int readIndex1;  // The index of the current reading
long total1;  // The running total


byte TitleXPosition;  // Variable to store the X position for printing profile title.
                    // This change depending on the amount of characters to center the text.

byte gSymbolXPosition;  // X position of the g symbol. Depends on the amount of digits of the weight

int ScreenPage;  // Page of the display

byte CounterForWhileEEPROMSectorState;  // I use this in a while function to go through all sectors of EEPROM and see how many are available
  
byte SelectorPosition;  // Stores the position of the cursor to select an option

byte ArrayCounterForCheckingSectorsNewProfile;  // A counter for a while to check what's the next sector available, when adding a new profile

byte TemporalSlotAvailable;  // Profile slot we are going to write with the previous used profile. This is used when we copy/move slots when adding a new profile
byte TemporalSlotToBeCopy;  // Profile slot we are going to copy to the next available slot. This is used when we copy/move slots when adding a new profile
 
byte TargetUserProfileSlot;  // Stores the target user profile slot that we are trying to get available when creating a new profile
byte SectorThatNowIsAvailable;  // When adding a new profile, it stores the EEPROM sector that is now available, so we can later move other sectors to here

// Constant Text in PROGMEM:
// The follwing list is all the text is going to be printed in the display that
// is static (never changes).
// The reason of putting it here is so it doesn't get stored in the Dynamic Memmory (RAM).
// Using the funtion PROGMEM force using only the flash memory.
const char ProfileChars[48] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ&'*+,-./0123456789";
const char ABCTextCharacterOPTION1[6] PROGMEM  = "SPACE";
const char ABCTextCharacterOPTION3[10] PROGMEM = "BACKSPACE";
const char ABCTextCharacterOPTION4[7] PROGMEM  = "CANCEL";
const char ABCTextCharacterOPTION5[5] PROGMEM  = "SAVE";

// Entering letters or numbers:
// The idea is to scroll letters or numbers left or right and show the adjacent character on each side to show that you can choose.
// Example:
// ABC[D]EFG
// Characters: ABCDEFGHIJKLMNOPQRSTUVWXYZ&'*+,-./0123456789 SPACE BACKSPACE CANCEL SAVE

const char TextGOBACK[8] PROGMEM = "GO BACK";
const char TextNO[3] PROGMEM = "NO";
const char TextSTART[6] PROGMEM = "START";
const char TextCANCEL[7] PROGMEM = "CANCEL";
const char TextCONTINUE[9] PROGMEM = "CONTINUE";
const char TextRETRY[] PROGMEM = "RETRY";
const char TextOK[3] PROGMEM = "OK";
const char TextDELETE[7] PROGMEM = "DELETE";

const char TextTheloadcell[14] PROGMEM = "The load cell";
const char Textrequirescalibration[22] PROGMEM = "requires calibration.";
const char TextDoyouwanttostart[21] PROGMEM = "Do you want to start";
const char Textthefullcalibration[21] PROGMEM = "the full calibration";
const char Textprocess[9] PROGMEM = "process?";

const char TextMAINMENU[10] PROGMEM = "MAIN MENU";
const char TextEDITTHISPROFILE[20] PROGMEM = "- EDIT THIS PROFILE";
const char TextADDNEWPROFILEMenu[18] PROGMEM = "- ADD NEW PROFILE";
const char TextOPTIONSMenu[10] PROGMEM = "- OPTIONS";
const char TextGOBACKMenu[10] PROGMEM = "- GO BACK";

const char TextEDITPROFILE[13] PROGMEM = "EDIT PROFILE";
const char TextTAREZERO[] PROGMEM = "- TARE";
const char TextEDITPROFILENAMEMenu[20] PROGMEM = "- EDIT PROFILE NAME";
const char TextDELETETHISPROFILE[22] PROGMEM = "- DELETE THIS PROFILE";

const char TextTARE[] PROGMEM = "TARE MENU";
const char TextSelecttaretype[] PROGMEM = "Choose tare method";
const char TextTareemptyspool[] PROGMEM = "- Tare empty spool";
const char TextTarefullspool[] PROGMEM = "- Tare full spool";

const char TextPlaceintheholder[20] PROGMEM = "Place in the holder";
const char Texttheemptyspoolyou[20] PROGMEM = "the empty spool you";
const char Textthefullspoolyou[20] PROGMEM = "the full spool you";
const char Textwanttouseforthis[21] PROGMEM = "want to use for this";
const char Textprofile[9] PROGMEM = "profile.";

const char TextSavingtheweightof[21] PROGMEM = "Saving the weight of";
const char TextEstimatingtheweightof[] PROGMEM = "Estimating the weight of";
const char Texttheemptyspool[17] PROGMEM = "the empty spool.";
const char TextPleasedonttouch[20] PROGMEM = "Please, don't touch";
const char Textanything[10] PROGMEM = "anything.";

const char TextTheweightofthe[18] PROGMEM = "The weight of the";
const char Textemptyspoolhasbeen[21] PROGMEM = "empty spool has been";
const char Textsaved[7] PROGMEM = "saved.";
const char Textestimated[] PROGMEM = "estimated.";
const char Textincorrectlyestimated[] PROGMEM = "incorrectly estimated.";

const char TextEDITPROFILENAME[18] PROGMEM = "EDIT PROFILE NAME";
const char TextADDNEWPROFILE[16] PROGMEM = "ADD NEW PROFILE";

const char TextMemoryisfull[16] PROGMEM = "Memory is full.";
const char TextYoucantaddmore[18] PROGMEM = "You can't add any";
const char Textprofiles[15] PROGMEM = "more profiles.";

const char TextTheprofilenamed[18] PROGMEM = "The profile named";
const char Textisgoingtobe[15] PROGMEM = "is going to be";
const char TextdeletedOK[13] PROGMEM = "deleted. OK?";

const char Texthasbeendeleted[18] PROGMEM = "has been deleted.";

const char TextYoucantdelete[17] PROGMEM = "You can't delete";
const char Texttheonlyprofile[17] PROGMEM = "the only profile";
const char Textleft[6] PROGMEM = "left.";

// Options:
const char TextOPTIONS[8] PROGMEM = "OPTIONS";
const char TextDEADZONEMenu[11] PROGMEM = "- DEADZONE";
const char TextFULLCALIBRATION[19] PROGMEM = "- FULL CALIBRATION";
const char TextFACTORYRESETMenu[16] PROGMEM = "- FACTORY RESET";

// Deadzone:
const char TextDEADZONE[9] PROGMEM = "DEADZONE";
const char TextINPUT[6] PROGMEM = "INPUT";
const char TextOUTPUT[7] PROGMEM = "OUTPUT";

const char TextThisprocedureis[18] PROGMEM = "This procedure is";
const char Textabouttakinga[15] PROGMEM = "about taking a";
const char Textreferencefromareal[22] PROGMEM = "reference from a real";
const char Textscaletocalibrate[19] PROGMEM = "scale to calibrate";
const char Texttheloadcell[15] PROGMEM = "the load cell.";

const char TextPlaceanyfullspool[21] PROGMEM = "Place any full spool";
const char Textonarealscaleand[20] PROGMEM = "on a real scale and";
const char Texttakenoteofthe[17] PROGMEM = "take note of the";
const char Textexactweightgrams[22] PROGMEM = "exact weight (grams).";

const char TextEntertheweight[17] PROGMEM = "Enter the weight";

const char TextPlacethatsamefull[21] PROGMEM = "Place that same full";
const char Textspoolintheholder[21] PROGMEM = "spool in the holder.";

const char Textthefullspool[16] PROGMEM = "the full spool.";

const char TextWeightsaved[14] PROGMEM = "Weight saved.";
const char TextRemovethespoolfrom[22] PROGMEM = "Remove the spool from";
const char Texttheholderleaving[20] PROGMEM = "the holder, leaving";
const char Texttheholdercompletely[22] PROGMEM = "the holder completely";
const char Textempty[7] PROGMEM = "empty.";

const char TextThecalibrationhas[20] PROGMEM = "The calibration has";
const char Textbeencompleted[16] PROGMEM = "been completed.";

const char Textg[2] PROGMEM = "g";

const char TextSavingthevalueof[20] PROGMEM = "Saving the value of";
const char Textanemptyholder[17] PROGMEM = "an empty holder.";

const char TextFactoryreseterases[21] PROGMEM = "Factory reset erases";
const char Textallyouruserdata[20] PROGMEM = "all your user data.";
const char TextFactoryresetin[17] PROGMEM = "Factory reset in";
const char Textprogress[10] PROGMEM = "progress.";
const char TextPleasewait[15] PROGMEM = "Please wait...";

const int initialfullSpoolWeight = 1250;  // When doing a full calibration, this is the weight of the full spool selected by the user

int EditTextCursorPosition;  // Editing text - Cursor position

int TextCursorPositionX;  // X position for text cursor

byte BackspacePosition;  // When editing text, the BACKSPACE moves the characters that are on the right side and mvoes it to the left.
                         // This variable is used to temporally store the position of the "while loop" to move all characters.


void setScreenPage(int page)
{
  ScreenPage = page;
  DEBUGV(ScreenPage);
}
// Refactored EEPROM Address layou structures
enum PAGES
{
  START_PAGE = 0,
  NORMAL_PAGE= 100,
  MAIN_MENU_PAGE = 1001,
  EDIT_PROFILE_PAGE = 2101,
  TARE0_PAGE = 2110,
  TARE1_PAGE = 2111,
  TARE2_PAGE = 2120, // Tare 2: saving the weight
  TARE3_PAGE = 2131, // Tare 3: weight saved, ok to continue
  EDIT_PROFILE_NAME_PAGE = 2201,
  DELETE_PROFILE_PAGE = 2301,
  PROFILE_DELETED_PAGE = 2311,
  PROFILE_NOT_DELETED_PAGE = 2312, // only 1 left, delete failed
  ENTER_FILAMENT_WEIGHT_PAGE = 2401, // Enter filament weight
  FULL_TARE2_PAGE = 2402, // Add the spool to the holder
  FULL_TARE3_PAGE = 2403, // Weigh the full spool, predict spool from weight
  FULL_TARE4_PAGE = 2404, // Save predicted spool weight 
  FULL_TARE5_PAGE = 2405, // Save predicted spool weight 
  NEW_PROFILE_PAGE = 3001,
  NO_MORE_SPACE_AVAILABLE_PAGE = 3012,
  OPTIONS_PAGE = 3201,
  DEADZONE_PAGE = 3210,
  FACTORY_RESET_PAGE = 3310,
  FACTORY_RESET2_PAGE = 3311,
  FULL_CALIBRATION_PAGE = 4001,
  WEIGH_FULL_SPOOL_EXTERNAL_PAGE = 4011,
  ENTER_WEIGHT_PAGE = 4021,
  PLACE_FULL_SPOOL_PAGE = 4031,
  WEIGH_FULL_SPOOL_PAGE = 4040, // saves the weight of the full spool
  REMOVE_SPOOL_PAGE = 4051,
  WEIGH_EMPTY_SCALE_PAGE = 4060,
  CALIBRATION_COMPLETE_PAGE = 4071
};

// Void to be able to reset arduino:
void(* resetFunc) (void) = 0;//declare reset function at address 0

UserProfile * getCurrentProfile()
{
  // DEBUGS("Getting profile");
  // DEBUGV(config.sequenceOfCurrentProfileSlot);
  // DEBUGV(config.userProfileSlotOrderSequence[config.sequenceOfCurrentProfileSlot]);
  // DEBUGV(config.profiles[config.userProfileSlotOrderSequence[config.sequenceOfCurrentProfileSlot]].name);

  return &config.profiles[config.userProfileSlotOrderSequence[config.sequenceOfCurrentProfileSlot]];
}

int checkAvailableProfileSlots()
{
  for(int i = 0; i < NUM_PROFILE_SLOTS; i++)
  {
    if(config.profiles[i].emptySpoolMeasuredWeight == -1)
    {
      DEBUGS("Slot found");
      return i;
    }
  }
  DEBUGS("No slot found");
  return -1;
}

int countValidProfiles()
{
  int count = 0;
  for(int i = 0; i < NUM_PROFILE_SLOTS; i++)
  {
    if(config.profiles[i].emptySpoolMeasuredWeight != -1)
    {
      count++;
    }
  }
  return count;
}

// Function to insert a number at a specified position in the array
void moveValueWithinArray( int value, int newPosition, int * array, int array_len)
{
    assert(newPosition < array_len);
    // Remove the value from its current position
    int curr_pos;
    for (curr_pos = 0; curr_pos < array_len; curr_pos++) {
        if (array[curr_pos] == value) {
            break;
        }
    };

    // Remove the value by moving the rest of the data
    memmove(&array[curr_pos], &array[curr_pos + 1], (array_len - curr_pos - 1) * sizeof(int));
    // printSeq();

    // Make room for the value at the new position
    memmove(&array[newPosition + 1], &array[newPosition], (array_len - newPosition - 1) * sizeof(int));

    // Insert the value at the new position
    array[newPosition] = value;
}

void insertSlotAtSequence( int value, int insertPosition)
{
  moveValueWithinArray( value, insertPosition, &config.userProfileSlotOrderSequence[0], NUM_PROFILE_SLOTS);
}

char selectorChar(int position)
{
  if(position<0 || position >= 44)
  {
    return(' ');
  }
  else
  {
    return ProfileChars[position];
  }
}

int nextClampedAndLooped(int value, int direction, int upper, int lower=0)
{
  value += direction;
  if(value < lower)
  {
    value = upper;
  }
  else if(value > upper)
  {
    value = lower;
  }
  return value;
}

int nextActiveSlot(int direction)
{
  int startingSlot = config.sequenceOfCurrentProfileSlot;
  // Go to previous profile:
  do
  {
    config.sequenceOfCurrentProfileSlot = nextClampedAndLooped(config.sequenceOfCurrentProfileSlot, direction, NUM_PROFILE_SLOTS - 1, 0);
    if(config.sequenceOfCurrentProfileSlot == startingSlot)
    {
      break; // avoid infinite loop
    }
    // emptySpoolMeasuredWeight = -1 means that the slot is unused, so we skip.
  }while(config.profiles[config.userProfileSlotOrderSequence[config.sequenceOfCurrentProfileSlot]].emptySpoolMeasuredWeight == -1);
  return config.sequenceOfCurrentProfileSlot;
} 

// Define a struct for button state and debounce data
struct ButtonData {
  int buttonPin;
  int debouncingCounter;
  int debouncingLastState;
  int activationStateWithDebounce;
  int holdCounter;
  int holdRepeatCounter;
  int activationStatePulses;
  int repeatFrequencyCounter;
  int activationStateLongHold;
  int onePulseOnly;
};

// Button initialization:
ButtonData leftButtonData   = {LEFT_BUTTON_PIN, 0, 0, 0, 0, 0, 0, 0};
ButtonData enterButtonData  = {ENTER_BUTTON_PIN, 0, 0, 0, 0, 0, 0, 0};
ButtonData rightButtonData  = {RIGHT_BUTTON_PIN, 0, 0, 0, 0, 0, 0, 0};

void debounce(struct ButtonData& buttonData) 
{
  // DEBUGS("Debounce");
  int currentState = digitalRead(buttonData.buttonPin);

  // DEBUGV(currentState);
  if (currentState == HIGH) {
    buttonData.debouncingCounter = 0;
    buttonData.debouncingLastState = 0;
    buttonData.activationStateWithDebounce = 0;
  }
  else 
  {
    buttonData.debouncingCounter++;

    if (buttonData.debouncingCounter >= ButtonDebouncingAmount) 
    {
      buttonData.activationStateWithDebounce = (buttonData.debouncingLastState == 0) ? 1 : 0;
      buttonData.debouncingLastState = currentState;
    }
  }
  // DEBUGS("~Debounce");
}

// Function to handle button actions
void handleButton(struct ButtonData &buttonData, int pulseLimit, int repeatLimit, int frequencyDelay) 
{
  // DEBUGS("handle button - enter");
  debounce(buttonData);
  // If debouncing state of the button says we are pressing the button:
  if (buttonData.activationStateWithDebounce == 1) {
    buttonData.holdRepeatCounter++;  // Increase counter to know for how long we are pressing the button

    // Do only one pulse:
    if (buttonData.holdCounter < 1) {
      buttonData.activationStatePulses = 1;  // Write state of the button as active
      buttonData.onePulseOnly = 1;
    } else {
      buttonData.activationStatePulses = 0;  // Write state of the button as inactive
      buttonData.onePulseOnly = 0;  // Store that we already went through a pulse, so now we are in the off position
    }
    buttonData.holdCounter = 1;  // Record that we already executed the action for one pulse, to prevent from executing again

    // Repeat when long-holding button:
    if (buttonData.holdRepeatCounter > pulseLimit) {
      buttonData.holdRepeatCounter = pulseLimit + 1;  // Prevents the counter from going indefinitely up
      buttonData.activationStateLongHold = 1;  // Record that we are long-holding the button

      // Create repeat pulses:
      if (buttonData.repeatFrequencyCounter == 0) {
        // Put whatever you want to do when we hold the button and a repeat pulse was created:
      }
      if (buttonData.repeatFrequencyCounter > frequencyDelay) {
        buttonData.activationStatePulses = 1;  // Execute button action
        buttonData.repeatFrequencyCounter = 0;  // Reset counter that counts the pulse delay to restart the cycle of waiting
      } else {
        buttonData.repeatFrequencyCounter++;  // Increase counter to keep waiting
      }
    }
  } else {  // If debouncing state of the button says we release the button:
    buttonData.holdCounter = 0;  // Record that we release the button
    buttonData.holdRepeatCounter = 0;  // Reset the counter that counts for how long we are pressing the button
    buttonData.activationStatePulses = 0;  // Write state of the button as inactive
    buttonData.repeatFrequencyCounter = 0;  // Reset counter that counts the pulse delay for the repeat when holding
    buttonData.activationStateLongHold = 0;  // Record that we are not longer long-holding the button
    buttonData.onePulseOnly = 0;  // Reset the one pulse only as release
  }
  // DEBUGS("handle button - exit");
}
int getDirectionFromKeys()
{
  int direction =0;
  if(leftButtonData.activationStatePulses == 1)  // If left button is press
  {
    direction = -1;  // Go to the left
  }
  if(rightButtonData.activationStatePulses == 1)  // If right button is press
  {
    direction = 1;  // Go to the right
  }
  return direction;
}

void ResetSmoothing()  // Start of reseting smoothing and deadzone variables to start calculating from scratch:
{ 
  DEBUGS("ResetSmoothing");
  // Match the correct Profile number to the Profile slot:
  currentProfilePtr = getCurrentProfile();
  DEBUGS("Got profile");

    // To prevent the smoothing process from starting from 0, we set the smoothing variables to store the current cell values:

  // Read scale sensor:
  if(scale.is_ready())
  {
    // do nothing for now
  }
  else
  {
    DEBUGS("Scale sensor not ready");
    return;
  }
  
  RawLoadCellReading = scale.read();
  DEBUGS("got raw cell reading");

  RawLoadCellReading = RawLoadCellReading /10;  // Remove one digit from the raw reading to reduce the value and could be handle by the map function

  // Remap to convert Raw Values into grams:
  // this effectively takes the full sppool raw and the empty holder raw, as the raw range and maps those to 0-"given weight", thus establishing a ratio of raw to real.
  DEBUGV(RawLoadCellReading);
  DEBUGV(config.emptyHolderRaw);
  DEBUGV(config.fullSpoolRaw);
  DEBUGV(config.fullSpoolWeight);
  HolderWeight = map(RawLoadCellReading, config.emptyHolderRaw, config.fullSpoolRaw, 0, config.fullSpoolWeight); 
  DEBUGV(HolderWeight);
  HolderWeight = constrain(HolderWeight, 0, 9999);  // limits value between min and max to prevent going over limits
  DEBUGV(currentProfilePtr->emptySpoolMeasuredWeight);    
  HolderWeight = HolderWeight - currentProfilePtr->emptySpoolMeasuredWeight;  // Remove the weight of the empty spool

  // Prevent going under 0:
  if(HolderWeight < 0)
  {
    HolderWeight = 0;
  }
  
  readIndex1 = 0;  // Reset array number
  while(readIndex1 < numReadings1)
  {
    readings1[readIndex1] = HolderWeight;  // Records the current value so the smoothing doesn't start from 0
    readIndex1 ++;  // Increase array number to write the following array
  }  // End of while

  total1 = HolderWeight * numReadings1;  // Multiply the current value with the amount of samples/arrays
                                                       // so the smoothing doesn't start from 0.
  readIndex1 = 0;  // Reset array number
  
  // I want the final value to start at the correct value considering the deadzone and to be
  // at the lower edge of the deadzone so I do the following:
  SmoothedWeight = HolderWeight;
  DEBUGV(SmoothedWeight);    
  WeightWithDeadzone = HolderWeight + config.deadzoneAmount;  
  DEBUGV(WeightWithDeadzone);    
    // When i boot-up arduino, I want the output to show the
    // lower edge of the deadzone so the value is close to what it would be in normal operation.
    // I manually used to do that every time I boot-up arduino by pressing more weight into
    // the holder and it would stabilize in the edge of the dead zone.
    // To do this automaticly I added this line in the setup.
    // Basically the first value is going to be the current real value of the scale.
  DEBUGS("Smoothing Reset - Complete");
}  // End of reseting smoothing and deadzone variables to start calculating from scratch

void readSmoothedWeight()
{
  // Read load cell only if it's available. This is so the loop run faster
  if(scale.is_ready())
  {
    RawLoadCellReading = scale.read();
    RawLoadCellReading = RawLoadCellReading /10;  // Remove one digit from the raw reading to reduce the value and could be handle by the map function
  }
  else
  {
    return;
  }

  // Remap to convert Raw Values into grams:
  HolderWeight = map (RawLoadCellReading, config.emptyHolderRaw, config.fullSpoolRaw, 0, config.fullSpoolWeight);  // Remaps temperature IN to color value
  HolderWeight = constrain(HolderWeight, 0, 9999);  // limits value between min and max to prevent going over limits

  // Add to the measurement the weight of the empty spool in that profile so we measure the weight of the filament only:
  CurrentProfileWeightShown = HolderWeight - currentProfilePtr->emptySpoolMeasuredWeight;
  CurrentProfileWeightShown = constrain(CurrentProfileWeightShown, 0, 9999);  // limits value between min and max to prevent going over limits

  // Smooth Weight:
  // Smoothing process:
  // Subtract the last reading:
  total1 = total1 - readings1[readIndex1];
  // Read value:
  readings1[readIndex1] = CurrentProfileWeightShown;
  // Add the reading to the total:
  total1 = total1 + readings1[readIndex1];
  // Advance to the next position in the array:
  readIndex1 = readIndex1 + 1;

  // if we're at the end of the array...
  if (readIndex1 >= numReadings1)
  {
    readIndex1 = 0;    // Wrap around to the beginning
  }

  // Calculate the average:
  SmoothedWeight = total1 / numReadings1;
  DEBUGV(SmoothedWeight);
  // End of smoothing process  
}
int strlen_trimmed(const char *s) 
{
  // returns length of s, not including nul terminator or trailing spaces.
  int stringLen = strlen(s);  // Get the length of the title
  while(s[stringLen-1] == ' ')
  {
    stringLen--;
  }  
  return stringLen;
}

void addLetterToProfileName(char letter) {
  DEBUGV(letter);
  tempProfileTitleBuffer[EditTextCursorPosition] = letter;
  DEBUGV(tempProfileTitleBuffer);
  // Increase the amount of characters in the profile name
  // if we are at the edge of the set amount. Also, prevent going
  // over the limit for the amount of characters:
  int titleLength = strlen_trimmed(tempProfileTitleBuffer);
  if (EditTextCursorPosition == titleLength -1 && titleLength < MAX_PROFILE_NAME_LEN) 
  {
    tempProfileTitleBuffer[EditTextCursorPosition + 1] = ' ';  // Set the next character as SPACE to clear whatever is recorded there
  }
  if(EditTextCursorPosition < MAX_PROFILE_NAME_LEN-1)
  {
    EditTextCursorPosition++;
  }
}
void removeLetterFromProfileName() {
  int titleLength = strlen_trimmed(tempProfileTitleBuffer);
  DEBUGV(titleLength);
  DEBUGV(EditTextCursorPosition);
  DEBUGV(tempProfileTitleBuffer);
  // if we are in the normal range 
  // or we are at the end but the final charater is blank
  if ( ((EditTextCursorPosition > 0 && titleLength >0) && 
        (EditTextCursorPosition < MAX_PROFILE_NAME_LEN-1 )) 
      || 
       ((EditTextCursorPosition == MAX_PROFILE_NAME_LEN-1) &&
        (tempProfileTitleBuffer[EditTextCursorPosition] == ' ')))
  {
    DEBUGS("memcpy");
    memcpy(tempProfileTitleBuffer + EditTextCursorPosition - 1,
           tempProfileTitleBuffer + EditTextCursorPosition,
           titleLength - EditTextCursorPosition + 1);
  }
  else if (EditTextCursorPosition >= MAX_PROFILE_NAME_LEN-1)
  {
    DEBUGS("inplace");
    tempProfileTitleBuffer[MAX_PROFILE_NAME_LEN-1] = ' ';
  }
  if( ( EditTextCursorPosition > 0 
        && EditTextCursorPosition < MAX_PROFILE_NAME_LEN-1 )
      || ( EditTextCursorPosition == MAX_PROFILE_NAME_LEN-1 
        && tempProfileTitleBuffer[EditTextCursorPosition-1]==' ' ))
  {
    DEBUGS("move cursor");
    EditTextCursorPosition--;
  }
}

void resetSequence()
{
    for(int i=0; i<NUM_PROFILE_SLOTS; i++)
    {
      config.userProfileSlotOrderSequence[i]=i;
      DEBUGV(i);
      DEBUGV(config.userProfileSlotOrderSequence[i]);
    }
}
void clearProfileName(UserProfile * prof)
{
  memset(prof->name, '\0', MAX_PROFILE_NAME_LEN+1); // memset includes the trailing nul too
}

void clearTempProfileName()
{
  memset(tempProfileTitleBuffer, '\0', MAX_PROFILE_NAME_LEN+1); // memset includes the trailing nul too
}
void resetProfiles()
{
    for(int i=0; i<NUM_PROFILE_SLOTS; i++)
    {
      UserProfile * prof = &config.profiles[i];
      prof->emptySpoolMeasuredWeight=-1;
      clearProfileName(prof);
    }
}

void setProfileToDefault(int profileNumber)
{
  UserProfile * prof = &config.profiles[profileNumber];
  strcpy(prof->name, "Default");
  prof->emptySpoolMeasuredWeight=177; // Set the weight of the empty spool to 177 grams
}

void setup()  // Start of setup
{
#ifdef DEBUG
// we only need the serial port open in debug mode, so we don't need to open it in normal mode.
  Serial.begin(115200); // Any baud rate should work
#endif
  DEBUGS("setup - BEGIN");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);  // Start scale object and assign the pins

  // Set pins as inputs or outputs:
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);  // Set pin for LEFT button to input and activate pullup resistor
  pinMode(ENTER_BUTTON_PIN, INPUT_PULLUP);  // Set pin for Enter button to input and activate pullup resistor
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);  // Set pin for RIGHT button to input and activate pullup resistor

  
  // Read EEPROM:
  readConfigurationFromEEPROM();
  if( config.calibrationDone != 1 )
  {
    DEBUGS("Uncalibrated - resetting profiles");
    // No calibration so this is a new start, wipe the profiles
    resetSequence();
    resetProfiles();
    setProfileToDefault(0);
    config.sequenceOfCurrentProfileSlot=0;
  }

  // Check if the EEPROM value is valid. If not, it means it's in the default so we need to set
  // a valid default value. This is mostly when using a new arduino or if we did a factory reset
  if(config.deadzoneAmount > 50)  // if the EEPROM that stores the deadzone set amount is above the limit
  {
    config.deadzoneAmount = 0;  // Store in the variable the deadzone set amount
  }

  // Check if the EEPROM for user profile order sequence in slot 0 is over the limit
  // to see if it's valid or not. If it's not valid, it should write the default values:
  if(config.userProfileSlotOrderSequence[0]>NUM_PROFILE_SLOTS-1)
  {
    resetSequence();
  }

  // If the profile slot number is invalid, it means there is no profile saved so go to default values:
  if(config.sequenceOfCurrentProfileSlot > 19)
  {
    config.sequenceOfCurrentProfileSlot = 0;
    // Match the correct Profile number to the Profile slot:
    currentProfilePtr = getCurrentProfile();
    strcpy(currentProfilePtr->name, "No profile"); // this will include the nul terminator
  }

  leftButtonData   = {LEFT_BUTTON_PIN, 0, 0, 0, 0, 0, 0, 0};
  enterButtonData  = {ENTER_BUTTON_PIN, 0, 0, 0, 0, 0, 0, 0};
  rightButtonData  = {RIGHT_BUTTON_PIN, 0, 0, 0, 0, 0, 0, 0};

  // DEBUGS("reset Smoothing");
  ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
  // DEBUGS("reset Smoothing - Done");
  

  // If Full Calibration has been done:
  // Change the initial page depending on if we ever did the full calibration:
  DEBUGV(config.calibrationDone);
  if(config.calibrationDone == 1)  // If we have done the calibration before
  {
    setScreenPage(NORMAL_PAGE);  // Put page in normal
  }
  else
  {
    setScreenPage(START_PAGE);
  }
  // If the value is any other value other than 1, it means we never had done the
  // full calibration so it will stay in ScreenPage 0 (welcome screen)
  writeConfigurationToEEPROM();
  DEBUGS("STEUP: writing Config - DONE");
  u8g.begin();
   // End of setup
}

void loop()  // Start of loop
{
  int lastScreenPage = -1;
  readConfigurationFromEEPROM();

  readSmoothedWeight();

  /////////////
  // Buttons //
  /////////////
  // DEBUGS("Button handling");
  // DEBUGV(RepeatWhenWeHoldAmount);
  // DEBUGV(RepeatWhenWeHoldFrequencyDelay);
  // DEBUGS("Handling Enter");
  handleButton(enterButtonData, 1, RepeatWhenWeHoldAmount, RepeatWhenWeHoldFrequencyDelay);
  // DEBUGS("Handling Right");
  handleButton(rightButtonData, 1, RepeatWhenWeHoldAmount, RepeatWhenWeHoldFrequencyDelay);
  // DEBUGS("Handling Left");
  handleButton(leftButtonData, 1, RepeatWhenWeHoldAmount, RepeatWhenWeHoldFrequencyDelay);


  //////////////////////////////////////////
  // Button Actions depending on the page //
  //////////////////////////////////////////

  //*******Page index:
  // 0    = Welcome screen recommending starting full calibration
  // 100  = Normal screen showing profile name and weight
  // 1001 = Main menu
  // 2101 = Main menu > Edit profile menu
  // 2111 = Main menu > Edit profile > Tare 1: Place in the holder the empty spool
  // 2120 = Main menu > Edit profile > Tare 2: Saving the weight
  // 2131 = Main menu > Edit profile > Tare 3: Weight saved, with selection on OK
  // 2401 = Main menu > Edit profile > FullTare 1: Enter the filament weight
  // 2402 = Main menu > Edit profile > FullTare 2: Place in the holder the empty spool
  // 2403 = Main menu > Edit profile > FullTare 3: Saving the weight, estimating spool weight
  // 2404 = Main menu > Edit profile > FullTare 4: Spool weight saved, with selection on OK
  // 2201 = Main menu > Edit profile > Edit name
  // 2301 = Main menu > Edit profile > Delete profile, with selection on CANCEL
  // 2311 = Main menu > Edit profile > Profile has been deleted, with selection on OK
  // 2312 = Main menu > Edit profile > Delete profile > Only 1 profile left so can't delete
  // 3001 = Main menu > New profile
  // 3012 = Main menu > New profile > No more space available
  // 3201 = Main menu > Options
  // 3210 = Main menu > Options > Deadzone
  // 4001 = Main menu > Options > Full calibration intro, with option on CONTINUE
  // 4011 = Main menu > Options > Full calibration > 2. Place full spool in real scale, with option on CONTINUE
  // 4021 = Main menu > Options > Full calibration > 3. Enter the weight, with option on SAVE
  // 4031 = Main menu > Options > Full calibration > 4. Put full spool on holder, with option on CONTINUE
  // 4040 = Main menu > Options > Full calibration > 5. Saving the weight of the full spool
  // 4051 = Main menu > Options > Full calibration > 6. Remove spool, with selection on CONTINUE
  // 4060 = Main menu > Options > Full calibration > 7. Saving weight of empty holder
  // 4071 = Main menu > Options > Full calibration > 8. Calibration has been completed, with selection on OK
  // 3310 = Main menu > Options > FACTORY RESET
  // 3311 = Main menu > Options > FACTORY RESET 2

  int direction = 0;
  direction = getDirectionFromKeys();
  // DEBUGS("Processing input for Screen ");
  if(lastScreenPage!= ScreenPage)
  {
    // screen has changed
    DEBUGV(ScreenPage);
  }
  lastScreenPage = ScreenPage;

  switch(ScreenPage)
  {
    case START_PAGE:  // If we are in page 0
    {
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 1, 0);
      // DEBUGV(SelectorPosition);
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        // DEBUGS("ENTER_BUTTON - pressed");
        if(SelectorPosition == 0)  // If selector is in START
        {
          setScreenPage(FULL_CALIBRATION_PAGE);  // Go to page full calibration
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest 
        }
        else if(SelectorPosition == 1)  // If selector is in NO
        {
          ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
          setScreenPage(NORMAL_PAGE);  // Go to page normal
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest 
        }
      }
    }  // End of if we are in page 0
    break;
    case NORMAL_PAGE:  // If we are in page "normal"
    {
      // left/right changes profile
      nextActiveSlot(direction);  // Go to the next slot

      // ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch

      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        setScreenPage(MAIN_MENU_PAGE);  // Go to page main menu
        SelectorPosition = 0;  // Reset selector position to 0
      }
    }  // End of if we are in page normal
    break;
    case MAIN_MENU_PAGE:  // If we are in page main menu
    {
      // DEBUGS("MAIN_MENU_PAGE");
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 3, 0);  // Go to the next slot
  
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        // DEBUGS("ENTER_PRESSED");
        DEBUGV(SelectorPosition);
        if(SelectorPosition == 0)  // If selector is in EDIT THIS PROFILE
        {
          // DEBUGS("EDIT SELECTED");
          setScreenPage(EDIT_PROFILE_PAGE);  // Go to page EDIT THIS PROFILE
          SelectorPosition = 0;  // Reset selector position to 0
          break;
        }
        if(SelectorPosition == 1)  // If selector is in ADD NEW PROFILE
        {
          // DEBUGS("NEW SELECTED");
          // DEBUGS("CAN WE GO TO NEW PROFILE");
          // Check if there is profile sectors available:
          int tempProfileSlot = checkAvailableProfileSlots();
          DEBUGV(tempProfileSlot);
          if( tempProfileSlot >= 0)  // If there are profile sectors available
          {
            // DEBUGS("YES");
            // Profile user slot:
            // Check the next profile user slot available.
            // If the next slot is already in use, we should move it forward to make space
            // we know there is space so all we need to do is move them all down by one using memmove
            insertSlotAtSequence(tempProfileSlot, config.sequenceOfCurrentProfileSlot + 1);
            config.sequenceOfCurrentProfileSlot += 1;
            setScreenPage(NEW_PROFILE_PAGE);  // Go to page new profile
            SelectorPosition = 0;  // Reset selector position to 0
            // update our ptr to match the new selection:
            currentProfilePtr = getCurrentProfile();
            clearProfileName(currentProfilePtr);
            clearTempProfileName();
            EditTextCursorPosition = 0;
            // Write the default empty spool weight:
            currentProfilePtr->emptySpoolMeasuredWeight = 0;
            break;
          }  // End of if there are profile sectors available
          else  // If there are not profile sectors available
          {
            // DEBUGS("NO");
            setScreenPage(NO_MORE_SPACE_AVAILABLE_PAGE);  // Go to page "no more space available"
            SelectorPosition = 0;  // Reset selector position to 0
            break;  // Skip the rest part of the code to check for button presses
          }  // End of if there are not profile sectors available
        }  // End of if selector is in ADD NEW PROFILE
        if(SelectorPosition == 2)  // If selector is in OPTIONS
        {
          // DEBUGS("OPTIONS SELECTED");
          setScreenPage(OPTIONS_PAGE);  // Go to page OPTIONS
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
        if(SelectorPosition == 3)  // If selector is in GO BACK
        {
          // DEBUGS("BACK SELECTED");
          ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
          setScreenPage(NORMAL_PAGE);  // Go to page normal
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
      }  // End of if Enter button is press
    }  // End of if we are in page main menu
    break;
    case EDIT_PROFILE_PAGE:  
    {
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 3, 0);
  
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        switch(SelectorPosition)
        {
          case 0:  // If selector is in Tare
          {
            setScreenPage(TARE0_PAGE);  // Go to page  Tare 1
            SelectorPosition = 0;  // Reset selector position to 0
          }
          break;
          case 1:  // If selector is in EDIT PROFILE NAME
          {
            setScreenPage(EDIT_PROFILE_NAME_PAGE);  // Go to page EDIT PROFILE NAME
            strcpy(tempProfileTitleBuffer,currentProfilePtr->name); // pre-populate with current title
            EditTextCursorPosition=strlen(currentProfilePtr->name);
            SelectorPosition = 0;  // Reset selector position to 0
          }
          break;
          case 2:  // If selector is in DELETE THIS PROFILE
          {
            // Check if we have more than 1 profile:
            // This is because if we have only 1 profile, we are not going to allowed.
            if(countValidProfiles() > 1)  // If there are more than 1 profile recorded
            {
              setScreenPage(DELETE_PROFILE_PAGE);  // Go to page DELETE THIS PROFILE
              SelectorPosition = 0;  // Reset selector position to 0
              break;  // Skip the rest part of the code to check for button presses
            }
            else
            {
              setScreenPage(PROFILE_NOT_DELETED_PAGE);  // Go to page only one profile left so you can't delete it
              SelectorPosition = 0;  // Reset selector position to 0
              break;  // Skip the rest part of the code to check for button presses
            }
          }
          break;
          case 3:  // If selector is in GO BACK
          {
            setScreenPage(MAIN_MENU_PAGE);  // Go to main menu
            SelectorPosition = 0;  // Reset selector position to 0
            break;  // Skip the rest part of the code to check for button presses
          }
          break;
        }  // End SWITCH ON SELECTOR
      }  // End If enter pressed
    }  // End of if we are in page EDIT PROFILE MENU
    break;
    case TARE0_PAGE:  // If we are in page Tare 0
    {
      // Left and right buttons:
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 2, 0);
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        if(SelectorPosition == 0)  // If selector is in Empty spool
        {
          setScreenPage(TARE1_PAGE);  // Go to page Tare 2
          SelectorPosition = 0;  // Reset selector position to 0
          break;
        }
        if(SelectorPosition == 1)  // If selector is in CANCEL
        {
          setScreenPage(ENTER_FILAMENT_WEIGHT_PAGE);  // Go to page normal
          currentProfilePtr->fullSpoolFilamentWeight=1000;
          SelectorPosition = 0;  // Reset selector position to 0
          break;
        }
        if(SelectorPosition == 2)  // If selector is in CANCEL
        {
          ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
          setScreenPage(NORMAL_PAGE);  // Go to page normal
          SelectorPosition = 0;  // Reset selector position to 0
          break;
        }
      }  // End of if Enter button is press
    }  // End of if we are in page Tare 1
    break;
    case TARE1_PAGE:  // If we are in page Tare 1
    {
      // Left and right buttons:
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 1, 0);
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        if(SelectorPosition == 0)  // If selector is in CONTINUE
        {
          setScreenPage(TARE2_PAGE);  // Go to page Tare 2
          SelectorPosition = 0;  // Reset selector position to 0
          break;
        }
        if(SelectorPosition == 1)  // If selector is in CANCEL
        {
          ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
          setScreenPage(NORMAL_PAGE);  // Go to page normal
          SelectorPosition = 0;  // Reset selector position to 0
          break;
        }
      }  // End of if Enter button is press
    }  // End of if we are in page Tare 1
    break;
    case FULL_TARE5_PAGE:  
    {
      // Left and right buttons:
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 1, 0);
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        if(SelectorPosition == 0)  // If selector is in CANCEL
        {
          setScreenPage(TARE0_PAGE);  // Go back to page Tare page
          SelectorPosition = 0;  // Reset selector position to 0
          break;
        }
        if(SelectorPosition == 1)  // If selector is in RETRY
        {
          ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
          setScreenPage(ENTER_FILAMENT_WEIGHT_PAGE);  // Go to page normal
          SelectorPosition = 0;  // Reset selector position to 0
          break;
        }
      }  // End of if Enter button is press
    }  // End of if we are in page Tare 1
    break;
    case EDIT_PROFILE_NAME_PAGE:  // If we are in page EDIT PROFILE
    case NEW_PROFILE_PAGE:  // If we are in page NEW PROFILE
    {
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 47, 0);
      // Enter button:
      if(enterButtonData.activationStatePulses == 1)  // If Enter button is press, inclusing long holding for fast repeat
      {
        // Here's the table of values that correspond to the selection:
        // 0  = A
        // 1  = B
        // 2  = C
        // 3  = D
        // 4  = E
        // 5  = F
        // 6  = G
        // 7  = H
        // 8  = I
        // 9  = J
        // 10 = K
        // 11 = L
        // 12 = M
        // 13 = N
        // 14 = O
        // 15 = P
        // 16 = Q
        // 17 = R
        // 18 = S
        // 19 = T
        // 20 = U
        // 21 = V
        // 22 = W
        // 23 = X
        // 24 = Y
        // 25 = Z
        // 26 SYMBOL1 = &
        // 27 SYMBOL2 = '
        // 28 SYMBOL3 = *
        // 29 SYMBOL4 = +
        // 30 SYMBOL5 = ,
        // 31 SYMBOL6 = -
        // 32 SYMBOL7 = .
        // 33 SYMBOL8 = /
        // 34 NUMBER0 = 0
        // 35 NUMBER1 = 1
        // 36 NUMBER2 = 2
        // 37 NUMBER3 = 3
        // 38 NUMBER4 = 4
        // 39 NUMBER5 = 5
        // 40 NUMBER6 = 6
        // 41 NUMBER7 = 7
        // 42 NUMBER8 = 8
        // 43 NUMBER9 = 9
        // 44 OPTION1 = [SPACE]
        // 45 OPTION3 = [BACKSPACE]
        // 46 OPTION4 = [CANCEL]
        // 47 OPTION5 = [SAVE]
        if(SelectorPosition < 44)
        {
          addLetterToProfileName(selectorChar(SelectorPosition));
          break;
        }
        if(SelectorPosition == 44)  // If selector is in [SPACE]
        {
          addLetterToProfileName(' ');
          break;
        }
        if(SelectorPosition == 45)  // If selector is in [BACKSPACE]
        {
          if(EditTextCursorPosition > 0)  // If the text cursor position is 1 or more
          {
            removeLetterFromProfileName();  // Remove the character at the cursor position
          }  // End of if the text cursor position is above the minimum
          break;
        }
        if(SelectorPosition == 46)  // If selector is in CANCEL
        {
          // If we are creating a new profile, when press CANCEL it should delete the profile to cancel operation:
          if( ScreenPage == NEW_PROFILE_PAGE )  // If page is on ADD NEW PROFILE
          {
            currentProfilePtr->emptySpoolMeasuredWeight = -1;

            if(countValidProfiles() > 1)  // If there is more than one profile
            {
              config.sequenceOfCurrentProfileSlot = nextActiveSlot(-1);
            }
          }
          // Match the correct Profile number to the Profile slot:
          currentProfilePtr = getCurrentProfile();
    
          ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
        
          setScreenPage(NORMAL_PAGE);  // Go to normal screen
          SelectorPosition = 0;  // Reset selector position to 0
          break;          // End of delete section
        }
        
        if(SelectorPosition == 47)  // If selector is in SAVE
        {
          // Before recording the amount of characters, we need to consider that
          // If the final character is an empty space, we should ignore that and
          // not count it as a character. Because of this, we check if the final
          // character is a SPACE and remove 1 from the AmountOfCharacters.
          int titleLen = strlen(tempProfileTitleBuffer);  // Get the length of the title
          while(tempProfileTitleBuffer[titleLen -1] == ' ' && titleLen > 1)  // If last character is a SPACE, and go back and check again until the last digit is not SPACE. Also the amount of characters has to be more than 1 to avoid going to 0
          {
            tempProfileTitleBuffer[titleLen -1] = '\0'; // set a trailing nul byte
            titleLen = titleLen -1;  // Remove 1 from amount of characters
          }
          currentProfilePtr = getCurrentProfile();
          strcpy(currentProfilePtr->name, tempProfileTitleBuffer);
          
          // Before setting which page to go, we need to check if we are editing a profile
          // or creating a new profile. If we are creating a new profile, I want to go to
          // Tare.
          if( ScreenPage == EDIT_PROFILE_NAME_PAGE )  // If we are in page edit profile name (from edit profile)
          {
            ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
            setScreenPage(NORMAL_PAGE);  // Go to normal screen
          }
          else if( ScreenPage == NEW_PROFILE_PAGE )  // If we are in page new profile
          {
            ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
            setScreenPage(TARE0_PAGE);  // Go to page Tare 0
          }
          
          SelectorPosition = 0;  // Reset selector position to 0
          break;
        }  // End of if selector is in SAVE
      }  // End of if Enter button is press
    }  // End of if we are in page EDIT PROFILE NAME
    break;
    case DELETE_PROFILE_PAGE:  // If we are in page Delete profile
    {
      // Left and right buttons:
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 1, 0);
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        if(SelectorPosition == 0)  // If selector is in CANCEL
        {
          ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
          setScreenPage(NORMAL_PAGE);  // Go to page normal
          SelectorPosition = 0;  // Reset selector position to 0
          break;  
        }
        if(SelectorPosition == 1)  // If selector is in DELETE
        {
          currentProfilePtr->emptySpoolMeasuredWeight = -1;
          if(countValidProfiles() > 1)  // If there is more than one profile
          {
            config.sequenceOfCurrentProfileSlot = nextActiveSlot(-1);
          }
          // Match the correct Profile number to the Profile slot:
          currentProfilePtr = getCurrentProfile();
          ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
          setScreenPage(PROFILE_DELETED_PAGE);  // Go to page Profile has been deleted
          SelectorPosition = 0;  // Reset selector position to 0
          break;
        }  // End of if selector is in CONTINUE
      }  // End of if Enter button is press
    }  // End of if we are in page Delete profile
    // We use the same code for multiple pages because all this pages do the same of
    // if you press enter, go to the page normal:
    case PROFILE_DELETED_PAGE:  // If we are in page Profile has been deleted
    case PROFILE_NOT_DELETED_PAGE:
    case NO_MORE_SPACE_AVAILABLE_PAGE:
    case TARE3_PAGE:
    case FULL_TARE4_PAGE:
    case CALIBRATION_COMPLETE_PAGE:
    {
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
        setScreenPage(NORMAL_PAGE);  // Go to page normal
        SelectorPosition = 0;  // Reset selector position to 0
      }  // End of if Enter button is press
    }  // End of "page with OK return to NORMAL"
    break;
    case OPTIONS_PAGE:  // If we are in page OPTIONS
    {
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 3, 0);
        // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        if(SelectorPosition == 0)  // If selector is in DEADZONE
        {
          setScreenPage(DEADZONE_PAGE);  // Go to page DEADZONE
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
        if(SelectorPosition == 1)  // If selector is in FULL CALIBRATION
        {
          setScreenPage(FULL_CALIBRATION_PAGE);  // Go to page FULL CALIBRATION
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
        if(SelectorPosition == 2)  // If selector is in FACTORY RESET
        {
          setScreenPage(FACTORY_RESET_PAGE);  // Go to FACTORY RESET
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }

        if(SelectorPosition == 3)  // If selector is in GO BACK
        {
          setScreenPage(MAIN_MENU_PAGE);  // Go to main menu
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
      }  // End of if Enter button is press
    }  // End of if we are in page OPTIONS
    break;
    case DEADZONE_PAGE:  // If we are in page Deadzone
    {
       config.deadzoneAmount = nextClampedAndLooped(config.deadzoneAmount, direction, 50, 0);
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        // Save in the EEPROM the amount of deadzone:
        ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
        setScreenPage(NORMAL_PAGE);  // Go to page normal
        SelectorPosition = 0;  // Reset selector position to 0
        break;  // Skip the rest part of the code to check for button presses
      }  // End of if Enter button is press
    }  // End of if we are in page Deadzone
    case FULL_CALIBRATION_PAGE:  // If we are in page Full Calibration intro
    {
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 1, 0);
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        if(SelectorPosition == 0)  // If selector is in CONTINUE
        {
          setScreenPage(WEIGH_FULL_SPOOL_EXTERNAL_PAGE);  // Go to page Full Calibration 2
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
        if(SelectorPosition == 1)  // If selector is in CANCEL
        {
          ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
          setScreenPage(NORMAL_PAGE);  // Go to page normal
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
      }  // End of if Enter button is press
    }  // End of if we are in page Full Calibration intro
    break;
    case WEIGH_FULL_SPOOL_EXTERNAL_PAGE:  // If we are in page Full calibration 2
    {
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 1, 0);
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        if(SelectorPosition == 0)  // If selector is in CONTINUE
        {
          setScreenPage(ENTER_WEIGHT_PAGE);  // Go to page Full Calibration 3
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
        if(SelectorPosition == 1)  // If selector is in CANCEL
        {
          setScreenPage(FULL_CALIBRATION_PAGE);  // Go to page Full Calibration intro
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
      }  // End of if Enter button is press
    }  // End of if we are in page Full calibration 2
    break;
    case ENTER_WEIGHT_PAGE:  // If we are in page Full Calibration 3
    {
      config.fullSpoolWeight = nextClampedAndLooped(config.fullSpoolWeight, direction, 3000, 0);
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        setScreenPage(PLACE_FULL_SPOOL_PAGE);  // Go to page Full Calibration 4
        SelectorPosition = 0;  // Reset selector position to 0
        break;  // Skip the rest part of the code to check for button presses
      }  // End of if Enter button is press
    }  // End of if we are in page Full Calibration 3
    break;
    case ENTER_FILAMENT_WEIGHT_PAGE:  // If we are in page Full Calibration 3
    {
      currentProfilePtr->fullSpoolFilamentWeight = nextClampedAndLooped(currentProfilePtr->fullSpoolFilamentWeight, direction, 3000, 0);
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        setScreenPage(FULL_TARE2_PAGE);  // Go to page Full Calibration 4
        SelectorPosition = 0;  // Reset selector position to 0
        break;  // Skip the rest part of the code to check for button presses
      }  // End of if Enter button is press
    }  // End of if we are in page Full Calibration 3
    break;
    
    case FULL_TARE2_PAGE: 
    {
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 1, 0);      
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        if(SelectorPosition == 0)  // If selector is in CONTINUE
        {
          setScreenPage(FULL_TARE3_PAGE);  // Go to page Full Tare 3 to weight it
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
        if(SelectorPosition == 1)  // If selector is in CANCEL
        {
          ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
          setScreenPage(NORMAL_PAGE);  // Go to page normal
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
      }  // End of if Enter button is press
    }  // End of if we are in page Full calibration 4
    case PLACE_FULL_SPOOL_PAGE:  // If we are in page Full calibration 4
    {
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 1, 0);      
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        if(SelectorPosition == 0)  // If selector is in CONTINUE
        {
          setScreenPage(WEIGH_FULL_SPOOL_PAGE);  // Go to page Full Calibration 5
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
        if(SelectorPosition == 1)  // If selector is in CANCEL
        {
          ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
          setScreenPage(NORMAL_PAGE);  // Go to page normal
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
      }  // End of if Enter button is press
    }  // End of if we are in page Full calibration 4
  
    case REMOVE_SPOOL_PAGE:  // If we are in page Full calibration 6
    {
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 1, 0);
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        if(SelectorPosition == 0)  // If selector is in CONTINUE
        {
          setScreenPage(WEIGH_EMPTY_SCALE_PAGE);  // Go to page Full Calibration 7
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
        if(SelectorPosition == 1)  // If selector is in CANCEL
        {
          ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
          setScreenPage(NORMAL_PAGE);  // Go to page normal
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
      }  // End of if Enter button is press
    }  // End of if we are in page Full calibration 6
    break;
    case FACTORY_RESET_PAGE:  // If we are in page Factory Reset
    {
      SelectorPosition = nextClampedAndLooped(SelectorPosition, direction, 1, 0);
      // Enter button:
      if(enterButtonData.onePulseOnly == 1)  // If Enter button is press
      {
        if(SelectorPosition == 0)  // If selector is in CANCEL
        {
          ResetSmoothing();  // Reset smoothing and deadzone variables to start calculating from scratch
          setScreenPage(NORMAL_PAGE);  // Go to page normal
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }

        if(SelectorPosition == 1)  // If selector is in DELETE
        {
          setScreenPage(FACTORY_RESET2_PAGE);  // Go to page Factory Reset 2
          SelectorPosition = 0;  // Reset selector position to 0
          break;  // Skip the rest part of the code to check for button presses
        }
      }  // End of if Enter button is press
    }  // End of if we are in page Factory Reset
    break;
    default:
    {
      // Nothing to do here, it is fine some screens have no input.
    }
  }// End of button excecution of actions
  // Do a few things in Normal page:
  if(ScreenPage == NORMAL_PAGE)  // If we are in page normal
  {
    DEBUGS("NORMAL_PAGE - processing of weight");
    // Weight deadzone:
      WeightWithDeadzone = WeightWithDeadzone + ((SmoothedWeight - WeightWithDeadzone)/(config.deadzoneAmount+1));  // Smooth the value of temp sensor.

    // The way the filament is fed to the extruder is by pulling in quick motions so we get a peak weight
    // and then there's a slack so there's a drop in measured weight. The deadzone itself helps minimizing
    // this effect, but I want the lowest value of the edge of the deadzone so I have to show the weight
    // with deadzone minus the amount of deadzone.
    FinalWeightToShow = WeightWithDeadzone - config.deadzoneAmount;  // Remove the amount of deadzone to the weight 

    // Make sure if weight close to 0, show 0:
    if(SmoothedWeight < MinimumWeightAllowed || FinalWeightToShow < MinimumWeightAllowed)
    {
      FinalWeightToShow = 0;
    }
    DEBUGV(FinalWeightToShow);
   // Reset text cursor:
    EditTextCursorPosition = 0;
  }  // End of if we are in page normal
    
  // Profile title - Transfer from array to final words:
  // Center title text:
  // The X position of the text depends on the amount of characters
  // so we check how many characters the profile title has so we can put the
  // correct X position so the text is centered on the display.

  int len = strlen(currentProfilePtr->name);
  // DEBUGV(len);
  // DEBUGV(currentProfilePtr->name);
  TitleXPosition = 64-(len*3);  // Cursor X position for title

  // Set the text cursor X position based on the cursor character position:
  TextCursorPositionX = EditTextCursorPosition*6;  // Cursor X position for text

  /////////////
  // DISPLAY //
  /////////////

  // Print Display:
  
  u8g.firstPage(); // Beginning of the picture loop
  do  // Include all you want to show on the display:
  {
    // Display - Welcome page: If no full calibration is detected, we show this:
    if(ScreenPage == START_PAGE)  // If page is on welcome page
    {
      // DEBUGS("Display StartPage");
      u8g.setFont(u8g2_font_profont12_tr);  // Set small font
      drawStr(0, 8, TextTheloadcell);  // (x,y,"Text")
      drawStr(0, 18, Textrequirescalibration);  // (x,y,"Text")
      drawStr(0, 28, TextDoyouwanttostart);  // (x,y,"Text")
      drawStr(0, 38, Textthefullcalibration);  // (x,y,"Text")
      drawStr(0, 48, Textprocess);  // (x,y,"Text")

      drawStr(22, 62, TextNO);  // (x,y,"Text")  Correctly centered
      drawStr(86, 62, TextSTART);  // (x,y,"Text")  Correctly centered
      // DEBUGS("Display StartPage - Done");

      // Selection box:
      if(SelectorPosition == 0)
      {
        u8g.drawFrame(SelectBoxXPositionRight, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for START (x,y,width,height)
      }
      if(SelectorPosition == 1)
      {
        u8g.drawFrame(0, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for NO (x,y,width,height)
      }
    }  // End of if page is on welcome page

    // Display - Normal screen
    if(ScreenPage == NORMAL_PAGE)  // If page is on profile normal screen
    {
      // Print profile tittle:
      currentProfilePtr = getCurrentProfile();
      u8g.drawStr(TitleXPosition, 8, currentProfilePtr->name);  // (x,y,"Text")

      // Preparing to print weight:
      // Since the weight X position depends on how big is the value, we need to check
      // and change the X position acordently. This is to print in the center.
      if(FinalWeightToShow < 10000)  // If weight is below 9999
      {
        WeightXPosition = 26;  // Change X position of the weight digits
        gSymbolXPosition = 105;  // Change the X position of the g symbol (5 pixels away from weight)
      }
      if(FinalWeightToShow < 1000)
      {
        WeightXPosition = 36;  // Change X position of the weight digits
        gSymbolXPosition = 96;  // Change the X position of the g symbol (5 pixels away from weight)
      }
      if(FinalWeightToShow < 100)
      {
        WeightXPosition = 45;  // Change X position of the weight digits
        gSymbolXPosition = 86;  // Change the X position of the g symbol (5 pixels away from weight)
      }
      if(FinalWeightToShow < 10)
      {
        WeightXPosition = 54;  // Change X position of the weight digits
        gSymbolXPosition = 76;  // Change the X position of the g symbol (5 pixels away from weight)
      }

      // Print weight:
      u8g.setCursor(WeightXPosition, WeightYPosition);  // (x,y)
      u8g.setFont(u8g2_font_fur25_tn);  // Set big number (only) font
      u8g.print(FinalWeightToShow);  // Print weight

      // Print "g" symbol:
      u8g.setFont(u8g2_font_profont12_tr);  // Set small font
      drawStr(gSymbolXPosition, WeightYPosition, Textg);  // (x,y,"Text")

      // Centering lines:
      // This is only used when testing centering items on the screen
      //u8g.drawLine(63, 0, 63, 63);  // Draw a line (x0,y0,x1,y1) Vertical center
    
      //u8g.drawLine(57, 0, 57, 63);  // Draw a line (x0,y0,x1,y1) 1 char left
      //u8g.drawLine(69, 0, 69, 63);  // Draw a line (x0,y0,x1,y1) 1 char right
      
      //u8g.drawLine(52, 0, 52, 63);  // Draw a line (x0,y0,x1,y1) 2 char left
      //u8g.drawLine(74, 0, 74, 63);  // Draw a line (x0,y0,x1,y1) 2 char right
      
      //u8g.drawLine(0, 0, 0, 63);  // Draw a line (x0,y0,x1,y1) Vertical left
      //u8g.drawLine(127, 0, 127, 63);  // Draw a line (x0,y0,x1,y1) Vertical right
      //u8g.drawLine(0, 0, 127, 0);  // Draw a line (x0,y0,x1,y1) Horizontal top
      //u8g.drawLine(0, 63, 127, 63);  // Draw a line (x0,y0,x1,y1) Horizontal bottom

    }  // End of if we are in normal page





    // Display - Main Menu
    if(ScreenPage == MAIN_MENU_PAGE)  // If page is on profile normal screen
    {
      // Print profile tittle:
      DEBUGS("MAIN_MENU_DRAW");
      DEBUGV(currentProfilePtr->name);
      DEBUGV(TitleXPosition);
      u8g.drawStr(TitleXPosition, 8, currentProfilePtr->name);  // (x,y,"Text")
      drawStr(37, 18, TextMAINMENU);  // (x,y,"Text")

      drawStr(0, 29, TextEDITTHISPROFILE);  // (x,y,"Text")
      drawStr(0, 40, TextADDNEWPROFILEMenu);  // (x,y,"Text")
      drawStr(0, 51, TextOPTIONSMenu);  // (x,y,"Text")
      drawStr(0, 62, TextGOBACKMenu);  // (x,y,"Text")

      // Selection box:
      if(SelectorPosition == 0)
      {
        u8g.drawFrame(0, 19, 127, SelectBoxHeight);  // Draw a square for EDIT THIS PROFILE (x,y,width,height)
      }
      if(SelectorPosition == 1)
      {
        u8g.drawFrame(0, 30, 127, SelectBoxHeight);  // Draw a square for ADD NEW PROFILE (x,y,width,height)
      }
      if(SelectorPosition == 2)
      {
        u8g.drawFrame(0, 41, 127, SelectBoxHeight);  // Draw a square for FULL CALIBRATION (x,y,width,height)
      }
      if(SelectorPosition == 3)
      {
        u8g.drawFrame(0, SelectBoxYPosition, 127, SelectBoxHeight);  // Draw a square for GO BACK (x,y,width,height)
      }
    }  // End of if we are in normal page





    // Display - Main Menu > EDIT PROFILE
    if(ScreenPage == EDIT_PROFILE_PAGE)  // If page is on EDIT THIS PROFILE
    {
      // Print profile tittle:
      u8g.drawStr(TitleXPosition, 8, currentProfilePtr->name);  // (x,y,"Text")
      drawStr(28, 18, TextEDITPROFILE);  // (x,y,"Text")

      drawStr(0, 29, TextTAREZERO);  // (x,y,"Text")
      drawStr(0, 40, TextEDITPROFILENAMEMenu);  // (x,y,"Text")
      drawStr(0, 51, TextDELETETHISPROFILE);  // (x,y,"Text")
      drawStr(0, 62, TextGOBACKMenu);  // (x,y,"Text")

      // Selection box:
      if(SelectorPosition == 0)
      {
        u8g.drawFrame(0, 19, 127, SelectBoxHeight);  // Draw a square for EDIT THIS PROFILE (x,y,width,height)
      }
      if(SelectorPosition == 1)
      {
        u8g.drawFrame(0, 30, 127, SelectBoxHeight);  // Draw a square for ADD NEW PROFILE (x,y,width,height)
      }
      if(SelectorPosition == 2)
      {
        u8g.drawFrame(0, 41, 127, SelectBoxHeight);  // Draw a square for FULL CALIBRATION (x,y,width,height)
      }
      if(SelectorPosition == 3)
      {
        u8g.drawFrame(0, SelectBoxYPosition, 127, SelectBoxHeight);  // Draw a square for GO BACK (x,y,width,height)
      }
    }  // End of if we are in EDIT THIS PROFILE
    // Display - Main Menu > EDIT THIS PROFILE > TARE 0 (which type of Tare)
    if(ScreenPage == TARE0_PAGE)  // If page is on TARE 1
    {
      // Print profile tittle:
      u8g.drawStr(TitleXPosition, 8, currentProfilePtr->name);  // (x,y,"Text")
      drawStr(28, 18, TextTARE);  // (x,y,"Text")            
      drawStr(0, 29, TextSelecttaretype);  // (x,y,"Text")
      drawStr(0, 40, TextTareemptyspool);  // (x,y,"Text")
      drawStr(0, 51, TextTarefullspool);  // (x,y,"Text")
      drawStr(0, 62, TextGOBACKMenu);  // (x,y,"Text")
      // Selection box:
      // Selection box:
      if(SelectorPosition == 0)
      {
        u8g.drawFrame(0, 30, 127, SelectBoxHeight);  // Draw a square for ADD NEW PROFILE (x,y,width,height)
      }
      if(SelectorPosition == 1)
      {
        u8g.drawFrame(0, 41, 127, SelectBoxHeight);  // Draw a square for FULL CALIBRATION (x,y,width,height)
      }
      if(SelectorPosition == 2)
      {
        u8g.drawFrame(0, SelectBoxYPosition, 127, SelectBoxHeight);  // Draw a square for GO BACK (x,y,width,height)
      }
    }  // End of if page is on Tare 1
    // Display - Main Menu > EDIT THIS PROFILE > TARE 1
    if(ScreenPage == TARE1_PAGE)  // If page is on TARE 1
    {
      // Print profile tittle:
      u8g.drawStr(TitleXPosition, 8, currentProfilePtr->name);  // (x,y,"Text")
            
      drawStr(0, 18, TextPlaceintheholder);  // (x,y,"Text")
      drawStr(0, 28, Texttheemptyspoolyou);  // (x,y,"Text")
      drawStr(0, 38, Textwanttouseforthis);  // (x,y,"Text")
      drawStr(0, 48, Textprofile);  // (x,y,"Text")

      drawStr(10, 62, TextCANCEL);  // (x,y,"Text")
      drawStr(77, 62, TextCONTINUE);  // (x,y,"Text")

      // Selection box:
      if(SelectorPosition == 0)
      {
        u8g.drawFrame(SelectBoxXPositionRight, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for START (x,y,width,height)
      }
      if(SelectorPosition == 1)
      {
        u8g.drawFrame(0, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for NO (x,y,width,height)
      }
    }  // End of if page is on Tare 1

    // Display - Main Menu > EDIT THIS PROFILE > TARE 2
    if(ScreenPage == TARE2_PAGE)  // If page is on TARE 2
    {
      // Print profile tittle:
      u8g.drawStr(TitleXPosition, 8, currentProfilePtr->name);  // (x,y,"Text")
      
      drawStr(0, 18, TextSavingtheweightof);  // (x,y,"Text")
      drawStr(0, 28, Texttheemptyspool);  // (x,y,"Text")
      drawStr(0, 38, TextPleasedonttouch);  // (x,y,"Text")
      drawStr(0, 48, Textanything);  // (x,y,"Text")

      // Progress bar:
      u8g.drawFrame(0, SelectBoxYPosition, 127, SelectBoxHeight);  // Draw a square for frame (x,y,width,height)

      // Print the inside of the progress bar:
      u8g.drawBox(2, 54, ProgressBarCounter, SelectBoxHeight -4);  // Draw a filled square for bar (x,y,width,height)

      // Fill progress bar gradually:
      if(ProgressBarCounter == PROGRESS_BAR_FULL)  // If the progress bar counter reach full limit
      {
        currentProfilePtr->emptySpoolMeasuredWeight =  HolderWeight;
        
        ProgressBarCounter = 0;  // Reset counter
        setScreenPage(TARE3_PAGE);  // Go to page Tare 3
      }
      else  // If progress bar is not yet full
      {
        if(ProgressBarSpeedCounter == ProgressBarSpeedAmount)  // If we waited enough
        {
          ProgressBarCounter = ProgressBarCounter + 1;  // Increase counter by 1
          ProgressBarSpeedCounter = 0;  // Reset counter
        }
        else  // If we are not yet in the limit of the counter
        {
          ProgressBarSpeedCounter = ProgressBarSpeedCounter + 1;  // Increase counter
        }
      }  // End of if progress bar is not yet full
    }  // End of if page is on Tare 2

    // Display - Main Menu > EDIT THIS PROFILE > TARE 3
    if(ScreenPage == TARE3_PAGE)  // If page is on TARE 3
    {
      // Print profile tittle:
      u8g.drawStr(TitleXPosition, 8, currentProfilePtr->name);  // (x,y,"Text")
            
      drawStr(0, 18, TextTheweightofthe);  // (x,y,"Text")
      drawStr(0, 28, Textemptyspoolhasbeen);  // (x,y,"Text")
      drawStr(0, 38, Textsaved);  // (x,y,"Text")

      drawStr(58, 62, TextOK);  // (x,y,"Text")

      // Selection box:
      u8g.drawFrame(SelectBoxXPositionCenter, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for OK (x,y,width,height)

      // Centering lines:
      // This is only used when testing centering items on the screen
      //u8g.drawLine(63, 0, 63, 63);  // Draw a line (x0,y0,x1,y1) Vertical center
    
      //u8g.drawLine(37, 0, 37, 63);  // Draw a line (x0,y0,x1,y1) 1 char left
      //u8g.drawLine(89, 0, 89, 63);  // Draw a line (x0,y0,x1,y1) 1 char right
          
    }  // End of if page is on Tare 3
    if(ScreenPage == FULL_TARE2_PAGE)  
    {
      // Print profile tittle:
      u8g.drawStr(TitleXPosition, 8, currentProfilePtr->name);  // (x,y,"Text")
            
      drawStr(0, 18, TextPlaceintheholder);  // (x,y,"Text")
      drawStr(0, 28, Textthefullspoolyou);  // (x,y,"Text")
      drawStr(0, 38, Textwanttouseforthis);  // (x,y,"Text")
      drawStr(0, 48, Textprofile);  // (x,y,"Text")

      drawStr(10, 62, TextCANCEL);  // (x,y,"Text")
      drawStr(77, 62, TextCONTINUE);  // (x,y,"Text")

      // Selection box:
      if(SelectorPosition == 0)
      {
        u8g.drawFrame(SelectBoxXPositionRight, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for START (x,y,width,height)
      }
      if(SelectorPosition == 1)
      {
        u8g.drawFrame(0, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for NO (x,y,width,height)
      }
    }  

    // Display - Main Menu > EDIT THIS PROFILE > full TARE 3
    if(ScreenPage == FULL_TARE3_PAGE)  
    {
      // Print profile tittle:
      u8g.drawStr(TitleXPosition, 8, currentProfilePtr->name);  // (x,y,"Text")
      
      drawStr(0, 18, TextEstimatingtheweightof);  // (x,y,"Text")
      drawStr(0, 28, Texttheemptyspool);  // (x,y,"Text")
      drawStr(0, 38, TextPleasedonttouch);  // (x,y,"Text")
      drawStr(0, 48, Textanything);  // (x,y,"Text")

      // Progress bar:
      u8g.drawFrame(0, SelectBoxYPosition, 127, SelectBoxHeight);  // Draw a square for frame (x,y,width,height)

      // Print the inside of the progress bar:
      u8g.drawBox(2, 54, ProgressBarCounter, SelectBoxHeight -4);  // Draw a filled square for bar (x,y,width,height)

      // Fill progress bar gradually:
      if(ProgressBarCounter == PROGRESS_BAR_FULL)  // If the progress bar counter reach full limit
      {
        currentProfilePtr->emptySpoolMeasuredWeight =  HolderWeight-currentProfilePtr->fullSpoolFilamentWeight;
        ProgressBarCounter = 0;  // Reset counter
        if(currentProfilePtr->emptySpoolMeasuredWeight <= 0)
        {
          setScreenPage(FULL_TARE5_PAGE);  
          // this simply doesn't add up so bale out
        }
        else
        {
          setScreenPage(FULL_TARE4_PAGE);  // Go to page Full tare 4
        }
      }
      else  // If progress bar is not yet full
      {
        if(ProgressBarSpeedCounter == ProgressBarSpeedAmount)  // If we waited enough
        {
          ProgressBarCounter = ProgressBarCounter + 1;  // Increase counter by 1
          ProgressBarSpeedCounter = 0;  // Reset counter
        }
        else  // If we are not yet in the limit of the counter
        {
          ProgressBarSpeedCounter = ProgressBarSpeedCounter + 1;  // Increase counter
        }
      }  // End of if progress bar is not yet full
    }  // End of if page is on Tare 2

    // Display - Main Menu > EDIT THIS PROFILE > FULL TARE 4
    if(ScreenPage == FULL_TARE4_PAGE)
    {
      // Print profile tittle:
      u8g.drawStr(TitleXPosition, 8, currentProfilePtr->name);  // (x,y,"Text")
            
      drawStr(0, 18, TextTheweightofthe);  // (x,y,"Text")
      drawStr(0, 28, Textemptyspoolhasbeen);  // (x,y,"Text")
      drawStr(0, 38, Textestimated);  // (x,y,"Text")
      u8g.drawStr(40, 38, currentProfilePtr->emptySpoolMeasuredWeight);  // (x,y,"Text")
      drawStr(58, 62, TextOK);  // (x,y,"Text")

      // Selection box:
      u8g.drawFrame(SelectBoxXPositionCenter, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for OK (x,y,width,height)

      // Centering lines:
      // This is only used when testing centering items on the screen
      //u8g.drawLine(63, 0, 63, 63);  // Draw a line (x0,y0,x1,y1) Vertical center
    
      //u8g.drawLine(37, 0, 37, 63);  // Draw a line (x0,y0,x1,y1) 1 char left
      //u8g.drawLine(89, 0, 89, 63);  // Draw a line (x0,y0,x1,y1) 1 char right
          
    }  // End of if page is on Tare 3
    if(ScreenPage == FULL_TARE5_PAGE)
    {
      // Print profile tittle:
      u8g.drawStr(TitleXPosition, 8, currentProfilePtr->name);  // (x,y,"Text")
            
      drawStr(0, 18, TextTheweightofthe);  // (x,y,"Text")
      drawStr(0, 28, Textemptyspoolhasbeen);  // (x,y,"Text")
      drawStr(0, 38, Textincorrectlyestimated);  // (x,y,"Text")
      drawStr(12, 62, TextCANCEL);  // (x,y,"Text")  Correctly centered
      drawStr(86, 62, TextRETRY);  // (x,y,"Text")  Correctly centered

      // Selection box:
      if(SelectorPosition == 0)
      {
        u8g.drawFrame(0, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for NO (x,y,width,height)
      }
      if(SelectorPosition == 1)
      {
        u8g.drawFrame(SelectBoxXPositionRight, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for START (x,y,width,height)
      }
    }  // End of if page is on Tare 3

    // Display Tare using full spool
    if(ScreenPage == ENTER_FILAMENT_WEIGHT_PAGE)  // If page is on full calibration > Step 2
    {
      drawStr(0, 8, TextEntertheweight);  // (x,y,"Text")
      u8g.setCursor(40, 35);  // (x,y)

      u8g.print(currentProfilePtr->fullSpoolFilamentWeight);  // Print full spool weight selected by user in calibration

      drawStr(58, 62, TextOK);  // (x,y,"Text")

      // Selection box:
      u8g.drawFrame(SelectBoxXPositionCenter, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for OK (x,y,width,height)

      // Big arrows:
      u8g.drawTriangle(0,30,    11,24,    11,36);  // Draw a filled triangle (x0,y0,    x1,y1,    x2,y2) LEFT ARROW
      u8g.drawTriangle(127,30,    116,24,    116,36);  // Draw a filled triangle (x0,y0,    x1,y1,    x2,y2) RIGHT ARROW

      // Centering lines:
      // This is only used when testing centering items on the screen
      //u8g.drawLine(63, 0, 63, 63);  // Draw a line (x0,y0,x1,y1) Vertical center
    
      //u8g.drawLine(37, 0, 37, 63);  // Draw a line (x0,y0,x1,y1) 1 char left
      //u8g.drawLine(89, 0, 89, 63);  // Draw a line (x0,y0,x1,y1) 1 char right
    }  // End of if page is on full calibration 2



    // Display - Main Menu > EDIT THIS PROFILE > EDIT PROFILE NAME
    if(ScreenPage == EDIT_PROFILE_NAME_PAGE || ScreenPage == NEW_PROFILE_PAGE)  // If page is on EDIT PROFILE NAME or NEW PROFILE
    {
      // Print profile title:
      // If we are editing an existing profile, we show the text for that, and
      // if we are creating a new profile, we show that other text:
      if(ScreenPage == EDIT_PROFILE_NAME_PAGE)  // If page is on EDIT PROFILE NAME
      {
        drawStr(13, 8, TextEDITPROFILENAME);  // (x,y,"Text")
      }
      else if(ScreenPage == NEW_PROFILE_PAGE)  // If page is on ADD NEW PROFILE
      {
        drawStr(19, 8, TextADDNEWPROFILE);  // (x,y,"Text")
      }

      // Profile name, so far:
      u8g.drawStr(0, 28, tempProfileTitleBuffer);  // Use the non-RO string version as this is not PROGMEM

      // Text Cursor:
      if(BlinkingCursorCounter == BlinkingCursorAmount)  // If counter reach limit
      {
        if(BlinkingCursorState == 0)  // If text cursor is OFF
        {
          BlinkingCursorState = 1;  // Turn text cursor ON
        }
        else  // If text cursor is ON
        {
          BlinkingCursorState = 0;  // Turn text cursor OFF
        }
        BlinkingCursorCounter = 0;  // Reset counter to start over
      }
      else  // If counter for cursor is not yet at the limit
      {
        BlinkingCursorCounter = BlinkingCursorCounter + 1;  // Increase counter by 1
      }

      // Print the line for the text cursor only if the state is 1:
      if(BlinkingCursorState == 1)
      {
        u8g.drawLine(TextCursorPositionX, 30, TextCursorPositionX + 4, 30);  // Draw a line (x0,y0,x1,y1) Cursor
      }

      // Print letters depending on the position of the selector:
      char buffer[20];
      DEBUGV(SelectorPosition);
      if(SelectorPosition < 44)
      {
        sprintf(buffer, "%c %c %c %c %c %c %c", 
                selectorChar(SelectorPosition-3),
                selectorChar(SelectorPosition-2),
                selectorChar(SelectorPosition-1),
                selectorChar(SelectorPosition),
                selectorChar(SelectorPosition+1),
                selectorChar(SelectorPosition+2),
                selectorChar(SelectorPosition+3));
        DEBUGV(buffer);
        u8g.drawStr(25, 48, buffer);  // (x,y,"Text")
      }
      else if(SelectorPosition == 44)  // SPACEBAR
      {
        drawStr(49, 48, ABCTextCharacterOPTION1);  // (x,y,"Text")
        u8g.drawFrame(28, 38, 71, 13);  // Draw filled square
        //u8g.setColorIndex(0);  // Print the next erasing the filled box
        // u8g.drawLine(47, 46, 47, 41);  // Draw a line (x0,y0,x1,y1) 1 char left
        // u8g.drawLine(79, 46, 79, 41);  // Draw a line (x0,y0,x1,y1) 1 char right
        // u8g.drawLine(47, 47, 79, 47);  // Draw a line (x0,y0,x1,y1) horizontal
        //u8g.setColorIndex(1);  // Return to printing the normal way

      }
      else if(SelectorPosition == 45)  // BACKSPACE
      {
        // drawStr(37, 48, ABCTextCharacterOPTION3);  // (x,y,"Text")

        u8g.drawFrame(28, 38, 71, 13);  // Draw filled square (x,y,width,height)

        u8g.drawBox(62, 40, 11, 9);  // Draw filled square for symbol (x,y,width,height)

        u8g.drawTriangle(54,44,    62,39,    62,49);  // Draw a filled triangle (x0,y0,    x1,y1,    x2,y2) LEFT ARROW

        u8g.setColorIndex(0);  // Print the next erasing the filled box
       
        u8g.drawLine(63, 41, 69, 47);  // Draw a line (x0,y0,x1,y1) X symbol
        u8g.drawLine(64, 41, 70, 47);  // Draw a line (x0,y0,x1,y1) X symbol
        
        u8g.drawLine(63, 47, 69, 41);  // Draw a line (x0,y0,x1,y1) X symbol
        u8g.drawLine(64, 47, 70, 41);  // Draw a line (x0,y0,x1,y1) X symbol
        u8g.setColorIndex(1);  // Return to printing the normal way
      }
      else if(SelectorPosition == 46)  // CANCEL
      {
        drawStr(46, 48, ABCTextCharacterOPTION4);  // (x,y,"Text")
      }
      else if(SelectorPosition == 47)  // SAVE
      {
        drawStr(52, 48, ABCTextCharacterOPTION5);  // (x,y,"Text")
      }

      // Selection box:
      // Depending on the options, the selection box size changes:
      if(SelectorPosition >= 44 && SelectorPosition <= 48)  // If the selection is in one of the options
      {
        u8g.drawFrame(28, 38, 71, 13);  // Draw a square box around the option
      }
      else  // If the selector is not in an option
      {
        u8g.drawFrame(58, 38, 11, 13);  // Draw a square for character on the center (x,y,width,height)
      }

      // Draw arrows:
      u8g.drawTriangle(0,43,    11,37,    11,49);  // Draw a filled triangle (x0,y0,    x1,y1,    x2,y2) LEFT ARROW
      u8g.drawTriangle(127,43,    116,37,    116,49);  // Draw a filled triangle (x0,y0,    x1,y1,    x2,y2) RIGHT ARROW

      // Centering lines:
      // This is only used when testing centering items on the screen
      //u8g.drawLine(63, 0, 63, 63);  // Draw a line (x0,y0,x1,y1) Vertical center

      //u8g.drawLine(52, 0, 52, 63);  // Draw a line (x0,y0,x1,y1) 1 char left
      //u8g.drawLine(74, 0, 74, 63);  // Draw a line (x0,y0,x1,y1) 1 char right
    
      //u8g.drawLine(37, 0, 37, 63);  // Draw a line (x0,y0,x1,y1) 1 char left
      //u8g.drawLine(89, 0, 89, 63);  // Draw a line (x0,y0,x1,y1) 1 char right
          
    }  // End of if page is on edit profile name





    // Display - Main Menu > ADD PROFILE - No space available
    if( ScreenPage == NO_MORE_SPACE_AVAILABLE_PAGE )  // If page is on ADD PROFILE - No space available
    {
      // Print profile tittle:
      drawStr(0, 8, TextMemoryisfull);  // (x,y,"Text")
      drawStr(0, 18, TextYoucantaddmore);  // (x,y,"Text")
      drawStr(0, 28, Textprofiles);  // (x,y,"Text")

      drawStr(58, 62, TextOK);  // (x,y,"Text")

      // Selection box:
      u8g.drawFrame(SelectBoxXPositionCenter, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for OK (x,y,width,height)
    }  // End of if page is on ADD PROFILE - No space available





    // Display - Main Menu > EDIT THIS PROFILE > DELETE THIS PROFILE
    if(ScreenPage == DELETE_PROFILE_PAGE)  // If page is on DELETE THIS PROFILE
    {
      drawStr(0, 8, TextTheprofilenamed);  // (x,y,"Text")
      u8g.drawStr(0, 18, currentProfilePtr->name);  // (x,y,"Text")
      drawStr(0, 28, Textisgoingtobe);  // (x,y,"Text")
      drawStr(0, 38, TextdeletedOK);  // (x,y,"Text")

      drawStr(10, 62, TextCANCEL);  // (x,y,"Text")
      drawStr(83, 62, TextDELETE);  // (x,y,"Text")

      // Selection box:
      if(SelectorPosition == 1)
      {
        u8g.drawFrame(SelectBoxXPositionRight, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for START (x,y,width,height)
      }
      if(SelectorPosition == 0)
      {
        u8g.drawFrame(0, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for NO (x,y,width,height)
      }
    }  // End of if page is on DELETE THIS PROFILE





    // Display - Main Menu > EDIT THIS PROFILE > DELETE THIS PROFILE 2
    if(ScreenPage == PROFILE_DELETED_PAGE)  // If page is on DELETE THIS PROFILE 2
    {
      // Print profile tittle:
      drawStr(0, 8, TextTheprofilenamed);  // (x,y,"Text")
      u8g.drawStr(0, 18, currentProfilePtr->name);  // (x,y,"Text")
      drawStr(0, 28, Texthasbeendeleted);  // (x,y,"Text")

      drawStr(58, 62, TextOK);  // (x,y,"Text")

      // Selection box:
      u8g.drawFrame(SelectBoxXPositionCenter, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for OK (x,y,width,height)

      // Centering lines:
      // This is only used when testing centering items on the screen
      //u8g.drawLine(63, 0, 63, 63);  // Draw a line (x0,y0,x1,y1) Vertical center
    
      //u8g.drawLine(37, 0, 37, 63);  // Draw a line (x0,y0,x1,y1) 1 char left
      //u8g.drawLine(89, 0, 89, 63);  // Draw a line (x0,y0,x1,y1) 1 char right
    }  // End of if page is on DELETE THIS PROFILE 2





    // Display - Main Menu > EDIT THIS PROFILE > DELETE THIS PROFILE 3
    if(ScreenPage == PROFILE_NOT_DELETED_PAGE)  // If page is on DELETE THIS PROFILE 3
    {
      // Print profile tittle:
      drawStr(0, 8, TextYoucantdelete);  // (x,y,"Text")
      drawStr(0, 18, Texttheonlyprofile);  // (x,y,"Text")
      drawStr(0, 28, Textleft);  // (x,y,"Text")

      drawStr(58, 62, TextOK);  // (x,y,"Text")

      // Selection box:
      u8g.drawFrame(SelectBoxXPositionCenter, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for OK (x,y,width,height)
    }  // End of if page is on DELETE THIS PROFILE 3





    // Display - Main Menu > Options
    if(ScreenPage == OPTIONS_PAGE)  // If page is on Options
    {
      // Print profile tittle:
      drawStr(43, 8, TextOPTIONS);  // (x,y,"Text")

      drawStr(0, 19, TextDEADZONEMenu);  // (x,y,"Text")
      drawStr(0, 30, TextFULLCALIBRATION);  // (x,y,"Text")
      drawStr(0, 41, TextFACTORYRESETMenu);  // (x,y,"Text")
      drawStr(0, 52, TextGOBACKMenu);  // (x,y,"Text")

      // Selection box:
      if(SelectorPosition == 0)
      {
        u8g.drawFrame(0, 9, 127, SelectBoxHeight);  // Draw a square for deadzone (x,y,width,height)
      }
      if(SelectorPosition == 1)
      {
        u8g.drawFrame(0, 20, 127, SelectBoxHeight);  // Draw a square for FULL CALIBRATION (x,y,width,height)
      }
      if(SelectorPosition == 2)
      {
        u8g.drawFrame(0, 31, 127, SelectBoxHeight);  // Draw a square for FACTORY RESET (x,y,width,height)
      }
      if(SelectorPosition == 3)
      {
        u8g.drawFrame(0, 42, 127, SelectBoxHeight);  // Draw a square for GO BACK (x,y,width,height)
      }
    }  // End of if we are in Options

    // Display - Main Menu > Options > Deadzone
    if(ScreenPage == DEADZONE_PAGE)  // If page is on Deadzone
    {
      // Print profile tittle:
      drawStr(40, 8, TextDEADZONE);  // (x,y,"Text")

      // Center deadzone amount text:
      if(config.deadzoneAmount * 2 < 10)
      {
        u8g.setCursor(61, 25);  // (x,y)
      }
      else if(config.deadzoneAmount * 2 < 100)
      {
        u8g.setCursor(58, 25);  // (x,y)
      }
      else
      {
        u8g.setCursor(55, 25);  // (x,y)
      }

      u8g.print(config.deadzoneAmount * 2);  // Deadzone amount set, multiply by 2 so it shows the total deadzone

      drawStr(0, 38, TextINPUT);  // (x,y,"Text")
      u8g.setCursor(0, 47);  // (x,y)
      // Make sure if weight close to 0, show 0:
      if(SmoothedWeight < MinimumWeightAllowed)
      {
        SmoothedWeightToShowAsInputInDeadzone = 0;
      }
      else
      {
        SmoothedWeightToShowAsInputInDeadzone = SmoothedWeight;  // Show smoothed weight
      }
      u8g.print(SmoothedWeightToShowAsInputInDeadzone);  // Print weight with smooth without deadzone

      drawStr(80, 38, TextOUTPUT);  // (x,y,"Text")
      u8g.setCursor(80, 47);  // (x,y)
      u8g.print(FinalWeightToShow);  // Print weight after deadzone

      drawStr(58, 62, TextOK);  // (x,y,"Text")

      // Selection box:
      u8g.drawFrame(SelectBoxXPositionCenter, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for OK (x,y,width,height)

      // Big arrows:
      u8g.drawTriangle(0,20,    11,14,    11,26);  // Draw a filled triangle (x0,y0,    x1,y1,    x2,y2) LEFT ARROW
      u8g.drawTriangle(127,20,    116,14,    116,26);  // Draw a filled triangle (x0,y0,    x1,y1,    x2,y2) RIGHT ARROW

      // Centering lines:
      // This is only used when testing centering items on the screen
      //u8g.drawLine(63, 0, 63, 63);  // Draw a line (x0,y0,x1,y1) Vertical center
    
      //u8g.drawLine(37, 0, 37, 63);  // Draw a line (x0,y0,x1,y1) 1 char left
      //u8g.drawLine(89, 0, 89, 63);  // Draw a line (x0,y0,x1,y1) 1 char right
    }  // End of if we are in Deadzone
    // Display full calibration intro:
    if(ScreenPage == FULL_CALIBRATION_PAGE)  // If page is on full calibration intro
    {
      drawStr(0, 8, TextThisprocedureis);  // (x,y,"Text")
      drawStr(0, 18, Textabouttakinga);  // (x,y,"Text")
      drawStr(0, 28, Textreferencefromareal);  // (x,y,"Text")
      drawStr(0, 38, Textscaletocalibrate);  // (x,y,"Text")
      drawStr(0, 48, Texttheloadcell);  // (x,y,"Text")

      drawStr(10, 62, TextCANCEL);  // (x,y,"Text")
      drawStr(77, 62, TextCONTINUE);  // (x,y,"Text")

      // Selection box:
      if(SelectorPosition == 0)
      {
        u8g.drawFrame(SelectBoxXPositionRight, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for START (x,y,width,height)
      }
      if(SelectorPosition == 1)
      {
        u8g.drawFrame(0, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for NO (x,y,width,height)
      }
    }  // End of if page is on full calibration intro





    // Display full calibration > Step 1:
    if(ScreenPage == WEIGH_FULL_SPOOL_EXTERNAL_PAGE)  // If page is on full calibration
    {
      drawStr(0, 8, TextPlaceanyfullspool);  // (x,y,"Text")
      drawStr(0, 18, Textonarealscaleand);  // (x,y,"Text")
      drawStr(0, 28, Texttakenoteofthe);  // (x,y,"Text")
      drawStr(0, 38, Textexactweightgrams);  // (x,y,"Text")

      drawStr(7, 62, TextGOBACK);  // (x,y,"Text")
      drawStr(77, 62, TextCONTINUE);  // (x,y,"Text")
      
      // Reset variable for selecting the weight to the default value:
      config.fullSpoolWeight = 1250;  // This is the default value I want to start with

      // Selection box:
      if(SelectorPosition == 0)
      {
        u8g.drawFrame(SelectBoxXPositionRight, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for START (x,y,width,height)
      }
      if(SelectorPosition == 1)
      {
        u8g.drawFrame(0, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for NO (x,y,width,height)
      }
    }  // End of if page is on full calibration 1





    // Display full calibration > Step 2:
    if(ScreenPage == ENTER_WEIGHT_PAGE)  // If page is on full calibration > Step 2
    {
      drawStr(0, 8, TextEntertheweight);  // (x,y,"Text")
      u8g.setCursor(40, 35);  // (x,y)
      u8g.print(config.fullSpoolWeight);  // Print full spool weight selected by user in calibration

      drawStr(58, 62, TextOK);  // (x,y,"Text")

      // Selection box:
      u8g.drawFrame(SelectBoxXPositionCenter, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for OK (x,y,width,height)

      // Big arrows:
      u8g.drawTriangle(0,30,    11,24,    11,36);  // Draw a filled triangle (x0,y0,    x1,y1,    x2,y2) LEFT ARROW
      u8g.drawTriangle(127,30,    116,24,    116,36);  // Draw a filled triangle (x0,y0,    x1,y1,    x2,y2) RIGHT ARROW

      // Centering lines:
      // This is only used when testing centering items on the screen
      //u8g.drawLine(63, 0, 63, 63);  // Draw a line (x0,y0,x1,y1) Vertical center
    
      //u8g.drawLine(37, 0, 37, 63);  // Draw a line (x0,y0,x1,y1) 1 char left
      //u8g.drawLine(89, 0, 89, 63);  // Draw a line (x0,y0,x1,y1) 1 char right
    }  // End of if page is on full calibration 2

    // Display full calibration > Step 3:
    if(ScreenPage == PLACE_FULL_SPOOL_PAGE)  // If page is on full calibration > Step 3
    {
      drawStr(0, 8, TextPlacethatsamefull);  // (x,y,"Text")
      drawStr(0, 18, Textspoolintheholder);  // (x,y,"Text")

      drawStr(10, 62, TextCANCEL);  // (x,y,"Text")
      drawStr(77, 62, TextCONTINUE);  // (x,y,"Text")

      // Selection box:
      if(SelectorPosition == 0)
      {
        u8g.drawFrame(SelectBoxXPositionRight, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for START (x,y,width,height)
      }
      if(SelectorPosition == 1)
      {
        u8g.drawFrame(0, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for NO (x,y,width,height)
      }
    }  // End of if page is on full calibration 3





    // Display full calibration > Step 4:
    if(ScreenPage == WEIGH_FULL_SPOOL_PAGE)  // If page is on full calibration > Step 4
    {
      drawStr(0, 8, TextSavingtheweightof);  // (x,y,"Text")
      drawStr(0, 18, Textthefullspool);  // (x,y,"Text")
      drawStr(0, 28, TextPleasedonttouch);  // (x,y,"Text")
      drawStr(0, 38, Textanything);  // (x,y,"Text")

      // Progress bar:
      u8g.drawFrame(0, SelectBoxYPosition, 127, SelectBoxHeight);  // Draw a square for frame (x,y,width,height)

      // Print the inside of the progress bar:
      u8g.drawBox(2, 54, ProgressBarCounter, SelectBoxHeight -4);  // Draw a filled square for bar (x,y,width,height)
      // Fill progress bar gradually:
      if(ProgressBarCounter == PROGRESS_BAR_FULL)  // If the progress bar counter reach full limit
      {
        // Transfer the value of the load cell to the temporal variable;
        config.fullSpoolRaw = RawLoadCellReading;
        DEBUGV(config.fullSpoolRaw);
        ProgressBarCounter = 0;  // Reset counter
        setScreenPage(REMOVE_SPOOL_PAGE);  // Go to page Full Calibration 5
      }
      else  // If progress bar is not yet full
      {
        if(ProgressBarSpeedCounter == ProgressBarSpeedAmount)  // If we waited enough
        {
          ProgressBarCounter = ProgressBarCounter + 1;  // Increase counter by 1
          ProgressBarSpeedCounter = 0;  // Reset counter
        }
        else  // If we are not yet in the limit of the counter
        {
          ProgressBarSpeedCounter = ProgressBarSpeedCounter + 1;  // Increase counter
        }
      }  // End of if progress bar is not yet full
    }  // End of if page is on full calibration 4

    // Display full calibration > Step 5:
    if(ScreenPage == REMOVE_SPOOL_PAGE)  // If page is on full calibration > Step 5
    {
      drawStr(0, 8, TextWeightsaved);  // (x,y,"Text")
      drawStr(0, 18, TextRemovethespoolfrom);  // (x,y,"Text")
      drawStr(0, 28, Texttheholderleaving);  // (x,y,"Text")
      drawStr(0, 38, Texttheholdercompletely);  // (x,y,"Text")
      drawStr(0, 48, Textempty);  // (x,y,"Text")

      drawStr(10, 62, TextCANCEL);  // (x,y,"Text")
      drawStr(77, 62, TextCONTINUE);  // (x,y,"Text")

      // Selection box:
      if(SelectorPosition == 0)
      {
        u8g.drawFrame(SelectBoxXPositionRight, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for START (x,y,width,height)
      }
      if(SelectorPosition == 1)
      {
        u8g.drawFrame(0, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for NO (x,y,width,height)
      }
    }  // End of if page is on full calibration 5





    // Display full calibration > Step 6:
    if(ScreenPage == WEIGH_EMPTY_SCALE_PAGE)  // If page is on full calibration > Step 6
    {
      drawStr(0, 8, TextSavingthevalueof);  // (x,y,"Text")
      drawStr(0, 18, Textanemptyholder);  // (x,y,"Text")
      drawStr(0, 28, TextPleasedonttouch);  // (x,y,"Text")
      drawStr(0, 38, Textanything);  // (x,y,"Text")

      // Progress bar:
      u8g.drawFrame(0, SelectBoxYPosition, 127, SelectBoxHeight);  // Draw a square for frame (x,y,width,height)

      // Print the inside of the progress bar:
      u8g.drawBox(2, 54, ProgressBarCounter, SelectBoxHeight -4);  // Draw a filled square for bar (x,y,width,height)

      // Fill progress bar gradually:
      if(ProgressBarCounter == PROGRESS_BAR_FULL)  // If the progress bar counter reach full limit
      {
        config.emptyHolderRaw = RawLoadCellReading;
        DEBUGV(config.emptyHolderRaw);       
        config.calibrationDone = 1;
        ProgressBarCounter = 0;  // Reset counter
        setScreenPage(CALIBRATION_COMPLETE_PAGE);  // Go to page Full Calibration 7
      }
      else  // If progress bar is not yet full
      {
        if(ProgressBarSpeedCounter == ProgressBarSpeedAmount)  // If we waited enough
        {
          ProgressBarCounter = ProgressBarCounter + 1;  // Increase counter by 1
          ProgressBarSpeedCounter = 0;  // Reset counter
        }
        else  // If we are not yet in the limit of the counter
        {
          ProgressBarSpeedCounter = ProgressBarSpeedCounter + 1;  // Increase counter
        }
      }  // End of if progress bar is not yet full
    }  // End of if page is on full calibration 6

    // Display full calibration > Step 7:
    if(ScreenPage == CALIBRATION_COMPLETE_PAGE)  // If page is on full calibration > Step 7
    {
      // Print profile tittle:
      drawStr(0, 8, TextThecalibrationhas);  // (x,y,"Text")
      drawStr(0, 18, Textbeencompleted);  // (x,y,"Text")

      drawStr(58, 62, TextOK);  // (x,y,"Text")

      // Selection box:
      u8g.drawFrame(SelectBoxXPositionCenter, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for OK (x,y,width,height)
    }  // End of if page is on full calibration 7

    // Display Options > Factory Reset:
    if(ScreenPage == FACTORY_RESET_PAGE)  // If page is on Factory Reset
    {
      drawStr(0, 8, TextFactoryreseterases);  // (x,y,"Text")
      drawStr(0, 18, Textallyouruserdata);  // (x,y,"Text")

      drawStr(10, 62, TextCANCEL);  // (x,y,"Text")
      drawStr(83, 62, TextDELETE);  // (x,y,"Text")

      // Selection box:
      if(SelectorPosition == 1)
      {
        u8g.drawFrame(SelectBoxXPositionRight, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for START (x,y,width,height)
      }
      if(SelectorPosition == 0)
      {
        u8g.drawFrame(0, SelectBoxYPosition, SelectBoxWigth, SelectBoxHeight);  // Draw a square for NO (x,y,width,height)
      }
    }  // End of if page is on Display Options > Factory Reset

    // Display Options > Factory Reset 2:
    if(ScreenPage == FACTORY_RESET2_PAGE)  // If page is on Factory Reset 2
    {
      drawStr(0, 8, TextFactoryresetin);  // (x,y,"Text")
      drawStr(0, 18, Textprogress);  // (x,y,"Text")
      drawStr(0, 38, TextPleasewait);  // (x,y,"Text")
    }  // End of if page is on Display Options > Factory Reset
  }  // End of display
  while(u8g.nextPage());  // End of the picture loop

  
  // If we are doing a Factory Reset, excecute this:
  if(ScreenPage == FACTORY_RESET2_PAGE)  // If page is on Factory Reset 2
  {
    // Clear all EEPROM:
    for(int i = 0 ; i < EEPROM.length() ; i++) {EEPROM.write(i, 255);}  // Go thourgh all the EEPROM addresses and write the default value (255)
    resetFunc();  // Do a soft Reset of Arduino 
  }

  writeConfigurationToEEPROM();
}  // End of loop

