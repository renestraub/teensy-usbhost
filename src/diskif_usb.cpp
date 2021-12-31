/*
 * Teensy 4.1 USB Host Tests
 *
 * USB Host wrapper for uSDFS
 * This replaces the code in sd_msc.cpp of uSDFS that doesn't compile
 * in my environment
 */
#include <Arduino.h>
#include <USBHost_t36.h>    // USBHost, USBHub, etc.
#include <diskio.h>


// TODO: Provide via init method to this module
// extern msController msDrive1;

extern USBHost myusb;

msController msDrive1(myusb);


extern "C"
{
  int MSC_disk_initialize()
  {
    uint8_t mscError = 0;

    if ((mscError = msDrive1.mscInit()))
      Serial.printf(F("msDrive1 not connected: Code: %d\n\n"), mscError);
    // else
    //   Serial.printf(F("msDrive1  connected\n"));

    return mscError;
  }

  int MSC_disk_status()
  {
    // 	int stat = 0;
    // 	if(!deviceAvailable()) stat = STA_NODISK; 	// No USB Mass Storage Device Connected
    // 	if(!deviceInitialized()) stat = STA_NOINIT; // USB Mass Storage Device Un-Initialized
    // 	return stat;

    // Serial.println("MSC_disk_status");
    uint8_t mscError = 0;

    if ((mscError = msDrive1.checkConnectedInitialized()) != MS_INIT_PASS)
    {
      Serial.printf(F("msDrive1 not connected: Code: %d\n\n"), mscError);
      return STA_NODISK;
    }
    return 0;
  }

  int MSC_disk_read(BYTE *buff, DWORD sector, UINT count)
  {
    // Serial.printf("MSC_disk_read %d %d\n", sector, count);
    return msDrive1.msReadBlocks(sector, count, 512, buff);
  }

  int MSC_disk_write(const BYTE *buff, DWORD sector, UINT count)
  {
  // {	//WaitDriveReady();
  // 	return writeSectors((BYTE *)buff, sector, count);
  // }
    return RES_WRPRT;
  }
}
