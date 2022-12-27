#include <Arduino.h>

// Need to install Timezone by Jack Christensen
#include <Timezone.h>    // https://github.com/JChristensen/Timezone

extern bool readyForTimeUpdate = false;

// Cooperative task to update the time
class UpdateTimeTask: public LeanTask {
    public:
        UpdateTimeTask(bool _enabled = false, unsigned long _interval = 0) : LeanTask (_enabled, _interval) {}

        volatile uint16_t displayNumber = 0;

        void setTimeZone(String timeZoneName) {
            Serial.printf("Timezone set to %s", timeZoneName);
            Serial.println();
            timezone = createTimeZone(timeZoneName);
        }

    protected:
        void loop() {
            bool hasHourChanged = (lastHour != hour());
            bool hasMinutesChanged = (lastMinute != minute());

            // Update the display time when the time changes
            if (hasHourChanged || hasMinutesChanged) {
                // Get the time and convert it to the current timezone
                //time_t timezoneTime = UK.toLocal(now());
                time_t timezoneTime = timezone->toLocal(now());
                tm timezoneTimeParts = getDateTimeByParams(timezoneTime);

                int displayHours = timezoneTimeParts.tm_hour;
                int displayMinutes = timezoneTimeParts.tm_min;
                displayNumber = displayHours * 100 + displayMinutes;

                Serial.printf("Time changed: %d", displayNumber);
                Serial.println();

                lastHour = hour();
                lastMinute = minute();
            }
        }
        
    private:
        uint8_t lastHour = 0;
        uint8_t lastMinute = 0;
        Timezone *timezone;

        /**
        * Input time in epoch format and return tm time format
        * by Renzo Mischianti <www.mischianti.org> 
        */
        tm getDateTimeByParams(long time){
            struct tm *newtime;
            const time_t tim = time;
            newtime = localtime(&tim);
            return *newtime;
        }

        Timezone* createTimeZone(String timeZoneName) {
            if (timeZoneName == "AEDT") {
                // Australia Eastern Time Zone (Sydney, Melbourne)
                TimeChangeRule aEDT = {"AEDT", First, Sun, Oct, 2, 660};    // UTC + 11 hours
                TimeChangeRule aEST = {"AEST", First, Sun, Apr, 3, 600};    // UTC + 10 hours
                return new Timezone(aEDT, aEST);
            }
            
            if (timeZoneName == "MSK") {
                // Moscow Standard Time (MSK, does not observe DST)
                TimeChangeRule msk = {"MSK", Last, Sun, Mar, 1, 180};
                return new Timezone(msk);
            }
            
            if (timeZoneName == "BST") {
                // United Kingdom (London, Belfast)
                TimeChangeRule BST = {"BST", Last, Sun, Mar, 1, 60};        // British Summer Time
                TimeChangeRule GMT = {"GMT", Last, Sun, Oct, 2, 0};         // Standard Time
                return new Timezone(BST, GMT);
            }

            if (timeZoneName == "CEST") {
                // Central European Time (Frankfurt, Paris)
                TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
                TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
                return new Timezone(CEST, CET);
            }

            if (timeZoneName == "UTC") {
                // UTC
                TimeChangeRule utcRule = {"UTC", Last, Sun, Mar, 1, 0};     // UTC
                return new Timezone(utcRule);
            }
            
            if (timeZoneName == "EST") {
                // US Eastern Time Zone (New York, Detroit)
                TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  // Eastern Daylight Time = UTC - 4 hours
                TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   // Eastern Standard Time = UTC - 5 hours
                return new Timezone(usEDT, usEST);
            }

            if (timeZoneName == "CST") {
                // US Central Time Zone (Chicago, Houston)
                TimeChangeRule usCDT = {"CDT", Second, Sun, Mar, 2, -300};
                TimeChangeRule usCST = {"CST", First, Sun, Nov, 2, -360};
                return new Timezone(usCDT, usCST);
            }

            if (timeZoneName == "MST") {
                // US Mountain Time Zone (Denver, Salt Lake City)
                TimeChangeRule usMDT = {"MDT", Second, Sun, Mar, 2, -360};
                TimeChangeRule usMST = {"MST", First, Sun, Nov, 2, -420};
                return new Timezone(usMDT, usMST);
            }

            if (timeZoneName == "AZT") {
                // Arizona is US Mountain Time Zone but does not use DST
                TimeChangeRule usMST = {"MST", First, Sun, Nov, 2, -420};
                return new Timezone(usMST);
            }
            
            if (timeZoneName == "AZT") {
                // US Pacific Time Zone (Las Vegas, Los Angeles)
                TimeChangeRule usPDT = {"PDT", Second, Sun, Mar, 2, -420};
                TimeChangeRule usPST = {"PST", First, Sun, Nov, 2, -480};
                return new Timezone(usPDT, usPST);
            }
        }
};

