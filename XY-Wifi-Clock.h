// constants for time display mode (variable displayMode)
// NOTE: these constants must be in the same order as the strings in data/displayModes.json
#define TIME_MODE_24_SOLID_COLON            0
#define TIME_MODE_24_BLINKING_COLON         1
#define TIME_MODE_12_SOLID_COLON            2
#define TIME_MODE_12_SOLID_COLON_AM_LED     3
#define TIME_MODE_12_SOLID_COLON_PM_LED     4
#define TIME_MODE_12_BLINKING_COLON_ALWAYS  5
#define TIME_MODE_12_BLINKING_COLON_AM      6
#define TIME_MODE_12_BLINKING_COLON_PM      7

// constants for date format (variable dateFormat)
// NOTE: these constants must be in the same order as the strings in data/dateFormats.json
#define DATE_FORMAT_MM_DD_YYYY              0
#define DATE_FORMAT_DD_MM_YYYY              1

// constants for alarm sound type (variable alarmSound)
// NOTE: these constants must be in the same order as the strings in data/alarmSounds.json
#define ALARM_SOUND_BUZZER                  0
#define ALARM_SOUND_IMPERIAL_MARCH          1
#define ALARM_SOUND_PINK_PANTHER            2

// WiFi connect timeout in seconds.  If WiFi doesn't connect within this time period
// the clock will revert to using the DS1307 clock chip time
#define WIFI_CONNECT_TIMEOUT                15

// default app name (also used as AP SSID)
#define DEFAULT_APP_NAME                    "XY-Clock"

// WiFi AP password
#define WIFI_AP_PASSWORD                    ""

// NTP initial timeout in seconds
//  On power-up after a successful network connection, this is the amount of time the clock
//  will wait for an NTP time update before switching to the DS1307 clock chip to get the
//  time.  A countdown of this time will appear on the display and the Serial port.  Note that
//  the DS1307 time will be used immediately if there is no network connection.  A timeout
//  could mean that the network connection being used doesn't have internet access, such as at
//  a hotel that provides wifi but requires a login via browser.
#define NTP_TIMEOUT                         15                      // should be a minimum of 5 seconds

// Clock time update interval in milliseconds
//  This is the time between updates of the ESP8285 time from the current
//  time source, either NTP or DS1307.  Whenever an NTP update occurs, the
//  new time is stored in the DS1307 clock chip.
#define TIME_UPDATE_INTERVAL                (1 * 60 * 60 * 1000)    // 1 hour

// button long-press time in milliseconds
#define BUTTON_LONG_PRESS                   (2 * 1000)              // 2 seconds

// hotspot open timeout in milliseconds
#define HOTSPOT_OPEN_TIME                   (5 * 60 * 1000)         // 5 minutes

// adjust this number to match the external LED brightness with the display
// the value must be greater than or equal to 8 and less than or equal to 255
#define AM_PM_BRIGHTNESS_FACTOR             32

// maximum alarm buzzer time in milliseconds
#define MAXIMUM_BUZZER_TIME                 (5 * 60 * 1000)         // 5 minutes
