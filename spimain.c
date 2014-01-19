/*******************************************************************************
*
*   spimain.c of spincl
*
*   January 2013, Gary Marks (garym@ipsolutionscorp.com)
*
*   Copyright (c) 2013 iP Solutions Corp.
*
*   Description:
*   spincl is a command-line utility for executing spi commands with the 
*   Broadcom bcm2835.  It was developed and tested on a Raspberry Pi single-board
*   computer model B.  The utility is based on the bcm2835 C library developed
*   by Mike McCauley of Open System Consultants, http://www.open.com.au/mikem/bcm2835/.
*
*   Invoking spincl results in a full-duplex SPI transfer.  Options include the
*   the SPI clock frequency, SPI Mode, chip select designation, and chip select
*   polarity.  The command usage and command-line parameters are described below
*   in the showusage function, which prints the usage if no command-line parameters
*   are included or if there are any command-line parameter errors.  Invoking spincl 
*   requires root privilege.
*
*   This file contains the main function as well as functions for displaying
*   usage and for parsing the command line.
*
*   Open Source Licensing GNU GPLv3
*
*   History:
*   1/13    gtm VERSION 1.0.0: Original
*   1/13    gtm VERSION 1.1.0: SPI begin and end are now options.  This allows
*               the command to be used to start SPI (designate the SPI pins to
*               be used), end SPI (revert SPI pins to GPIO inputs), or niether.
*               Previously, spi_begin() and spi_end() were automatically invoked
*               every time the command was executed.  spi_end was especially a
*               a problem because it returned the SPI pins to GPIO inputs at the
*               end of every SPI transfer, which is not desired most of the time.
*   2/13    gtm VERSION 1.2.0: The order of the conditionals in the while loop  
*               needed to be changed: 
*                   while (argnum < argc && argv[argnum][0] == '-')
*               Previously, the argv was tested before the argc, which allowed
*               for the posibility of accessing an argv that didn't exist causing
*               a segmentation fault.
*   3/13    gtm VERSION 1.2.1: The usage statement that prints to stdout had a 
*               few typos that needed to be corrected.  The same typos had to be
*               corrected in the README document in addition to other typos.
*   5/13    gtm VERSION 1.3.0: A bug in the way the number of trasmit bytes was 
*               calculated was fixed. Previously, if the total byte length of the 
*               transfer was greater than the number of specified xmit bytes then 
*               an attempt to access command-line arguments beyond the number 
*               available would occur, which causes a segmentation fault.
*   6/13    gtm VERSION 1.3.1: built and tested using 2013-02-09 raspian wheezy
*               (previously 2012-10-28 raspian wheezy) whith the bcm2835-1.25
*               library (previously bcm2835-17). Also, updated this "History"
*               section.  Previously, it had only the first couple of versions
*               listed.
*
*******************************************************************************/


//*******************************************************************************
//	Includes
//*******************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "bcm2835.h"


//*******************************************************************************
//	Prototypes
//*******************************************************************************

static int showusage(int errcode);
static int comparse(int argc, char **argv);


//*******************************************************************************
//	Constants
//*******************************************************************************

#define MAX_LEN 32

typedef enum {
    NO_ACTION,
    SPI_BEGIN,
    SPI_END
} spi_init;

//*******************************************************************************
//	Globals
//*******************************************************************************

char rbuf[MAX_LEN];
char wbuf[MAX_LEN];

uint32_t len = 0;
uint8_t init = NO_ACTION;

// Default parameters (defined in bcm2835)
uint8_t mode = BCM2835_SPI_MODE0; 
uint16_t clk_div = BCM2835_SPI_CLOCK_DIVIDER_65536;
uint8_t cs = BCM2835_SPI_CS0;
uint8_t polarity = LOW;


//*******************************************************************************
//	Main
//*******************************************************************************

int main (int argc, char **argv)
{
    uint32_t i;
	
    // parse the command line
    if (comparse(argc, argv) == EXIT_FAILURE) return showusage (EXIT_FAILURE);

    // Initialize bcm2835 library
    if (!bcm2835_init()) return EXIT_FAILURE;

    // SPI begin if specified    
    if (init == SPI_BEGIN) bcm2835_spi_begin();
    
    // If len is 0, no need to continue, but do SPI end if specified
    if (len == 0) {
        if (init == SPI_END) bcm2835_spi_end();
        return EXIT_SUCCESS;
    }

    // SPI operations
    bcm2835_spi_setDataMode(mode);
    bcm2835_spi_setClockDivider(clk_div);
    bcm2835_spi_chipSelect(cs);
    bcm2835_spi_setChipSelectPolarity(cs, polarity);

    // execute SPI transfer	
    memset(rbuf, 0, sizeof(rbuf));    
    bcm2835_spi_transfernb(wbuf, rbuf, len);

    // This SPI end is done after a transfer if specified
    if (init == SPI_END) bcm2835_spi_end();
    
    // Close the bcm2835 library
    bcm2835_close();

    // print received data	
    for (i = 0; i < len; i++) {
        printf("0x%02x ", rbuf[i]);
    }
    printf("\n");
	
    return EXIT_SUCCESS;	
}    

	
//*******************************************************************************
//  comparse: Parse the command line and return EXIT_SUCCESS or EXIT_FAILURE
//    argc: number of command-line arguments
//    argv: array of command-line argument strings
//*******************************************************************************
static int comparse(int argc, char **argv)
{
    int argnum, i, xmitnum;
	
    if (argc < 2) {  // must have at least program name and len arguments
                     // or -ie (SPI_END) or -ib (SPI_BEGEIN)
        fprintf(stderr, "Insufficient command line arguments\n");
        return EXIT_FAILURE;
    }
    
    argnum = 1;
    while (argnum < argc && argv[argnum][0] == '-') {

        switch (argv[argnum][1]) {

            case 'i':  // SPI init
                switch (argv[argnum][2]) {
                    case 'b': init = SPI_BEGIN; break;
                    case 'e': init = SPI_END; break;
                    default:
                        fprintf(stderr, "%c is not a valid init option\n", argv[argnum][2]);
                        return EXIT_FAILURE;
                }
                break;

            case 'm':  // Mode
                mode = argv[argnum][2] - 0x30;
                if (mode > 3) {
                    fprintf(stderr, "Invalid mode\n");
                    return EXIT_FAILURE;
                }
                break;

            case 'c':  // Clock divider
                if (argv[argnum][2] < '0' || (i = atoi(argv[argnum]+2)) > 15) {
                    fprintf(stderr, "Invalid clock divider exponent\n");
                    return EXIT_FAILURE;
                }
                clk_div = 1 << i;
                break;

            case 's':  // Chip Select
                cs = argv[argnum][2] - 0x30;
                if (cs > 3) {
                    fprintf(stderr, "Invalid chip select\n");
                    return EXIT_FAILURE;
                }
                break;

            case 'p':  // Polarity
                polarity = argv[argnum][2] - 0x30;
                if (polarity > 1) {
                    fprintf(stderr, "Invalid chip select polarity\n");
                    return EXIT_FAILURE;
                }
                break;

            default:
                fprintf(stderr, "%c is not a valid option\n", argv[argnum][1]);
                return EXIT_FAILURE;

        }

        argnum++;   // advance the argument number

    }

    // If command is used for SPI_END or SPI_BEGIN only
    if (argnum == argc && init != NO_ACTION) // no further arguments are needed
        return EXIT_SUCCESS;
	
    // Get len
    if (strspn(argv[argnum], "0123456789") != strlen(argv[argnum])) {
        fprintf(stderr, "Invalid number of bytes specified\n");
        return EXIT_FAILURE;
    }
    
    len = atoi(argv[argnum]);

    if (len > MAX_LEN) {
    	fprintf(stderr, "Invalid number of bytes specified\n");
        return EXIT_FAILURE;
    }
    
    argnum++;   // advance the argument number

    xmitnum = argc - argnum;    // number of xmit bytes

    memset(wbuf, 0, sizeof(wbuf));
    for (i = 0; i < xmitnum; i++) {
        if (strspn(argv[argnum + i], "0123456789abcdefABCDEFxX") != strlen(argv[argnum + i])) {
            fprintf(stderr, "Invalid data\n");
            return EXIT_FAILURE;
        }
        wbuf[i] = (char)strtoul(argv[argnum + i], NULL, 0);
    }

    return EXIT_SUCCESS;
}


//*******************************************************************************
//  showusage: Print the usage statement and return errcode.
//*******************************************************************************
static int showusage(int errcode)
{
    printf("spincl v%s\n",VERSION);
    printf("Usage: \n");
    printf("  spincl [options] len [xmit bytes]\n");
    printf("\n");
    printf("  Invoking spincl results in a full-duplex SPI transfer of a specified\n");
    printf("    number of bytes.  Additionally, it can be used to set the appropriate\n");
    printf("    GPIO pins to their respective SPI configurations or return them\n");
    printf("    to GPIO input configuration.  Options include the SPI clock frequency,\n");
    printf("    SPI Mode, chip select designation, chip select polarity and an\n");
    printf("    initialization option (spi_begin and spi_end).  spincl must be invoked\n");
    printf("    with root privileges.\n");
    printf("\n");
    printf("  The following are the options, which must be a single letter\n");
    printf("    preceded by a '-' and followed by another character.\n");
    printf("    -ix where x is the SPI init option, b[egin] or e[nd]\n");
    printf("      The begin option must be executed before any transfer can happen.\n");
    printf("        It may be included with a transfer.\n");
    printf("      The end option will return the SPI pins to GPIO inputs.\n");
    printf("        It may be included with a transfer.\n");
    printf("    -mx where x is the SPI mode, 0, 1, 2, or 3\n");
    printf("    -cx where x is the exponent of 2 of the clock divider. Allowed values\n");
    printf("      are 0 through 15.  Valid clock divider values are powers of 2.\n");
    printf("      Corresponding frequencies are specified in bcm2835.h.\n");
    printf("    -sx where x is 0 (CS0), 1 (CS1), 2 (CS1&CS2), or 3 (None)\n");
    printf("    -px where x is chip select polarity, 0(LOW) or 1(HIGH)\n");
    printf("\n");
    printf("  len: The number of bytes to be transmitted and received (full duplex).\n");
    printf("    The maximum number of bytes allowed is %d\n", MAX_LEN);
    printf("\n");
    printf("  xmit bytes: The bytes to be transmitted if specified.  If none are\n");
    printf("    specified, 0s will be transmitted, which may be the case when only\n");
    printf("    the received data is relevant.\n");
    printf("\n");
    printf("\n");
    return errcode;
}


