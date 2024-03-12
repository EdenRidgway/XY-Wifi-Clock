#include "DS1307.h"


// GPIO pin for software I2C clock
int SCLpin = -1;

// GPIO pin for software I2C data
int SDApin = -1;


// I2C address of DS1307
const byte DS1307_ADDRESS = (0x68 << 1);


void DS1307_Setup(int scl, int sda)
{
    SCLpin = scl;
    SDApin = sda;
}


static void DS1307_Start()
{
    // we assume that the SDA/SCL pins are not in use by the TM1650
    // since the display update code can't run at the same time as
    // this clock chip code

    digitalWrite(SCLpin, LOW);
    DS1307_BitDelay();
    digitalWrite(SDApin, HIGH);
    DS1307_BitDelay();
    digitalWrite(SCLpin, HIGH);
    DS1307_BitDelay();
    digitalWrite(SDApin, LOW);
    DS1307_BitDelay();
    digitalWrite(SCLpin, LOW);
    DS1307_BitDelay();
}


static void DS1307_Stop()
{
    digitalWrite(SCLpin, LOW);
    DS1307_BitDelay();
    digitalWrite(SDApin, LOW);
    DS1307_BitDelay();
    digitalWrite(SCLpin, HIGH);
    DS1307_BitDelay();
    digitalWrite(SDApin, HIGH);
    DS1307_BitDelay();
    digitalWrite(SCLpin, LOW);
    DS1307_BitDelay();
    digitalWrite(SDApin, LOW);
    DS1307_BitDelay();
}


static bool DS1307_SendData(byte data)
{
    // send 8 bits of data, MSB first
    for (int i = 0; i < 8; i++)
    {
        // write the next bit
        digitalWrite(SDApin, ((data & 0x80) ? HIGH : LOW));
        DS1307_BitDelay();
        data <<= 1;

        // generate a clock
        digitalWrite(SCLpin, HIGH);
        DS1307_BitDelay();
        digitalWrite(SCLpin, LOW);
        DS1307_BitDelay();
    }

    // read the ACK
    digitalWrite(SDApin, HIGH);
    pinMode(SDApin, INPUT);
    DS1307_BitDelay();
    digitalWrite(SCLpin, HIGH);
    DS1307_BitDelay();
    byte ack = digitalRead(SDApin);
    digitalWrite(SCLpin, LOW);
    DS1307_BitDelay();
    pinMode(SDApin, OUTPUT);
    DS1307_BitDelay();

    //  0x00 = ACK, 0x01 = NACK
    return (ack == 0x00);
}


static byte DS1307_ReceiveData(bool lastByte)
{
    // initialize the return data
    byte data = 0x00;

    // set SDA to receive
    digitalWrite(SDApin, HIGH);
    pinMode(SDApin, INPUT);
    DS1307_BitDelay();

    // receive 8 bits of data, MSB first
    for (int i = 0; i < 8; i++)
    {
        data <<= 1;

        digitalWrite(SCLpin, HIGH);
        DS1307_BitDelay();

        if (digitalRead(SDApin))
        {
            data |= 0x01;
        }

        digitalWrite(SCLpin, LOW);
        DS1307_BitDelay();
    }

    // send the ACK
    pinMode(SDApin, OUTPUT);
    DS1307_BitDelay();
    digitalWrite(SDApin, (lastByte ? HIGH : LOW));
    DS1307_BitDelay();
    digitalWrite(SCLpin, HIGH);
    DS1307_BitDelay();
    digitalWrite(SCLpin, LOW);
    DS1307_BitDelay();

    return (data);
}


static void DS1307_BitDelay()
{
    // I2C runs at 100 kHz, 5 uSec is half a bit time
    delayMicroseconds(5);
}


void DS1307_ReadTime()
{
    // make sure DS1307_Setup() was called
    if ((SCLpin == -1)  ||  (SDApin == -1))
    {
        Serial.println("DS1307_Setup() was not called");
    }
    else
    {
        // DS1307 has 7 registers we are interested in
        byte registers[7];

        // number of bytes actually read with a successful ACK
        int numBytes = 0;

        // acknowledge from write to slave
        bool ack;

        // start the read
        DS1307_Start();

        // write slave address with the direction bit set to write
        ack = DS1307_SendData(DS1307_ADDRESS | 0x00);
        if (ack)
        {
            // write zero to DS1307 register pointer
            ack = DS1307_SendData(0x00);
        }
        if (ack)
        {
            // do a restart
            DS1307_Start();

            // write slave address with the direction bit set to read
            if (DS1307_SendData(DS1307_ADDRESS | 0x01))
            {
                // read the data
                bool lastByte;
                while (numBytes < sizeof(registers))
                {
                    lastByte = ((numBytes == (sizeof(registers) - 1)) ? true : false);
                    registers[numBytes++] = DS1307_ReceiveData(lastByte);
                }
            }
        }

        // stop the read
        DS1307_Stop();

        if (numBytes != sizeof(registers))
        {
            Serial.print("DS1307 read wrong number of bytes: ");
            Serial.println(numBytes);
        }
        else
        {
            // convert from DS1307 format to struct tm format
            struct tm now;
            now.tm_sec  = (10 * ((registers[0] & 0x70) >> 4)) + (registers[0] & 0x0F);
            now.tm_min  = (10 * ((registers[1] & 0x70) >> 4)) + (registers[1] & 0x0F);
            now.tm_hour = (10 * ((registers[2] & 0x30) >> 4)) + (registers[2] & 0x0F);
            now.tm_wday = (registers[3] & 0x07) - 1;
            now.tm_mday = (10 * ((registers[4] & 0x30) >> 4)) + (registers[4] & 0x0F);
            now.tm_mon  = (10 * ((registers[5] & 0x10) >> 4)) + (registers[5] & 0x0F) - 1;
            now.tm_year = (10 * ((registers[6] & 0xF0) >> 4)) + (registers[6] & 0x0F) + 100;
            now.tm_isdst = 0;

            // set the time in the ESP8285
            time_t t = mktime(&now);
            struct timeval newTime = { .tv_sec = t };
            settimeofday(&newTime, NULL);

            // read time to get possible DST correction
            time_t rawtime;
            time(&rawtime);
            now = *localtime(&rawtime);

            char buffer[80];
            strftime(buffer, sizeof(buffer), "DS1307 time update to %A, %d %B %Y, %H:%M:%S", &now);
            Serial.println(buffer);
        }
    }
}


void DS1307_WriteTime()
{
    // make sure DS1307_Setup() was called
    if ((SCLpin == -1)  ||  (SDApin == -1))
    {
        Serial.println("DS1307_Setup() was not called");
    }
    else
    {
        // DS1307 has 8 registers
        byte registers[8];

        // get the current time
        time_t rawtime;
        time(&rawtime);

        // check if it is DST
        struct tm now = *localtime(&rawtime);
        if (now.tm_isdst)
        {
            // if it is DST, subtract one hour's worth of seconds
            rawtime -= (1 * 60 * 60);

            // and adjust the current time
            now = *localtime(&rawtime);
        }

        // adjust format
        now.tm_wday += 1;
        now.tm_mon  += 1;
        now.tm_year -= 100;   // assume year is 2000 or after

        // setup data in DS1307 registers
        registers[0] = ((now.tm_sec  / 10) << 4) + (now.tm_sec  % 10);
        registers[1] = ((now.tm_min  / 10) << 4) + (now.tm_min  % 10);
        registers[2] = ((now.tm_hour / 10) << 4) + (now.tm_hour % 10);
        registers[3] = ((now.tm_wday / 10) << 4) + (now.tm_wday % 10);
        registers[4] = ((now.tm_mday / 10) << 4) + (now.tm_mday % 10);
        registers[5] = ((now.tm_mon  / 10) << 4) + (now.tm_mon  % 10);
        registers[6] = ((now.tm_year / 10) << 4) + (now.tm_year % 10);
        registers[7] = 0x03;

        // number of bytes actually sent with a successful ACK
        int numBytes = 0;

        // start the write
        DS1307_Start();

        // write slave address with the direction bit set to write
        if (DS1307_SendData(DS1307_ADDRESS | 0x00))
        {
            // write zero to DS1307 register pointer
            if (DS1307_SendData(0x00))
            {
                // write the data
                while ((numBytes < sizeof(registers))  &&  DS1307_SendData(registers[numBytes]))
                {
                    numBytes++;
                }
            }
        }

        // stop the write
        DS1307_Stop();

        // check result
        if (numBytes != sizeof(registers))
        {
            Serial.print("DS1307 wrote wrong number of bytes: ");
            Serial.println(numBytes);
        }
    }
}
