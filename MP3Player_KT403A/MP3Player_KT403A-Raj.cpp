/*
 * MP3Player_KT403A-Raj.cpp
 *
 * A library for Grove-Serial MP3 Player V2.0
 * Modified for Nabby (Nabaztag) on 28/11/2019 by Raj
 * Note: the parameter boundary checking can be stretched if needed.
 *
 * Copyright (c) 2015 seeed technology inc.
 * Website    : www.seeed.cc
 * Author     : Wuruibin
 * Created Time: Dec 2015
 * 
 The MIT License (MIT)

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
 
#include <Arduino.h>
#include "MP3Player_KT403A-Raj.h"


/**************************************************************** 
 * Function Name: SelectPlayerDevice
 * Description: Select the player device, U DISK or SD card.
 * Parameters: 0x01:U DISK;  0x02:SD card
 * Return: none
****************************************************************/ 
void SelectPlayerDevice(uint8_t device)
{
    if (device < 0) device = 0;
    if (device > 2) device = 2;
    Serial2.write(0x7E);
    Serial2.write(0xFF);
    Serial2.write(0x06);
    Serial2.write(0x09);
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(device);
    Serial2.write(0xEF);
    delay(200);
}

/**************************************************************** 
 * Function Name: SpecifyMusicPlay
 * Description: Specify the music index to play, the index is decided by the input sequence of the music.
 * Parameters: index: the music index: 0-65535.
 * Return: none
****************************************************************/ 
void SpecifyMusicPlay(uint16_t index)
{
    if (index < 0) index = 0;
    if (index > 1000) index = 1000;
    uint8_t hbyte, lbyte;
    hbyte = index / 256;
    lbyte = index % 256;
    Serial2.write(0x7E);
    Serial2.write(0xFF);
    Serial2.write(0x06);
    Serial2.write(0x03);
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(hbyte));
    Serial2.write(uint8_t(lbyte));
    Serial2.write(0xEF);
    delay(10);
//  return true;
}

/**************************************************************** 
 * Function Name: SpecifyfolderPlay
 * Description: Specify the music index in the folder to play, the index is decided by the input sequence of the music.
 * Parameters: folder: folder name, must be number;  index: the music index.
 * Return: none
****************************************************************/ 
void SpecifyfolderPlay(uint8_t folder, uint8_t index)
{
    if (folder < 0) folder = 0;
    if (folder > 100) folder = 100;
    if (index < 0) index = 0;
    if (index > 100) index = 100;
    Serial2.write(0x7E);
    Serial2.write(0xFF);
    Serial2.write(0x06);
    Serial2.write(0x0F);
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(folder));
    Serial2.write(uint8_t(index));
    Serial2.write(0xEF);
    delay(10);
//  return true;
}

/**************************************************************** 
 * Function Name: PlayPause
 * Description: Pause the MP3 player.
 * Parameters: none
 * Return: none
****************************************************************/ 
void PlayPause(void)
{
    Serial2.write(0x7E);
    Serial2.write(0xFF);
    Serial2.write(0x06);
    Serial2.write(0x0E);
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
//  Serial2.write(0xFE);
//  Serial2.write(0xED);
    Serial2.write(0xEF);
    delay(20);
//  return true;
}

/**************************************************************** 
 * Function Name: PlayResume
 * Description: Resume the MP3 player.
 * Parameters: none
 * Return: none
****************************************************************/ 
void PlayResume(void)
{
    Serial2.write(0x7E);
    Serial2.write(0xFF);
    Serial2.write(0x06);
    Serial2.write(0x0D);
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
//  Serial2.write(0xFE);
//  Serial2.write(0xEE);
    Serial2.write(0xEF);
    delay(20);
//  return true;
}

/**************************************************************** 
 * Function Name: PlayNext
 * Description: Play the next song.
 * Parameters: none
 * Return: none
****************************************************************/ 
void PlayNext(void)
{
    Serial2.write(0x7E);
    Serial2.write(0xFF);
    Serial2.write(0x06);
    Serial2.write(0x01);
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(0xEF);
    delay(10);
//  return true;
}

/**************************************************************** 
 * Function Name: PlayPrevious
 * Description: Play the previous song.
 * Parameters: none
 * Return: none
****************************************************************/ 
void PlayPrevious(void)
{
    Serial2.write(0x7E);
    Serial2.write(0xFF);
    Serial2.write(0x06);
    Serial2.write(0x02);
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(0xEF);
    delay(10);
//  return true;
}

/**************************************************************** 
 * Function Name: PlayLoop
 * Description: Play loop for all the songs.
 * Parameters: none
 * Return: none
****************************************************************/ 
void PlayLoop(void)
{
    Serial2.write(0x7E);
    Serial2.write(0xFF);
    Serial2.write(0x06);
    Serial2.write(0x11);
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(0x01);
    Serial2.write(0xEF);
    delay(10);
//  return true;
}

/**************************************************************** 
 * Function Name: SetVolume
 * Description: Set the volume, the range is 0x00 to 0x1E.
 * Parameters: volume: the range is 0x00 to 0x1E.
 * Return: none
****************************************************************/ 
void SetVolume(uint8_t volume)
{ 
    if (volume < 0) volume = 0;
    if (volume > 31) volume = 31;
    Serial2.write(0x7E);
    Serial2.write(0xFF);
    Serial2.write(0x06);
    Serial2.write(0x06);
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(volume);
    Serial2.write(0xEF);
    delay(10);
//  return true;
}
/**************************************************************** 
 * Function Name:GetVolume
 * Description: Read the volume.
 * Parameters: none
 * Return: none
****************************************************************/ 
void GetVolume(void)
{
    Serial2.write(0x7E);
    Serial2.write(0xFF);
    Serial2.write(0x06);
    Serial2.write(0x43);
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(0xEF);
    delay(10);
//  return true;
}

/**************************************************************** 
 * Function Name: IncreaseVolume
 * Description: Increase the volume.
 * Parameters: none
 * Return: none
****************************************************************/ 
void IncreaseVolume(void)
{
    Serial2.write(0x7E);
    Serial2.write(0xFF);
    Serial2.write(0x06);
    Serial2.write(0x04);
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(0xEF);
    delay(10);
//  return true;
}

/**************************************************************** 
 * Function Name: DecreaseVolume
 * Description: Decrease the volume.
 * Parameters: none
 * Return: none
****************************************************************/ 
void DecreaseVolume(void)
{
    Serial2.write(0x7E);
    Serial2.write(0xFF);
    Serial2.write(0x06);
    Serial2.write(0x05);
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(0xEF);
    delay(10);
//  return true;
}

/**************************************************************** 
 * Function Name: SetEqualizer
 * Description: Sets the Equalizer, the range is 0x00 to 0x5.
 * Parameters: volume: the range is 0x00 to 0x1E.
 * Return: none
****************************************************************/ 
void SetEqualizer(uint8_t EqValue)
{
    if (EqValue < 0) EqValue = 0;
    if (EqValue > 5) EqValue = 5;
    Serial2.write(0x7E);
    Serial2.write(0xFF);
    Serial2.write(0x06);
    Serial2.write(0x07);
    Serial2.write(uint8_t(0x00));
    Serial2.write(uint8_t(0x00));
    Serial2.write(EqValue);
    Serial2.write(0xEF);
    delay(10);
//  return true;
}



/**************************************************************** 
 * Function Name: printReturnedData
 * Description: Print the returned data that sent from the Grove_Serial_MP3_Player.
 * Parameters: none
 * Return: none
****************************************************************/ 
void printReturnedData(void)
{
    unsigned char c;
    //check if there's any data sent from the Grove_Serial_MP3_Player
    while(Serial2.available())
    {
        c = Serial2.read();
        Serial.print("0x");
        Serial.print(c, HEX);
        Serial.print(" ");
    }
    Serial.println(" "); 
}

/**************************************************************** 
 * Function Name: QueryPlayStatus
 * Description: Query play status.
 * Parameters: none
 * Return: 0: played out; 1: other.
 * Usage: while(QueryPlayStatus() != 0);  // Waiting to play out.
****************************************************************/ 
uint8_t QueryPlayStatus(void)
{
    unsigned char c[10] = {0};
    uint8_t i = 0;
    //check if there's any data sent from the Grove_Serial_MP3_Player
    while(Serial2.available())
    {
        c[i] = Serial2.read();
        i++;
		delay(1);
		if (i == 10) break;
//        Serial.print(" 0x");
//        Serial.print(c[i], HEX);
    }
//    Serial.println(" "); 
    
    if(c[3] == 0x3C || c[3] == 0x3D || c[3] == 0x3E)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}