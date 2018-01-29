#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#ifndef BITMAP_H
#include "bitmap.h"
#endif

#ifndef SYNCH_H
#include "synch.h"
#include "list.h"
#endif

// enum EvictMethod{
// 	LRU
// };

class MemoryManager{
public:
	MemoryManager(int numPages);
	~MemoryManager();
	int AllocPage();
	void FreePage(int physPageNum);
	bool PageIsAllocated(int physPageNum);
	bool IsAnyPageFree();
	int NumFreePages();
	int AllocPage(int processNo, TranslationEntry *entry);
	int AllocByForce();
	int VictimPage();
	int UpdateLastAccesTime(int physNum);
	List<int> *fifoList;
private:
	BitMap *bitMap;
	Lock *lock;
	int* processMap;
  TranslationEntry** entries;
	// EvictMethod evictMethod;

	int numPhysPages;
	AddrSpace** spaceTable;
	int* vpnTable;

	int timestamp;
	int freeNumPhysPages;
	int* accessTimes;
};

#endif
