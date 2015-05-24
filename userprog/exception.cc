// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	is in machine.h.
//----------------------------------------------------------------------
// SC_Halt			0
// SC_Exit			1
// SC_Exec			2
// SC_Join			3
// SC_Create		4
// SC_Remove    	5
// SC_Open			6
// SC_Read			7
// SC_Write			8
// SC_Seek     	 	9
// SC_Close			10
// SC_Delete       	11
// SC_ThreadFork	12
// SC_ThreadYield	13
// SC_ExecV			14
// SC_ThreadExit   	15
// SC_ThreadJoin   	16
// SC_getSpaceID   	17
// SC_getThreadID  	18
// SC_Ipc          	19
// SC_Clock        	20
// SC_Add			42
void ExceptionHandler(ExceptionType which) {
	int type = kernel->machine->ReadRegister(2);
	int vaddr = kernel->machine->ReadRegister(BadVAddrReg);

	DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

	switch (which) {
	case PageFaultException:
			printf("PageFaultException happend with vaddr %d.\n",vaddr);
			++kernel->stats->numPageFaults;
			kernel->currentThread->space->SwapIn(vaddr/PageSize);
			return;
			ASSERTNOTREACHED();
			break;
	case SyscallException:
		switch (type) {
		case SC_Halt:
			DEBUG(dbgSys, "Shutdown, initiated by user program.\n")	;

			SysHalt();

			ASSERTNOTREACHED();
			break;
		case SC_Exit:
			do {
				int result = (int) kernel->machine->ReadRegister(4);
				printf("\nThe current process exit with code %d!!!\n", result);
			} while (0);

			kernel->machine->WriteRegister(PrevPCReg,
					kernel->machine->ReadRegister(PCReg));
			kernel->machine->WriteRegister(PCReg,
					kernel->machine->ReadRegister(PCReg) + 4);
			kernel->machine->WriteRegister(NextPCReg,
					kernel->machine->ReadRegister(PCReg) + 4);

			return;
			ASSERTNOTREACHED();
			break;
		case SC_Exec: {
			// Read the virtual address of the name of the process from the register R4
			int virtualAddress = kernel->machine->ReadRegister(4);
			DEBUG(dbgSys,
					"Executive Process Virtual Address " << virtualAddress << "\n");

			// Read the memory and convert it to process name
			const int SIZE = 80;
			int temp;
			char processName[SIZE];
			for (int i = 0; i < SIZE; i++) {
				kernel->machine->ReadMem(virtualAddress++, 1, &temp);
				processName[i] = (char) temp;
			}
			processName[SIZE - 1] = '\0';
			DEBUG(dbgSys, "Process Name " << (*processName) << "\n");

			/*// Create new address space for this executable file.
			 AddrSpace* addrSpace = new AddrSpace();
			 addrSpace->Load((char*)processName);
			 addrSpace->Execute();*/

			// Execute the process
			SpaceId pid = SysExec((char*) processName);
			DEBUG(dbgSys, "PID " << pid << "\n");

			// Write the process ID to the register R2
			kernel->machine->WriteRegister(2, (int) pid);

			// Modify return point
			{
				/* set previous program counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg,
						kernel->machine->ReadRegister(PCReg));

				/* set program counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg,
						kernel->machine->ReadRegister(PCReg) + 4);

				/* set next program counter for branch execution */
				kernel->machine->WriteRegister(NextPCReg,
						kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;
			ASSERTNOTREACHED();
			break;
		}
		case SC_Join: {
			// Read the ID of the process from the register R4
			int pid = kernel->machine->ReadRegister(4);
			DEBUG(dbgSys, "Pid " << pid << "\n");

			// SysJoin Systemcall
			int joinResult = SysJoin((SpaceId) pid);
			DEBUG(dbgSys, "Child PID " << joinResult << "\n");

			// Write the child PID to the register R2
			kernel->machine->WriteRegister(2, joinResult);

			// Modify return point
			{
				/* set previous program counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg,
						kernel->machine->ReadRegister(PCReg));

				/* set program counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg,
						kernel->machine->ReadRegister(PCReg) + 4);

				/* set next program counter for branch execution */
				kernel->machine->WriteRegister(NextPCReg,
						kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;
			ASSERTNOTREACHED();
			break;
		}
		case SC_Read: {
			// Read the fileAddress, fileSize and fileID from Registers R4, R5, R6
			int fileAddress = kernel->machine->ReadRegister(4);
			int fileSize = kernel->machine->ReadRegister(5);
			int fileID = kernel->machine->ReadRegister(6);
			DEBUG(dbgSys,
					"Read " << fileAddress << "+" << fileSize << "+" << fileID << "\n");

			// SysRead Systemcall
			int readResult = SysRead(fileAddress, fileSize, fileID);
			DEBUG(dbgSys, "Read Result " << readResult << "\n");

			// Write the the result to Register R2
			kernel->machine->WriteRegister(2, readResult);

			// Modify return point
			{
				/* set previous program counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg,
						kernel->machine->ReadRegister(PCReg));

				/* set program counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg,
						kernel->machine->ReadRegister(PCReg) + 4);

				/* set next program counter for branch execution */
				kernel->machine->WriteRegister(NextPCReg,
						kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;
			ASSERTNOTREACHED();
			break;
		}
		case SC_Write: {
			// Read the fileAddress, fileSize and fileID from Registers R4, R5, R6
			int fileAddress = kernel->machine->ReadRegister(4);
			int fileSize = kernel->machine->ReadRegister(5);
			int fileID = kernel->machine->ReadRegister(6);
			DEBUG(dbgSys,
					"Write " << fileAddress << "+" << fileSize << "+" << fileID << "\n");

			// SysWrite Systemcall
			int writeResult = SysWrite(fileAddress, fileSize, fileID);
			DEBUG(dbgSys, "Write Result " << writeResult << "\n");

			// Write the the result to Register R2
			kernel->machine->WriteRegister(2, writeResult);

			// Modify return point
			{
				/* set previous program counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg,
						kernel->machine->ReadRegister(PCReg));

				/* set program counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg,
						kernel->machine->ReadRegister(PCReg) + 4);

				/* set next program counter for branch execution */
				kernel->machine->WriteRegister(NextPCReg,
						kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;
			ASSERTNOTREACHED();
			break;
		}
		case SC_Add: {
			DEBUG(dbgSys,
					"Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysAdd Systemcall*/
			int result;
			result = SysAdd(/* int op1 */(int) kernel->machine->ReadRegister(4),
			/* int op2 */(int) kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Add returning with " << result << "\n");
			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int) result);

			/* Modify return point */
			{
				/* set previous program counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg,
						kernel->machine->ReadRegister(PCReg));

				/* set program counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg,
						kernel->machine->ReadRegister(PCReg) + 4);

				/* set next program counter for branch execution */
				kernel->machine->WriteRegister(NextPCReg,
						kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;
			ASSERTNOTREACHED();
			break;
		}
		default:
			cerr << "Unexpected system call " << type << "\n";
			break;
		}
		break;
	case AddressErrorException:
		cerr << "WTF!" << "\n";
		break;
	default:
		cerr << "Unexpected user mode exception " << (int) which << "\n";
		break;
	}
	ASSERTNOTREACHED()
	;
}

