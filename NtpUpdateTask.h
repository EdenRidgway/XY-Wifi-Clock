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
class NtpUpdateTask: public LeanTask {
    public:
        NtpUpdateTask(bool _enabled = false, unsigned long _interval = 0) : LeanTask(_enabled, _interval) {
            //WiFiUDP ntpUDP;
            //timeClient = new NTPClient(ntpUDP, "pool.ntp.org", 0, 60000 * 30);  // Use the pool.ntp.org an update it every 30 minutes
        }

        // void setWifiStatus(bool isConnected) {
        //     wifiConnected = isConnected;

        //     Serial.println("UpdateTimeTask.setWifiStatus");
        // }

    protected:
        void loop() {
            //if (!wifiConnected) return;
            
            if (!isSetup) {
                initialise();
            }

            updateTime();
        }

    private:
        bool isSetup = false;
        //bool wifiConnected = false;
        //NTPClient timeClient;

        void initialise() {
            timeClient.begin();
            isSetup = true;
        }

        void updateTime() {
            if (timeClient.update()) {
                Serial.println("NTP Time updated");

                unsigned long epoch = timeClient.getEpochTime();
                setTime(epoch);
            }
        }
};
