#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"
#include "noff.h"
#include "synch.h"

static void SwapHeader(NoffHeader *noffH) {
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
#ifdef RDATA
	noffH->readonlyData.size = WordToHost(noffH->readonlyData.size);
	noffH->readonlyData.virtualAddr =
	WordToHost(noffH->readonlyData.virtualAddr);
	noffH->readonlyData.inFileAddr =
	WordToHost(noffH->readonlyData.inFileAddr);
#endif 
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);

#ifdef RDATA
	DEBUG(dbgAddr, "code = " << noffH->code.size <<
			" readonly = " << noffH->readonlyData.size <<
			" init = " << noffH->initData.size <<
			" uninit = " << noffH->uninitData.size << "\n");
#endif
}
////////////////////////////////////////////////////////////////////////////////////

/* NumVirtPages is power(2,24),
 * so that AddrSpace is 32 bit,as well as 4 G.
 * it's big enough to cause PageFaultException.
 *13.4.28
 */
#define NumVirtPages 1<<24

/* This function has been modified,
 * and you can use it without any change.
 */
AddrSpace::AddrSpace() {
	pageTable = new TranslationEntry[NumVirtPages];
	numPages = NumVirtPages;
	/* why to make numPages be equal to NumVirtPages?
	 * Because we can use AddrSpace::Translate so that!
	 * 13.4.30
	 */
	for (int i = 0; i < numPages; ++i) {
		pageTable[i].virtualPage = i;
		pageTable[i].physicalPage = -1;
		pageTable[i].valid = FALSE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;
	}
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace() {
	delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Assumes that the page table has been initialized, and that
//	the object code file is in NOFF format.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

bool AddrSpace::Load(char *fileName) {

	OpenFile *executable = kernel->fileSystem->Open(fileName);
	NoffHeader noffH;

	if (executable == NULL) {
		cerr << "Unable to open file " << fileName << "\n";
		return FALSE;
	}

	executable->ReadAt((char *) &noffH, sizeof(noffH), 0);
	if ((noffH.noffMagic != NOFFMAGIC)
			&& (WordToHost(noffH.noffMagic) == NOFFMAGIC))
		SwapHeader(&noffH);
	ASSERT(noffH.noffMagic == NOFFMAGIC);

	/* 13.4.30
	 * for every segment, pagePerSeg is the
	 * number of pages that it covering,
	 * and vpn is the first index of those pages.
	 */
	unsigned int pagePerSeg, vpn;

	/* code segment. */
	pagePerSeg = divRoundUp(noffH.code.size, PageSize);
	vpn = noffH.code.virtualAddr / PageSize;
	for (int i = 0; i < pagePerSeg; ++i) {
		pageTable[vpn + i].physicalPage = this->UseFreeFrame();
		// set attributes.
		pageTable[vpn + i].valid = TRUE;
		pageTable[vpn + i].readOnly = TRUE; // code segment.
	}

	/* initData segment. */
	pagePerSeg = divRoundUp(noffH.initData.size, PageSize);
	vpn = noffH.initData.virtualAddr / PageSize;
	for (int i = 0; i < pagePerSeg; ++i) {
		pageTable[vpn + i].physicalPage = this->UseFreeFrame();
		pageTable[vpn + i].valid = TRUE;
		pageTable[vpn + i].readOnly = FALSE;
	}

	/* uninitData segment. */
	pagePerSeg = divRoundUp(noffH.uninitData.size, PageSize);
	vpn = noffH.uninitData.virtualAddr / PageSize;
	for (int i = 0; i < pagePerSeg; ++i) {
		pageTable[vpn + i].physicalPage = this->UseFreeFrame();
		pageTable[vpn + i].valid = TRUE;
		pageTable[vpn + i].readOnly = FALSE;
	}

#ifdef RDATA
	pagePerSeg = divRoundUp(noffH.readonlyData.size, PageSize);
	vpn = noffH.readonlyData.virtualAddr/PageSize;
	for (int i = 0;i<pagePerSeg;++i)
	{
		pageTable[vpn+i].physicalPage = this->UseFreeFrame();
		pageTable[vpn+i].valid = TRUE;
		pageTable[vpn+i].readOnly = TRUE;
	}
#endif

	for (int i = 0; i < UserStackSize / PageSize; ++i) {
		pageTable[numPages - 1 - i].physicalPage = this->UseFreeFrame();
		pageTable[numPages - 1 - i].valid = TRUE;
		pageTable[numPages - 1 - i].readOnly = FALSE;
	}

	unsigned int paddr;
	if (noffH.code.size > 0) {
		DEBUG(dbgAddr, "Initializing code segment.");
		DEBUG(dbgAddr, noffH.code.virtualAddr << ", " << noffH.code.size);

		Translate(noffH.code.virtualAddr, &paddr, 0);
		executable->ReadAt(&(kernel->machine->mainMemory[paddr]),
				noffH.code.size, noffH.code.inFileAddr);
	}

	if (noffH.initData.size > 0) {
		DEBUG(dbgAddr, "Initializing data segment.");
		DEBUG(dbgAddr,
				noffH.initData.virtualAddr << ", " << noffH.initData.size);

		Translate(noffH.initData.virtualAddr, &paddr, 1);
		executable->ReadAt(&(kernel->machine->mainMemory[paddr]),
				noffH.initData.size, noffH.initData.inFileAddr);
	}

#ifdef RDATA
	if (noffH.readonlyData.size > 0) {
		DEBUG(dbgAddr, "Initializing read only data segment.");
		DEBUG(dbgAddr, noffH.readonlyData.virtualAddr << ", " << noffH.readonlyData.size);

		Translate(noffH.readonlyData.virtualAddr,&paddr,0);
		executable->ReadAt(&(kernel->machine->mainMemory[paddr]),noffH.readonlyData.size, noffH.readonlyData.inFileAddr);
	}
#endif

	delete executable;			// close file
	return TRUE;			// success
}

//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program using the current thread
//
//      The program is assumed to have already been loaded into
//      the address space
//
//----------------------------------------------------------------------

void AddrSpace::Execute() {

	kernel->currentThread->space = this;

	this->InitRegisters();		// set the initial register values
	this->RestoreState();		// load page table register

	kernel->machine->Run();		// jump to the user progam

	ASSERTNOTREACHED()
	;			// machine->Run never returns;
	// the address space exits
	// by doing the syscall "exit"
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters() {
	Machine *machine = kernel->machine;
	int i;

	for (i = 0; i < NumTotalRegs; i++)
		machine->WriteRegister(i, 0);
	machine->WriteRegister(PCReg, 0);
	machine->WriteRegister(NextPCReg, 4);
	machine->WriteRegister(StackReg, numPages * PageSize - 16);
	DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, don't need to save anything!
//----------------------------------------------------------------------

void AddrSpace::SaveState() {
	// do nothing.
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() {
	kernel->machine->pageTable = pageTable;
	kernel->machine->pageTableSize = numPages;
}

//----------------------------------------------------------------------
// AddrSpace::Translate
//  Translate the virtual address in _vaddr_ to a physical address
//  and store the physical address in _paddr_.
//  The flag _isReadWrite_ is false (0) for read-only access; true (1)
//  for read-write access.
//  Return any exceptions caused by the address translation.
//----------------------------------------------------------------------
ExceptionType AddrSpace::Translate(unsigned int vaddr, unsigned int *paddr,
		int isReadWrite) {
	TranslationEntry *pte;
	int pfn;
	unsigned int vpn = vaddr / PageSize;
	unsigned int offset = vaddr % PageSize;

	if (vpn >= numPages) {
		return AddressErrorException;
	}

	pte = &pageTable[vpn];

	if (isReadWrite && pte->readOnly) {
		return ReadOnlyException;
	}

	pfn = pte->physicalPage;

	// if the pageFrame is too big, there is something really wrong!
	// An invalid translation was loaded into the page table or TLB.
	if (pfn >= NumPhysPages) {
		DEBUG(dbgAddr, "Illegal physical page " << pfn);
		return BusErrorException;
	}

	pte->use = TRUE;          // set the use, dirty bits

	if (isReadWrite)
		pte->dirty = TRUE;

	*paddr = pfn * PageSize + offset;

	ASSERT((*paddr < MemorySize));

	return NoException;
}

//----------------------------------------------------------
#include <list>
using std::list;
static int referCount[NumPhysPages];
list<SwapId*> swapArray[NumPhysPages];

int AddrSpace::NextPageToSwapInplace() {
	int pageNum = -1;
	for (int i = 0; i < numPages; i++) {
		if (pageNum == -1) {
			if (pageTable[i].valid && !pageTable[i].readOnly) {
				pageNum = i;
			}
		} else {
			if (pageTable[i].valid && !pageTable[i].readOnly) {
				if ((referCount[pageTable[i].physicalPage])
						< referCount[pageTable[pageNum].physicalPage]) {
					pageNum = i;
				}
			}
		}
	}
	return pageNum;
}

int AddrSpace::UseFreeFrame() {
	int i = 0;
	int frame = -1;
	for (; i < NumPhysPages; ++i) {
		if (referCount[i] == 0)
			break;
	}
	if (i < NumPhysPages) {
		frame = i;
	} else {
		int tmp = NextPageToSwapInplace();
		if (tmp >= 0) {
			SwapOut(tmp);
			frame = this->pageTable[tmp].physicalPage;
		} else {
			// kernel->NextPageToSwapGlobal();
			ASSERT(-1 > 0); // now, it's ok.
		}
	}
	++referCount[frame];
	return frame;
}

int AddrSpace::FreeFrame() {
	return 0;
}

Semaphore *lock = new Semaphore("Mutex", 1);

void AddrSpace::SwapOut(int i) {
	unsigned int frame; // frame
	frame = pageTable[i].physicalPage;
	printf("Page %d in frame %d was swaped out.\n", i, frame);

	char *temp = new char[PageSize];
	memcpy(temp, (kernel->machine->mainMemory + frame * PageSize), PageSize);
	if (temp == NULL) {
		cout << "Null" << endl;
	}

	SwapId *virtualPage = new SwapId(0, i, temp);
	swapArray[frame].push_back(virtualPage);
	pageTable[i].valid = FALSE;
}

void AddrSpace::SwapIn(int i) {
	unsigned int frame; // frame

	lock->P();
	frame = this->UseFreeFrame();
	pageTable[i].physicalPage = frame;
	for (int j = 0; j < NumPhysPages; j++) {
		list<SwapId*>::iterator iter = swapArray[j].begin();
		while (iter != swapArray[j].end()) {
			if ((*iter)->vpn == i) {
				printf("page %d was swaped in frame %d.\n", i, frame);
				memcpy(&(kernel->machine->mainMemory[frame * PageSize]),
						(*iter)->point, PageSize);
				pageTable[i].valid = TRUE;
				iter = swapArray[j].erase(iter);
				lock->V();
				return;
			} else {
				iter++;
			}
		}
	}
}

