#pragma once

//
// Generic commands
//
#define CMD_IDN  "IDN?"

// Test commands
#define CMD_TEST  "TEST"
#define CMD_ECHO  "ECHO"

// Programmable Logic access commands
#define CMD_PL    "PL"

// PL state control
#define CMD_ACTIVE "ACTIVE"

// PL data exchange transaction
#define CMD_TX    "TX"

// PL flash access commands
#define CMD_FLASH "FLASH"
#define CMD_RD    "RD"  // Read transaction
#define CMD_WR    "WR"  // Write transaction
#define CMD_WAIT  "WA"  // Wait BUSY status clear, returns status byte
#define CMD_PROG  "PR"  // Program is the combination of wait, write enable and write, returns status byte

