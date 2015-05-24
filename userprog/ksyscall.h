/*************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__ 
#define __USERPROG_KSYSCALL_H__ 

#include "kernel.h"
#include "synchconsole.h"
#include "syscall.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


void SysHalt()
{
  kernel->interrupt->Halt();
}


int SysAdd(int op1, int op2)
{
  return op1 + op2;
}

int SysRead(int buffer, int size, OpenFileId id)
{
	int value;
	if(id == ConsoleIn){
		for (int i = 0; i < size; i ++) {
			value = kernel->synchConsoleIn->GetChar();
			kernel->machine->WriteMem(buffer++, 1, value);
		}
	}
	return size;
}

int SysWrite(int buffer, int size, OpenFileId id)
{
	int value;
	if(id == ConsoleOut){
		for (int i = 0; i < size; i ++) {
                        kernel->machine->ReadMem(buffer++, 1, &value);
			kernel->synchConsoleOut->PutChar((char)value);
                }
	}
	return size;
}

int SysJoin(SpaceId id)
{
	return waitpid(id, NULL, 0);
}

SpaceId SysExec(char *fileName)
{
	pid_t pid = vfork();
	if(pid == 0)
	{
		execl("/bin/sh", "sh", "-c", fileName, NULL);
		_exit(1);
	}
	else if (pid < 0)
	{
		return -1;
	}
	return (SpaceId)pid;
}

#endif /* ! __USERPROG_KSYSCALL_H__ */
