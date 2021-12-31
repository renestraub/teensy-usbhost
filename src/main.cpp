/*
 * Teensy 4.1 USB Host Tests
 *
 * Uses uSDFS with modifications for Platform IO
 *   Core 5.2.4·Home 3.4.0
 *
 * Filesystem API see http://elm-chan.org/fsw/ff/00index_e.html
 */
#include <Arduino.h>
#include <USBHost_t36.h>    // USBHost, USBHub, etc.
#include "uSDFS.h"          // Filesystem wrapper


USBHost myusb;
USBHub hub1(myusb);
msController msDrive1(myusb);

/* 
 * Select interface
 * See http://elm-chan.org/fsw/ff/doc/filename.html and
 * uSDFS.h -> PARTITION VolToPart[]
 */
// const char *Dev = "0:/"; // SPI
// const char *Dev = "1:/"; // SDHC
static const char *device = "2:/"; // USB Host Port, Auto Detect Partition
static FATFS fatfs; /* File system object */

static char testfile_path[] = "/testfiles/128MBrandom.bin";
static uint32_t test_size = 16*1024*1024;


static void usage();
static void test_list();
static void test_largefile();

//-----------------------------------------------------------------------------

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  analogWrite(LED_BUILTIN, 20);

  // Wait for console connection
  while (!Serial) {}

  Serial.println("Setting up USB host");
  myusb.begin();

  usage();
}

//-----------------------------------------------------------------------------

void loop()
{
  if (Serial.available() > 0) {
    auto incomingByte = Serial.read();

    // Serial.print("I received: ");
    // Serial.println(incomingByte, DEC);

    // Flush all other bytes received
    if (Serial.available() > 0) {
      (void)Serial.read();
    }

    analogWrite(LED_BUILTIN, 200);

    switch (incomingByte)
    {
      case 'h':
        usage();
        break;

      case '1':
        test_list();
        break;

      case '2':
        test_largefile();
        break;

      default:
        usage();
    }

    analogWrite(LED_BUILTIN, 20);
  }
}

//-----------------------------------------------------------------------------

static void usage()
{
  Serial.println("Teensy USB Host Experiments");
  Serial.println(" h: show this help page");
  Serial.println(" 1: list files/folders");
  Serial.println(" 2: usb speed test");
}

//-----------------------------------------------------------------------------

static void die(const char *text, FRESULT rc)
{
  Serial.printf("%s: Failed with rc=%s.\r\n", text, FR_ERROR_STRING[rc]);
  Serial.println("Please reboot...");
  while (true) {
    asm("wfi");
  }
}

//-----------------------------------------------------------------------------

static void mount()
{
  FRESULT rc;

  Serial.print("Mounting drive ");
  Serial.println(device);

  if ((rc = f_mount(&fatfs, device, 1))) {
    die("Mount", rc); /* Mount/Unmount a logical drive */
  }

  // Serial.println();
  Serial.println("Changing drive");
  if ((rc = f_chdrive(device)))
    die("chdrive", rc);
}

//-----------------------------------------------------------------------------

static void unmount()
{
  FRESULT rc;

  // Serial.println();
  Serial.printf("Unmounting %s\n", device);
  if ((rc = f_unmount(device)))
    die("Unmount", rc); /* Mount/Unmount a logical drive */
}

//-----------------------------------------------------------------------------

static void test_list()
{
  FRESULT rc;

  mount();

  // Serial.println();
  // Serial.println("Root directory");

  DIR d;
  FILINFO fi;

  rc = f_findfirst(&d, &fi, "/", "*");
  while (rc == FR_OK && fi.fname[0]) {         /* Repeat while an item is found */
    // Serial.printf("  %s %d\n", fi.fname, fi.fsize);
    if (fi.fattrib & AM_DIR) {
      Serial.printf("         <DIR> %s\n", fi.fname);
    }
    else {
      Serial.printf("  %12lld %s\n", fi.fsize, fi.fname);
    }
    rc = f_findnext(&d, &fi);
  }
  f_closedir(&d);

  unmount();
}

//-----------------------------------------------------------------------------

static void test_largefile()
{
  // static TCHAR buff[1024];
  static TCHAR buff[8192];
  FRESULT rc;

  mount();

  Serial.println();
  Serial.println("Measuring read performance");

  FIL fil;

  if ((rc = f_open(&fil, testfile_path, FA_READ))) {
    die("Open", rc);
  }

  auto start = millis();
  uint32_t read = 0;
  while (read < test_size)
  {
    UINT br = 0;

    f_read(&fil, buff, sizeof(buff), &br);
    if (br == sizeof(buff))
    {
      read += sizeof(buff);
    }
    else
    {
      Serial.println("Read error");
      break;
    }
  }
  auto end = millis();
  float duration = (end - start) / 1000.0;
  Serial.printf("Read: %d bytes\n", read);
  Serial.printf("Time: %d ms, %.0f bps\n", end-start, (float)read/duration);

  Serial.println();
  Serial.println("Closing file");
  if ((rc = f_close(&fil))) {
    die("Close", rc);
  }

  unmount();
}

//-----------------------------------------------------------------------------

/*
 * MSC access wrapper.
 * This replaces the code in sd_msc.cpp of uSDFS that doesn't compile
 * in my environment
 */

#include <diskio.h>

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
