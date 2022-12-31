// Need to install NTPClient by Fabrice Weinberg
// Source for NTP with timezones: https://randomnerdtutorials.com/esp32-ntp-timezones-daylight-saving/
#include <WiFiUdp.h>
#include <NTPClient.h>

// Needs the time library: https://github.com/PaulStoffregen/Time
#include <time.h>

WiFiUDP ntpUDP;
extern NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000 * 30);  // Use the pool.ntp.org an update it every 30 minutes

extern bool readyForNtp = false;

// Task that runs as a separate "thread" to display the time on the TM1650 Segmented display
class NtpUpdateTask {
    public:
        NtpUpdateTask() {
            
        }
        
        void loop() {
            updateTime();
        }

        void initialise() {
            timeClient.begin();
        }

        void updateTime() {
            if (timeClient.update()) {
                Serial.println("NTP Time updated");

                unsigned long epoch = timeClient.getEpochTime();
                setTime(epoch);
                yield();
            }
        }
};
