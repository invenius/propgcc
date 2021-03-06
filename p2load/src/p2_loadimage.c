#include <stdio.h>
#include <string.h>
#include "p2_loadimage.h"
#include "osint.h"

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

/* maximum packet size for the second-stage loader */
#define PKTMAXLEN       1024

/* clock frequency (for Propeller 2 FPGA boards) */
#define CLOCK_FREQ      80000000

/* offset of signature in image */
#define SIG_OFFSET      0x1f8

/* hub memory buffer for writing files to flash */
#define FLASH_BUF_START 0x1000
#define FLASH_BUF_SIZE  (16 * 1024)

/* size of a COG image */
#define COG_IMAGE_SIZE  (512 * sizeof(uint32_t))

/* second-stage loader header structure */
typedef struct {
    uint32_t jmpinit;
    uint32_t period;    // clkfreq / baudrate
} Stage2Hdr;

static void p2_CreateSignedImage(int baudRate, uint8_t *image, uint8_t *array, int size);

static uint8_t txbuf[1024];
static int txcnt;
static uint8_t rxbuf[1024];
static int rxnext, rxcnt;
static uint8_t lfsr;

static void SerialInit(void);
static void TByte(uint8_t x);
static void TLong(uint32_t x);
static void TComm(void);
static int RBit(int timeout);
static int IterateLFSR(void);

static int WaitForInitialAck(void);
static int SendPacket(uint8_t *buf, int len);
static int WaitForAckNak(int timeout);

/* p2_InitLoader - initialize the loader */
int p2_InitLoader(int baudRate)
{
    extern uint8_t loader_array[];
    extern int loader_size;
    uint8_t loader_image[COG_IMAGE_SIZE];
    int i;

    /* create a signed image for the ROM loader */
    p2_CreateSignedImage(baudRate, loader_image, loader_array, loader_size);
        
    /* download the second-stage loader binary */
    for (i = 0; i < sizeof(loader_image); i += 4)
        TLong( loader_image[i    ]
            | (loader_image[i + 1] << 8)
            | (loader_image[i + 2] << 16)
            | (loader_image[i + 3] << 24));
    TComm();
    
    /* wait for the loader to start */
    msleep(100);
    
    /* wait for the loader to be ready */
    if (!WaitForInitialAck()) {
        printf("error: packet handshake failed\n");
        return 1;
    }
    
    /* return successfully */
    return 0;
}

/* p2_CreateSignedImage - create a signed COG image */
static void p2_CreateSignedImage(int baudRate, uint8_t *image, uint8_t *array, int size)
{
    Stage2Hdr *hdr;
    uint32_t *ptr;
    int i;

    /* build the loader image */
    memset(image, 0, COG_IMAGE_SIZE);
    memcpy(image, array, size);
        
    /* patch the binary loader with the baud rate information */
    hdr = (Stage2Hdr *)image;
    hdr->period = CLOCK_FREQ / baudRate;
    
    /* add the (dummy) signature */
    ptr = (uint32_t *)image;
    for (i = 0; i < 8; ++i)
        ptr[SIG_OFFSET + i] = 0x00000001;
}

/* p2_LoadImage - load a binary hub image into a propeller 2 */
int p2_LoadImage(uint8_t *imageBuf, uint32_t addr, uint32_t size)
{
    LoadCmd loadCmd;

    /* send the load command */
    loadCmd.cmd = CMD_LOAD;
    loadCmd.addr = addr;
    loadCmd.count = size;
    if (!SendPacket((uint8_t *)&loadCmd, sizeof(loadCmd))) {
        printf("error: send load packet failed\n");
        return 1;
    }
    
    /* send the data */
    return p2_SendBuffer(imageBuf, size);
}
    
/* p2_SendBuffer - send a buffer to the helper */
int p2_SendBuffer(uint8_t *imageBuf, uint32_t size)
{
    uint8_t *ptr;
    int cnt;

    /* load the binary image */
    for (ptr = imageBuf; (cnt = size) > 0; ptr += cnt) {
        if (cnt > PKTMAXLEN)
            cnt = PKTMAXLEN;
        if (!SendPacket(ptr, cnt)) {
            printf("error: send data packet failed\n");
            return 1;
        }
        size -= cnt;
        printf(".");
        fflush(stdout);
    }
    printf("\n");
    
    /* return successfully */
    return 0;
}

/* p2_StartImage - start the loaded image */
int p2_StartImage(int id, uint32_t addr, uint32_t param)
{    
    StartCmd startCmd;

    /* send the start command */
    startCmd.cmd = (id << 8) | CMD_START;
    startCmd.addr = addr;
    startCmd.param = param;
    if (!SendPacket((uint8_t *)&startCmd, sizeof(startCmd))) {
        printf("error: send start packet failed\n");
        return 1;
    }

    /* return successfully */
    return 0;
}

/* p2_StartCog - start a COG image */
int p2_StartCog(int id, uint32_t addr, uint32_t param)
{    
    StartCmd startCmd;

    /* send the coginit command */
    startCmd.cmd = (id << 8) | CMD_COGINIT;
    startCmd.addr = addr;
    startCmd.param = param;
    if (!SendPacket((uint8_t *)&startCmd, sizeof(startCmd))) {
        printf("error: send coginit packet failed\n");
        return 1;
    }

    /* return successfully */
    return 0;
}

/* p2_Flash - write data from hub memory to flash */
int p2_Flash(uint32_t flashaddr, uint32_t hubaddr, uint32_t count)
{    
    FlashCmd flashCmd;

    /* send the flash command */
    flashCmd.cmd = (count << 8) | CMD_FLASH;
    flashCmd.flashaddr = flashaddr;
    flashCmd.hubaddr = hubaddr;
    if (!SendPacket((uint8_t *)&flashCmd, sizeof(flashCmd))) {
        printf("error: send flash packet failed\n");
        return 1;
    }

    /* return successfully */
    return 0;
}

/* p2_FlashBooter - write a boot image to flash */
int p2_FlashBooter(int baudRate)
{
    extern uint8_t booter_array[];
    extern int booter_size;
    uint8_t booter_image[COG_IMAGE_SIZE];
    int i;

    /* create a signed image for the ROM loader */
    p2_CreateSignedImage(baudRate, booter_image, booter_array, booter_size);
    
    /* make it big-endian for the rom loader */
    for (i = 0; i < sizeof(booter_image); i += 4) {
        int tmp = booter_image[i];
        booter_image[i] = booter_image[i + 3];
        booter_image[i + 3] = tmp;
        tmp = booter_image[i + 1];
        booter_image[i + 1] = booter_image[i + 2];
        booter_image[i + 2] = tmp;
    }

    /* write the booter to flash */
    return p2_FlashBuffer(booter_image, sizeof(booter_image), 0, FALSE);
}

/* p2_FlashBuffer - write data from a file to flash using a hub memory buffer */
int p2_FlashBuffer(uint8_t *buffer, int size, uint32_t flashaddr, int progress)
{
    int remaining, cnt;
    LoadCmd loadCmd;
    FlashCmd flashCmd;
    uint8_t *ptr;

    /* send the load command */
    loadCmd.cmd = CMD_LOAD;
    loadCmd.addr = FLASH_BUF_START;
    loadCmd.count = size;
    if (!SendPacket((uint8_t *)&loadCmd, sizeof(loadCmd))) {
        printf("error: send load packet failed\n");
        return 1;
    }

    /* load the binary image */
    for (ptr = buffer, remaining = size; (cnt = remaining) > 0; ptr += cnt) {
        if (cnt > PKTMAXLEN)
            cnt = PKTMAXLEN;
        if (!SendPacket(ptr, cnt)) {
            printf("error: send data packet failed\n");
            return 1;
        }
        remaining -= cnt;
        if (progress) {
            printf(".");
            fflush(stdout);
        }
    }
    if (progress)
        printf("\n");
    
    /* send the flash command */
    flashCmd.cmd = (size << 8) | CMD_FLASH;
    flashCmd.flashaddr = flashaddr;
    flashCmd.hubaddr = FLASH_BUF_START;
    if (!SendPacket((uint8_t *)&flashCmd, sizeof(flashCmd))) {
        printf("error: send flash packet failed\n");
        return 1;
    }

    /* return successfully */
    return 0;
}

/* p2_FlashFile - write data from a file to flash using a hub memory buffer */
int p2_FlashFile(char *file, uint32_t flashaddr)
{
    uint8_t buf[FLASH_BUF_SIZE];
    int size, err;
    FILE *fp;

    if (!(fp = fopen(file, "rb"))) {
        printf("error: can't open '%s'\n", file);
        return 1;
    }
    
    /* load the binary image */
    while ((size = fread(buf, 1, sizeof(buf), fp)) > 0) {
        if ((err = p2_FlashBuffer(buf, size, flashaddr, TRUE)) != 0)
            return err;
        flashaddr += size;
    }
    printf("\n");
    
    /* close the file */
    fclose(fp);
    
    /* return successfully */
    return 0;
}

/* this code is adapted from Chip Gracey's PNut IDE */

int p2_HardwareFound(int *pVersion)
{
    int version, i;
    
    /* reset the propeller */
    hwreset();
    
    /* initialize the serial buffers */
    SerialInit();
    
    /* send the connect string + blanks for echoes */
    TByte(0xf9);
    lfsr = 'P';
    for (i = 0; i < 250; ++i)
        TByte(IterateLFSR() | 0xfe);
    for (i = 0; i < 250 + 8; ++i)
        TByte(0xf9);
    TComm();
    
    /* receive the connect string */
    for (i = 0; i < 250; ++i)
        if (RBit(100) != IterateLFSR()) {
            printf("error: hardware lost\n");
            return FALSE;
        }
        
    /* receive the chip version */
    for (version = i = 0; i < 8; ++i) {
        int bit = RBit(50);
        if (bit < 0) {
            printf("error: hardware lost\n");
            return FALSE;
        }
        version = ((version >> 1) & 0x7f) | (bit << 7);
    }
    *pVersion = version;
        
    /* return successfully */
    return TRUE;
}

/* SerialInit - initialize the serial buffers */
static void SerialInit(void)
{
    txcnt = rxnext = rxcnt = 0;
}

/* TByte - add a byte to the transmit buffer */
static void TByte(uint8_t x)
{
    txbuf[txcnt++] = x;
    if (txcnt >= sizeof(txbuf))
        TComm();
}

/* TLong - add a long to the transmit buffer */
static void TLong(uint32_t x)
{
    int i;
    for (i = 0; i < 11; ++i) {
        TByte(0x92
            | ((i == 10 ? -1 : 0) & 0x60)
            | ((x >> 31) & 1)
            | (((x >> 30) & 1) << 3)
            | (((x >> 29) & 1) << 6));
        x <<= 3;
    }
}

/* TComm - write the transmit buffer to the port */
static void TComm(void)
{
    tx(txbuf, txcnt);
    txcnt = 0;
}

/* RBit - receive a bit with a timeout */
static int RBit(int timeout)
{
    int result;
    for (;;) {
        if (rxnext >= rxcnt) {
            rxcnt = rx_timeout(rxbuf, sizeof(rxbuf), timeout);
            if (rxcnt <= 0) {
                /* hardware lost */
                return -1;
            }
            rxnext = 0;
        }
        result = rxbuf[rxnext++] - 0xfe;
        if ((result & 0xfe) == 0)
            return result;
        /* checksum error */
    }
}

/* IterateLFSR - get the next bit in the lfsr sequence */
static int IterateLFSR(void)
{
    int result = lfsr & 1;
    lfsr = ((lfsr << 1) & 0xfe) | (((lfsr >> 7) ^ (lfsr >> 5) ^ (lfsr >> 4) ^ (lfsr >> 1)) & 1);
    return result;
}

/* end of code adapted from Chip Gracey's PNut IDE */
/* timeouts for waiting for ACK/NAK */
#define INITIAL_TIMEOUT     10000   // 10 seconds
#define PACKET_TIMEOUT      10000   // 10 seconds - this is long because SD cards may take a file to scan the FAT

/* packet format: SOH length-lo length-hi hdrchk length*data crc1 crc2 */
#define HDR_SOH     0
#define HDR_LEN_HI  1
#define HDR_LEN_LO  2
#define HDR_CHK     3

/* packet header and crc lengths */
#define PKTHDRLEN   4
#define PKTCRCLEN   2

/* maximum length of a frame */
#define FRAMELEN    (PKTHDRLEN + PKTMAXLEN + PKTCRCLEN)

/* protocol characters */
#define SOH     0x01    /* start of a packet */
#define ACK     0x06    /* positive acknowledgement */
#define NAK     0x15    /* negative acknowledgement */

#define updcrc(crc, ch) (crctab[((crc) >> 8) & 0xff] ^ ((crc) << 8) ^ (ch))

static const uint16_t crctab[256] = {
    0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
    0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
    0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
    0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
    0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
    0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
    0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
    0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
    0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
    0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
    0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
    0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
    0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
    0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
    0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
    0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
    0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
    0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
    0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
    0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
    0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
    0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
    0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
    0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
    0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
    0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
    0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
    0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
    0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
    0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
    0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
    0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
};

static int WaitForInitialAck(void)
{
    return WaitForAckNak(INITIAL_TIMEOUT) == ACK;
}

static int SendPacket(uint8_t *buf, int len)
{
    uint8_t hdr[PKTHDRLEN], crc[PKTCRCLEN], *p;
    uint16_t crc16 = 0;
    int cnt, ch;

    /* setup the frame header */
    hdr[HDR_SOH] = SOH;                                 /* SOH */
    hdr[HDR_LEN_HI] = (uint8_t)(len >> 8);              /* data length - high byte */
    hdr[HDR_LEN_LO] = (uint8_t)len;                     /* data length - low byte */
    hdr[HDR_CHK] = hdr[1] + hdr[2];                     /* header checksum */

    /* compute the crc */
    for (p = buf, cnt = len; --cnt >= 0; ++p)
        crc16 = updcrc(crc16, *p);
    crc16 = updcrc(crc16, '\0');
    crc16 = updcrc(crc16, '\0');

    /* add the crc to the frame */
    crc[0] = (uint8_t)(crc16 >> 8);
    crc[1] = (uint8_t)crc16;

    /* send the packet */
    tx(hdr, PKTHDRLEN);
    if (len > 0)
        tx(buf, len);
    tx(crc, PKTCRCLEN);

    /* wait for an ACK/NAK */
    if ((ch = WaitForAckNak(PACKET_TIMEOUT)) < 0) {
        printf("Timeout waiting for ACK/NAK\n");
        ch = NAK;
    }

    /* return status */
    return ch == ACK;
}

static int WaitForAckNak(int timeout)
{
    uint8_t buf[1];
    return rx_timeout(buf, 1, timeout) == 1 ? buf[0] : -1;
}
