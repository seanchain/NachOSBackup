// stats.h 
//	Routines for managing statistics about Nachos performance.
//
// DO NOT CHANGE -- these stats are maintained by the machine emulation.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "stats.h"

//----------------------------------------------------------------------
// Statistics::Statistics
// 	Initialize performance metrics to zero, at system startup.
//----------------------------------------------------------------------

Statistics::Statistics()
{
    totalTicks = idleTicks = systemTicks = userTicks = 0;
    numDiskReads = numDiskWrites = 0;
    numConsoleCharsRead = numConsoleCharsWritten = 0;
    numPageFaults = numPacketsSent = numPacketsRecvd = 0;
}

//----------------------------------------------------------------------
// Statistics::Print
// 	Print performance metrics, when we've finished everything
//	at system shutdown.
//----------------------------------------------------------------------

void
Statistics::Print()
{
    std::cout << "Ticks: total " << totalTicks << ", idle " << idleTicks;
		std::cout << ", system " << systemTicks << ", user " << userTicks <<"\n";
    std::cout << "Disk I/O: reads " << numDiskReads;
		std::cout << ", writes " << numDiskWrites << "\n";
		std::cout << "Console I/O: reads " << numConsoleCharsRead;
    std::cout << ", writes " << numConsoleCharsWritten << "\n";
    std::cout << "Paging: faults " << numPageFaults << "\n";
    std::cout << "Network I/O: packets received " << numPacketsRecvd;
		std::cout << ", sent " << numPacketsSent << "\n";
}
