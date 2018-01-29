// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "synch.h"
#include "memorymanager.h"


#define min(x, y) (((x) < (y)) ? (x) : (y))
extern MemoryManager *memoryManager;
extern Lock *memoryLock;

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    //NoffHeader noffH;
    unsigned int i, j, size;
		exeFile = executable;
    //executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
		exeFile->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    //ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
					numPages, size);

    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
    	pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
    	pageTable[i].physicalPage = -1;
    	pageTable[i].valid = false;
    	pageTable[i].use = false;
    	pageTable[i].dirty = false;
    	pageTable[i].readOnly = false;  // if the code segment was entirely on
                    					// a separate page, we could set its
                    					// pages to be read-only
			// pageTable[i].timestamp = 0;
    }
		swapPage = new SwapPage(500);
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   // if(pageTable != NULL){
		//  for(int i = 0; i < numPages; i++){
		// 	 if(pageTable[i].valid){
		// 		 memoryManager->FreePage(pageTable[i].physicalPage);
		// 	 }
		//  }
		//  delete[] pageTable;
	 // }
	 delete pageTable;
	 delete exeFile;
	 if(swapPage)delete swapPage;
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

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
				machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState()
{}

// TranslationEntry* AddrSpace::getPageTable(){
// 	return pageTable;
// }


//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState()
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

int AddrSpace::WhichSegment(int virtAddr, Segment* segPtr){
	if(noffH.code.size > 0){
		printf("In which segment code\n");
		if((virtAddr >= noffH.code.virtualAddr)
		 && (virtAddr < noffH.code.virtualAddr + noffH.code.size)){
			*segPtr = noffH.code;
			return 0;
		}
	}
	if(noffH.initData.size > 0){
		printf("In which segment initData\n");
		if((virtAddr >= noffH.initData.virtualAddr)
		 && (virtAddr < noffH.initData.virtualAddr + noffH.initData.size)){
			*segPtr = noffH.initData;
			return 1;
		}
	}

	if(noffH.uninitData.size > 0){
		printf("In which segment uninitData\n");
		if((virtAddr >= noffH.uninitData.virtualAddr)
		 && (virtAddr < noffH.uninitData.virtualAddr + noffH.uninitData.size)){
			*segPtr = noffH.uninitData;
			return 2;
		}
	}
	return 3;
}

// int AddrSpace::LoadIntoFreePage(int virtualPageNo, int physicalPageNo){
// 	int readAddr, physAddr, size, segOffs;
// 	int virtualAddress = virtualPageNo * PageSize;
// 	int offset = 0;
// 	printf("In LoadIntoFreePage\n");
// 	Segment seg;
// 	bool readFromFile = false;
// 	pageTable[virtualPageNo].readOnly = false;
// 	do{
// 		printf("LoadIntoFreePage 1\n");
// 		physAddr = pageTable[virtualPageNo].physicalPage * PageSize + offset;
// 		// physAddr = physicalPageNo * PageSize + offset;
// 		switch (WhichSegment(virtualAddress, &seg)) {
// 			case 0:
// 				printf("LoadIntoFreePage in code\n");
// 				segOffs = virtualAddress - seg.virtualAddr;
//   			readAddr = segOffs + seg.inFileAddr;
// 				printf("PAGESIZE - OFFSET: %d\n",PageSize - offset);
// 				printf("SEG.SIZE - SEGOFFS: %d\n",seg.size - segOffs);
// 				size = min(PageSize -  offset, seg.size - segOffs);
// 				exeFile->ReadAt(&(machine->mainMemory[physAddr]), size, readAddr);
// 				readFromFile = true;
// 				if(size == PageSize){
// 					pageTable[virtualPageNo].readOnly = true;
// 				}
//
// 				// if(virtualPageNo == 1){
// 				// 	printf("Virtual page no 1\n");
// 				// 	ASSERT(machine->mainMemory[physAddr] == 7);
// 				// }
// 				break;
// 			case 1:
// 				printf("LoadIntoFreePage in initData\n");
// 				segOffs = virtualAddress -  seg.virtualAddr;
// 				readAddr = segOffs + seg.inFileAddr;
// 				size = min(PageSize - offset, seg.size - segOffs);
// 				exeFile->ReadAt(&(machine->mainMemory[physAddr]), size, readAddr);
// 				readFromFile = true;
// 				break;
// 			case 2:
// 				printf("LoadIntoFreePage in uninitData\n");
// 				size = min(PageSize - offset, seg.size + seg.virtualAddr - virtualAddress);
// 				bzero(&(machine->mainMemory[physAddr]), size);
// 				break;
// 			case 3:
// 				printf("LoadIntoFreePage in others\n");
// 				bzero(&(machine->mainMemory[physAddr]), PageSize - offset);
// 				return 0;
// 		}
// 		printf("Offset before: %d\n", offset);
// 		offset += size;
// 		printf("Offset after: %d\n", offset);
// 		virtualAddress += size;
// 	}while (offset < PageSize);
// 	if(readFromFile){
// 		stats->numSwapPagesSavedIn++;
// 	}
// 	return 0;
// }
int AddrSpace::LoadIntoFreePage(int virtualPageNo, int physicalPageNo){
	pageTable[virtualPageNo].valid = true;
	pageTable[virtualPageNo].physicalPage = physicalPageNo;
	if(swapPage->LoadFromSwapSpace(&pageTable[virtualPageNo]) == -1){
		printf("did not found in swap space\n");
		printf("in AddrSpace: LoadIntoFreePage\n");
		// pageTable[virtualPageNo].valid = true;
		// pageTable[virtualPageNo].physicalPage = physicalPageNo;


		// memoryLock->Acquire();
		// int physAddr = pageTable[virtualPageNo].physicalPage * PageSize;
		// bzero(&(machine->mainMemory[physicalPageNo * PageSize]), PageSize);
		// exeFile->ReadAt(&(machine->mainMemory[physAddr]), PageSize,  noffH.code.inFileAddr + (virtualPageNo * PageSize));
		bzero(&(machine->mainMemory[physicalPageNo * PageSize]), PageSize);
		exeFile->ReadAt(&(machine->mainMemory[physicalPageNo * PageSize]), PageSize, noffH.code.inFileAddr + (virtualPageNo * PageSize));
		// pageTable[virtualPageNo].valid = true;
		// pageTable[virtualPageNo].physicalPage = physicalPageNo;
		// memoryLock->Release();
		// return 0;
	}
	// pageTable[virtualPageNo].valid = true;
	// pageTable[virtualPageNo].physicalPage = physicalPageNo;
	return 0;
}
// int AddrSpace::PageFault(int virtualPageNo){
//
// 	printf("addrspace (outside) pageFault\n");
// 	// memoryLock->Acquire();
// 	int physPageNo;
//
// 	if(memoryManager->IsAnyPageFree()){
// 			printf("In AddrSpace: pageFault --> got free page\n");
// 			pageTable[virtualPageNo].physicalPage =
// 					memoryManager->AllocPage(currentThread->id,&(pageTable[virtualPageNo]));
// 			physPageNo = pageTable[virtualPageNo].physicalPage;
// 	}
// 	else{
// 		printf("In AddrSpace: pageFault-->Allocating by force\n");
// 		pageTable[virtualPageNo].physicalPage = memoryManager->AllocByForce();
// 		physPageNo = pageTable[virtualPageNo].physicalPage;
// 		//swapFile case
// 	}
// 	// machine->space->LoadIntoFreePage(virtualPageNo);
// 	// if(swapPage->LoadFromSwapSpace(&pageTable[virtualPageNo]) == -1){
// 	// 	printf("in AddrSpace: did not load from SwapSpace\n");
// 	// 	LoadIntoFreePage(virtualPageNo, physPageNo);
// 	// }
// 	// else{
// 	// 		printf("in AddrSpace: loaded from SwapSpace\n");
// 	// }
// 	pageTable[virtualPageNo].valid = true;
// 	pageTable[virtualPageNo].use = false;
// 	pageTable[virtualPageNo].dirty = false;
// 	LoadIntoFreePage(virtualPageNo, physPageNo);
// 	// pageTable[virtualPageNo].valid = true;
// 	// pageTable[virtualPageNo].use = false;
// 	// pageTable[virtualPageNo].dirty = false;
// 	printf("In AddrSpace:: returning from pageFault()\n");
// 	// memoryLock->Release();
// 	return 0;
// }
 // || !swapPage->valid[pageTable[virtualPageNo].virtualPage]
int AddrSpace::evictPage(int virtualPageNo){
	printf("in AddrSpace: evictPage()\n");
	if(pageTable[virtualPageNo].dirty){
		//load into swap memory
		printf("Entry at physNum %d found dirty\n",pageTable[virtualPageNo].physicalPage);
		printf("Saving at swap space\n");
		swapPage->SaveIntoSwapSpace(&pageTable[virtualPageNo]);
	}
	// else if(!swapPage->valid[pageTable[virtualPageNo].virtualPage]){
	// 	printf("Entry at physNum %d not in swapSpace\n",pageTable[virtualPageNo].physicalPage);
	// 	printf("Saving at swap space\n");
	// 	swapPage->SaveIntoSwapSpace(&pageTable[virtualPageNo]);
	// }
	// pageTable[virtualPageNo].physicalPage = -1;
	pageTable[virtualPageNo].valid = false;
	// pageTable[virtualPageNo].use = false;
	pageTable[virtualPageNo].dirty = false;
}
