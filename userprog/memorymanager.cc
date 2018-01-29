#include "memorymanager.h"
#include "system.h"
#include "sysdep.h"
#include <stdlib.h>
#include "addrspace.h"
#include "processtable.h"

extern ProcessTable* processTable;
// extern SwapPage* swapPage;
MemoryManager::MemoryManager(int numPages)
{

	timestamp = 0;
	numPhysPages = numPages;
	freeNumPhysPages = numPages;
	bitMap = new BitMap(numPages);
	lock = new Lock("lock of memory manager");
	processMap = new int[numPages];
	entries = new TranslationEntry*[numPages];
	accessTimes = new int[numPages];
	// spaceTable = new AddrSpace*[numPages];
	// vpnTable = new int[numPages];
	// frames = new Frame[numPages];
	for(int i = 0; i < numPages; i++){
		entries[i] = NULL;
		processMap[i] = -1;
		accessTimes[i] = 0;
		// spaceTable[i] = NULL;
		// vpnTable[i] = -1;
		// frames[i].isOccupied = false;
		// frames[i].processID = -1;
		// frames[i].pageTableEntry = NULL;
		// frames[i].lastAccessTime = 0;
	}
	// fifoList = new List<int>;
}

MemoryManager::~MemoryManager()
{
	delete bitMap;
	delete lock;
	delete[] processMap;
	delete[] entries;
	// delete fifoList;
	// delete[] spaceTable;
	// delete[] vpnTable;
	// delete[] frames;
}
int
MemoryManager::AllocPage()
{
	lock->Acquire();
	int ret = bitMap->Find();
	lock->Release();

	return ret;
	// lock->Acquire();
	// printf("in MemoryManager-->AllocPage() memory found\n");
  // int physNum = bitMap->Find();
  // if(physNum == -1){
	// 	printf("in MemoryManager-->AllocPage() no memory found\n");
  //   lock->Release();
  //   return -1;
  // }
	// printf("in MemoryManager-->AllocPage() memory found\n");
  // lock->Release();
  // return physNum;
}

int MemoryManager::AllocPage(int processNo, TranslationEntry *entry){
	int physNum = AllocPage();

	lock->Acquire();
	// int physNum = bitMap->Find();
  if(physNum == -1){
    lock->Release();
    return -1;
  }
	processMap[physNum] = processNo;
	entries[physNum] = entry;
	// frames[physNum].processID = processNo;
	// frames[physNum].pageTableEntry = entry;
	// spaceTable[physNum] = space;
	// vpnTable[physNum] = vpn;
	// fifoList->Append(physNum);
	// printf("FIFO_LIST SIZE(APPENDING IN NORMAL ALLOC): %d\n",fifoList->size);
  lock->Release();
  return physNum;
}

int MemoryManager::AllocByForce(){
	printf("in memorymanager:in AllocByForce()\n");
	int physNum;
	lock->Acquire();
	//----->replacement using FIFO algorithm
	physNum = VictimPage();
	//----->replacement using FIFO algorithm
	// spaceTable[physNum]->evictPage(vpnTable[physNum]);
	// if(entries[physNum]->dirty){
	// 	printf("Entry at physNum %d found dirty\n",physNum);
	// 	printf("Saving at swap space\n");
	// 	currentThread->space->swapPage->SaveIntoSwapSpace(entries[physNum]);
	// }
	// else if(!currentThread->space->swapPage->valid[vpnTable[physNum]]){
  // 	printf("in MemoryManager:: not in swapPage\n");
	// 	currentThread->space->swapPage->SaveIntoSwapSpace(entries[physNum]);
	// }

	//----->random replacement
	// physNum = rand() % numPhysPages;
	// entries[physNum]->valid = false;
	// entries[physNum]->physicalPage = -1;
	// entries[physNum]->use = false;
	// entries[physNum]->dirty = false;
	//----->random replacement

	// entries[physNum]->physicalPage = -1;
	// entries[physNum]->use = false;
	// entries[physNum]->dirty = false;
	// fifoList->Append(physNum);
	// printf("FIFO_LIST SIZE(APPENDING IN FORCED ALLOC): %d\n",fifoList->size);
	// int victimID = frames[physNum].processID;
	int victimID = processMap[physNum];
	Thread* thread = (Thread*)processTable->Get(victimID);
	AddrSpace* space = thread->space;
	// TranslationEntry* pageTableEntry = frames[physNum].pageTableEntry;
	TranslationEntry* pageTableEntry = entries[physNum];
	int victimPageNumber = pageTableEntry->virtualPage;
	space->evictPage(victimPageNumber);
	lock->Release();
	printf("in memorymanager: returning from AllocByForce() vpn: %d  physNum: %d\n", entries[physNum]->virtualPage, physNum);
	return physNum;
}

int MemoryManager::UpdateLastAccesTime(int physNum){
	// fifoList->Append(physNum);
	timestamp++;
	// frames[physNum].lastAccessTime = timestamp;
	accessTimes[physNum] = timestamp;
}

void
MemoryManager::FreePage(int physPageNum)
{
	lock->Acquire();
	bitMap->Clear(physPageNum);
	// entries[physPageNum] = NULL;
	lock->Release();
}

bool
MemoryManager::PageIsAllocated(int physPageNum)
{
	lock->Acquire();
	bool ret = bitMap->Test(physPageNum);
	lock->Release();
	return ret;
}

bool
MemoryManager::IsAnyPageFree()
{
	lock->Acquire();
	bool ret;
	if(bitMap->NumClear() == 0)
		ret = false;
	else
		ret = true;
	lock->Release();
	return ret;
}

int
MemoryManager::NumFreePages()
{
	lock->Acquire();
	int ret = bitMap->NumClear();
	return ret;
	lock->Release();
}

//LRU algorithm to select a page as victim page
int MemoryManager::VictimPage(){
	int physicalPageNo = 0;
	// ASSERT(!fifoList->IsEmpty());
	// physicalPageNo = fifoList->Remove();
	// printf("FIFO_LIST SIZE(REMOVING) %d\n",fifoList->size);
	// int i;
	// 	for (i = 1; i<numPhysPages; i++) {
	// 		if (machine->LRUcounter[i] < machine->LRUcounter[physicalPageNo])
	// 			physicalPageNo = i;
	// 	}
	int minTime;
	// LRUFframNumber = 0;
	// minTime = frames[0].lastAccessTime;
	minTime = accessTimes[0];
	for(int i = 1; i < numPhysPages; i++){
		if(accessTimes[i] < minTime){
			physicalPageNo = i;
			// minTime = frames[i].lastAccessTime;
			minTime = accessTimes[i];
		}
	}
	return physicalPageNo;
}
