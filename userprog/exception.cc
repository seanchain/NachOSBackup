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
#include "ksyscall.h"
#include "syscall.h"
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

void
ExceptionHandler(ExceptionType which)
{
    int type = kernel->machine->ReadRegister(2);

    DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");
    switch (which) {
    case SyscallException:
      switch(type) {
      case SC_Halt:
	DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

	SysHalt();

	ASSERTNOTREACHED();
	break;
      case SC_Exec:
      {
    	  int virtualAddress = kernel->machine->ReadRegister(4);
    	  DEBUG(dbgSys, "Executive Process Virtual Address " << virtualAddress << "\n");

    	  const int SIZE = 80;
    	  int temp;
    	  char processName[SIZE];
    	  for (int i=0; i<SIZE; i++)
    	  {
    		  kernel->machine->ReadMem(virtualAddress++, 1, &temp);
    		  processName[i] = (char)temp;
    	  }
    	  processName[SIZE-1] = '\0';
    	  DEBUG(dbgSys, "Process Name " << (*processName) << "\n");


    	  SpaceId pid = SysExec((char*)processName);
    	  DEBUG(dbgSys, "PID " << pid << "\n");

    	  kernel->machine->WriteRegister(2, (int)pid);

    	  {
    		  kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

    		  kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

    		  kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
    	  }

    	  return;

    	  ASSERTNOTREACHED();

    	  break;
      }
      case SC_Join:
      {
    	  int pid = kernel->machine->ReadRegister(4);
    	  DEBUG(dbgSys, "Pid " << pid << "\n");

    	  int joinResult = SysJoin((SpaceId)pid);
    	  DEBUG(dbgSys, "Child PID " << joinResult << "\n");

    	  kernel->machine->WriteRegister(2, joinResult);

    	  {
    		  kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

    		  kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

    		  kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
    	  }

    	  return;

    	  ASSERTNOTREACHED();

    	  break;
      }
      case SC_Read:
      {
	int buffer = kernel->machine->ReadRegister(4);
	int size = kernel->machine->ReadRegister(5);
	OpenFileId fileID = kernel->machine->ReadRegister(6);
	DEBUG(dbgSys, "Read " << buffer << " + " << size << " + " << fileID << "\n");
	int result = SysRead(buffer, size, fileID);
	kernel->machine->WriteRegister(2, (int)result);
	{
            kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

            kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

            kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
	}
	
	return;
	ASSERTNOTREACHED();
      }

      case SC_Write:
      {
        int buffer = kernel->machine->ReadRegister(4);
        int size = kernel->machine->ReadRegister(5);
        OpenFileId fileID = kernel->machine->ReadRegister(6);
        DEBUG(dbgSys, "Write " << buffer << " + " << size << " + " << fileID << "\n");
        int result = SysWrite(buffer, size, fileID);
        kernel->machine->WriteRegister(2, (int)result);
        {
            kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

            kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

            kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
        }

        return;
        ASSERTNOTREACHED();
      }
      case SC_Add:
	DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");
	
	/* Process SysAdd Systemcall*/
	int result;
	result = SysAdd(/* int op1 */(int)kernel->machine->ReadRegister(4),
			/* int op2 */(int)kernel->machine->ReadRegister(5));

	DEBUG(dbgSys, "Add returning with " << result << "\n");
	/* Prepare Result */
	kernel->machine->WriteRegister(2, (int)result);
	
	/* Modify return point */
	{
	  /* set previous programm counter (debugging only)*/
	  kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

	  /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
	  kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);
	  
	  /* set next programm counter for brach execution */
	  kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
	}

	return;
	
	ASSERTNOTREACHED();

	break;

      default:
	std::cerr << "Unexpected system call " << type << "\n";
	break;
      }
      break;
    default:
      std::cerr << "Unexpected user mode exception" << (int)which << "\n";
      break;
    }
    ASSERTNOTREACHED();
}
