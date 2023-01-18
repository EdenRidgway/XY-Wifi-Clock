#include <Arduino.h>

// Need to install Timezone by Jack Christensen
#include <Timezone.h>    // https://github.com/JChristensen/Timezone

class UpdateTimeTask {
    public:
        UpdateTimeTask() {

        }

        volatile uint16_t displayNumber = 0;

        void setTimezone(String timeZoneName) {
            Serial.printf("Timezone set to %s", timeZoneName);
            Serial.println();
            timezone = createTimeZone(timeZoneName);
        }

        int getDayOfWeek() {
            time_t timezoneTime = timezone->toLocal(now());
            tm timeParts = getDateTimeByParams(timezoneTime);
            return timeParts.tm_wday;            
        }

        tm getTimeParts() {
            time_t timezoneTime = timezone->toLocal(now());
            tm timeParts = getDateTimeByParams(timezoneTime);
            return timeParts;
        }

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
            Serial.print("Creating timezone: ");
            Serial.print(timeZoneName);
            Serial.println();

            if (timeZoneName == "WET") {
                // UTC0 Western European Time (with DST)
                TimeChangeRule WEST = {"WEST", Last, Sun, Mar, 1, 60};
                TimeChangeRule WET = {"WET", Last, Sun, Oct, 2, 0};
                return new Timezone(WEST, WET);
            }

            if (timeZoneName == "WAT") {
                // UTC+1 West Africa Time (no DST)
                TimeChangeRule WAT = {"WAT", Last, Sun, Mar, 1, 60};
                return new Timezone(WAT);
            }

            if (timeZoneName == "CET") {
                // UTC+1 Central European Time (with DST)
                TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};
                TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};
                return new Timezone(CEST, CET);
            }

            if (timeZoneName == "CAT") {
                // UTC+2 Central Africa Time (no DST)
                TimeChangeRule CAT = {"CAT", Last, Sun, Mar, 1, 120};
                return new Timezone(CAT);
            }

            if (timeZoneName == "EET") {
                // UTC+2 Eastern European Time (with DST)
                TimeChangeRule EEST = {"EEST", Last, Sun, Mar, 2, 180};
                TimeChangeRule EET = {"EET ", Last, Sun, Oct, 3, 120};
                return new Timezone(EEST, EET);
            }

            if (timeZoneName == "MSK") {
                // UTC+3 Moscow Standard Time (no DST)
                TimeChangeRule MSK = {"MSK", Last, Sun, Mar, 1, 180};
                return new Timezone(MSK);
            }

            if (timeZoneName == "IRT") {
                // UTC+3:30 Iran Standard Time (no DST)
                TimeChangeRule IRT = {"IRT", Last, Sun, Mar, 1, 210};
                return new Timezone(IRT);
            }

            if (timeZoneName == "SAMT") {
                // UTC+4 Samara Standard Time (no DST)
                TimeChangeRule SAMT = {"SAMT", Last, Sun, Mar, 1, 240};
                return new Timezone(SAMT);
            }

            if (timeZoneName == "AFT") {
                // UTC+4:30 Afghanistan Standard Time (no DST)
                TimeChangeRule AFT = {"AFT", Last, Sun, Mar, 1, 270};
                return new Timezone(AFT);
            }

            if (timeZoneName == "PAT") {
                // UTC+5 Pakistan/Yekaterinburg Standard Time (no DST)
                TimeChangeRule PAT = {"PAT", Last, Sun, Mar, 1, 300};
                return new Timezone(PAT);
            }

            if (timeZoneName == "IST") {
                // UTC+5:30 India Standard Time (no DST)
                TimeChangeRule IST = {"IST", Last, Sun, Mar, 1, 330};
                return new Timezone(IST);
            }

            if (timeZoneName == "NST") {
                // UTC+5:45 Nepal Standard Time (no DST)
                TimeChangeRule NST = {"NST", Last, Sun, Mar, 1, 345};
                return new Timezone(NST);
            }

            if (timeZoneName == "BST") {
                // UTC+6 Bangladesh/Omsk Standard Time (no DST)
                TimeChangeRule BST = {"BST", Last, Sun, Mar, 1, 360};
                return new Timezone(BST);
            }

            if (timeZoneName == "MMT") {
                // UTC+6:30 Myanmar Standard Time (no DST)
                TimeChangeRule MMT = {"MMT", Last, Sun, Mar, 1, 390};
                return new Timezone(MMT);
            }

            if (timeZoneName == "ICT") {
                // UTC+7 Indochina/Krasnoyarsk Standard Time (no DST)
                TimeChangeRule ICT = {"ICT", Last, Sun, Mar, 1, 420};
                return new Timezone(ICT);
            }

            if (timeZoneName == "CST") {
                // UTC+8 China/Australian Western/Irkutsk Standard Time (no DST)
                TimeChangeRule CST = {"CST", Last, Sun, Mar, 1, 480};
                return new Timezone(CST);
            }

            if (timeZoneName == "AWCST") {
                // UTC+8:45 Australian Western Central Standard Time (no DST)
                TimeChangeRule AWCST = {"AWCST", Last, Sun, Mar, 1, 525};
                return new Timezone(AWCST);
            }

            if (timeZoneName == "JST") {
                // UTC+9 Japan/Korea/Yakutsk Standard Time (no DST)
                TimeChangeRule JST = {"JST", Last, Sun, Mar, 1, 540};
                return new Timezone(JST);
            }

            if (timeZoneName == "ACST") {
                // UTC+9:30 Australian Central Standard Time (no DST)
                TimeChangeRule ACST = {"ACST", Last, Sun, Mar, 1, 570};
                return new Timezone(ACST);
            }

            if (timeZoneName == "ACDT") {
                // UTC+9:30 Australian Central Standard Time (with DST)
                TimeChangeRule ACDT = {"ACDT", First, Sun, Oct, 2, 630};
                TimeChangeRule ACSTx = {"ACSTx", First, Sun, Apr, 3, 570};
                return new Timezone(ACDT, ACSTx);
            }

            if (timeZoneName == "AEST") {
                // UTC+10 Australian Eastern/Vladivostok Standard Time (no DST)
                TimeChangeRule AEST = {"AEST", Last, Sun, Mar, 1, 600};
                return new Timezone(AEST);
            }

            if (timeZoneName == "AEDT") {
                // UTC+10 Australian Eastern Standard Time (with DST)
                TimeChangeRule AEDT = {"AEDT", First, Sun, Oct, 2, 660};
                TimeChangeRule AESTx = {"AESTx", First, Sun, Apr, 3, 600};
                return new Timezone(AEDT, AESTx);
            }

            if (timeZoneName == "MAGT") {
                // UTC+11 Magadan Standard Time (no DST)
                TimeChangeRule MAGT = {"MAGT", Last, Sun, Mar, 1, 660};
                return new Timezone(MAGT);
            }

            if (timeZoneName == "PETT") {
                // UTC+12 Kamchatka Standard Time (no DST)
                TimeChangeRule PETT = {"PETT", Last, Sun, Mar, 1, 720};
                return new Timezone(PETT);
            }

            if (timeZoneName == "NZST") {
                // UTC+12 New Zealand Standard Time (with DST)
                TimeChangeRule NZDT = {"NZDT", Last, Sun, Sep, 2, 780};
                TimeChangeRule NZST = {"NZST", First, Sun, Apr, 3, 720};
                return new Timezone(NZDT, NZST);
            }

            if (timeZoneName == "NZCST") {
                // UTC+13:30 New Zealand Chatham Standard Time (with DST)
                // Unsure if I could get code right since daylight saving time runs from the last Sunday in September at 2:45 to the first Sunday in April at 3:45
                // So fudged for 3 and 4 am...
                TimeChangeRule NZCDT = {"NZCDT", Last, Sun, Sep, 3, 825};
                TimeChangeRule NZCST = {"NZCST", First, Sun, Apr, 4, 765};
                return new Timezone(NZCDT, NZCST);
            }

            if (timeZoneName == "KST") {
                // UTC+14 Kiribati Standard Time (no DST)
                TimeChangeRule KST = {"KST", Last, Sun, Mar, 1, 840};
                return new Timezone(KST);
            }

            if (timeZoneName == "SST") {
                // UTC-11 Samoa Standard Time (no DST)
                TimeChangeRule SST = {"SST", Last, Sun, Mar, 1, -660};
                return new Timezone(SST);
            }

            if (timeZoneName == "HST") {
                // UTC-10 Hawaii–Aleutian Standard Time (no DST)
                TimeChangeRule HST = {"HST", Last, Sun, Mar, 1, -600};
                return new Timezone(HST);
            }

            if (timeZoneName == "AKST") {
                // UTC-10 Alaska Standard Time (with DST)
                TimeChangeRule AKDT = {"AKDT", Second, Sun, Mar, 2, -540};
                TimeChangeRule AKST = {"AKST", First, Sun, Nov, 2, -600};
                return new Timezone(AKDT, AKST);
            }

            if (timeZoneName == "FPMST") {
                // UTC-9:30 French Polynesia Marquesas Islands Standard Time (no DST)
                TimeChangeRule FPMST = {"FPMST", Last, Sun, Mar, 1, -570};
                return new Timezone(FPMST);
            }

            if (timeZoneName == "FPGST") {
                // UTC-9 French Polynesia Marquesas Islands Standard Time (no DST)
                TimeChangeRule FPGST = {"FPGST", Last, Sun, Mar, 1, -540};
                return new Timezone(FPGST);
            }

            if (timeZoneName == "UKPST") {
                // UTC-8 UK Pitcairn Islands Standard Time (no DST)
                TimeChangeRule UKPST = {"UKPST", Last, Sun, Mar, 1, -480};
                return new Timezone(UKPST);
            }

            if (timeZoneName == "PST") {
                // UTC-8 Pacific Standard Time (with DST)
                TimeChangeRule PDT = {"PDT", Second, Sun, Mar, 2, -420};
                TimeChangeRule PST = {"PST", First, Sun, Nov, 2, -480};
                return new Timezone(PDT, PST);
            }

            if (timeZoneName == "AZT") {
                // UTC-7 Arizona Standard Time (no DST)
                TimeChangeRule AZT = {"AZT", First, Sun, Nov, 2, -420};
                return new Timezone(AZT);
            }

            if (timeZoneName == "MST") {
                // UTC-7 Mountain Standard Time (with DST)
                TimeChangeRule MDT = {"MDT", Second, Sun, Mar, 2, -360};
                TimeChangeRule MST = {"MST", First, Sun, Nov, 2, -420};
                return new Timezone(MDT, MST);
            }

            if (timeZoneName == "SKST") {
                // UTC-6 Saskatchewan Standard Time (no DST)
                TimeChangeRule SKST = {"SKST", First, Sun, Nov, 2, -360};
                return new Timezone(SKST);
            }

            if (timeZoneName == "CST") {
                // UTC-6 Central Time (with DST)
                TimeChangeRule CDT = {"CDT", Second, Sun, Mar, 2, -300};
                TimeChangeRule CST = {"CST", First, Sun, Nov, 2, -360};
                return new Timezone(CDT, CST);
            }

            if (timeZoneName == "CAST") {
                // UTC-5 Caribbean/Nunavut Standard Time (no DST)
                TimeChangeRule CAST = {"CAST", First, Sun, Nov, 2, -300};
                return new Timezone(CAST);
            }

            if (timeZoneName == "EST") {
                // UTC-5 Eastern Time (with DST)
                TimeChangeRule EDT = {"EDT", Second, Sun, Mar, 2, -240};
                TimeChangeRule EST = {"EST", First, Sun, Nov, 2, -300};
                return new Timezone(EDT, EST);
            }

            if (timeZoneName == "ECST") {
                // UTC-4 Eastern Caribbean Standard Time (no DST)
                TimeChangeRule ECST = {"ECST", First, Sun, Nov, 2, -240};
                return new Timezone(ECST);
            }

            if (timeZoneName == "AST") {
                // UTC-4 Atlantic Standard Time (with DST)
                TimeChangeRule ADT = {"ADT", Second, Sun, Mar, 2, -180};
                TimeChangeRule AST = {"AST", First, Sun, Nov, 2, -240};
                return new Timezone(ADT, AST);
            }

            if (timeZoneName == "NFT") {
                // UTC-3:30 Newfoundland Standard Time (with DST)
                TimeChangeRule NFDT = {"NFDT", Second, Sun, Mar, 2, -150};
                TimeChangeRule NFT = {"NFT", First, Sun, Nov, 2, -210};
                return new Timezone(NFDT, NFT);
            }

            if (timeZoneName == "BRT") {
                // UTC-3 Brasilia Time (no DST)
                TimeChangeRule BRT = {"BRT", First, Sun, Nov, 2, -180};
                return new Timezone(BRT);
            }

            if (timeZoneName == "WGT") {
                // UTC-2 West Greenland Time (with DST) (DST possibly incorrect)
                TimeChangeRule WGST = {"WGST", Last, Sun, Mar, 2, -180};
                TimeChangeRule WGT = {"WGT ", Last, Sun, Oct, 3, -120};
                return new Timezone(WGST, WGT);
            }

            if (timeZoneName == "EGT") {
                // UTC-1 East Greenland Time (with DST) (DST possibly incorrect)
                TimeChangeRule EGST = {"EGST", Last, Sun, Mar, 2, -120};
                TimeChangeRule EGT = {"EGT ", Last, Sun, Oct, 3, -60};
                return new Timezone(EGST, EGT);
            }

            if (timeZoneName == "PAIT") {
                // UTC0 Portugal Azores Islands Time (with DST) (DST possibly incorrect)
                TimeChangeRule PAIST = {"PAIST", Last, Sun, Mar, 2, -60};
                TimeChangeRule PAIT = {"PAIT ", Last, Sun, Oct, 3, 0};
                return new Timezone(PAIST, PAIT);
            }
            
            // UTC
            TimeChangeRule utcRule = {"UTC", Last, Sun, Mar, 1, 0};     // UTC
            return new Timezone(utcRule);
        }
};
