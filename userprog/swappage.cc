#include "swappage.h"
#include "system.h"

SwapPage::SwapPage(int numPages){
  swapFileName = (char*)"swapFile";
  // sprintf(swapFileName, "Mem%d", pId);
  swapFile = NULL;
  this->numPages = numPages;
  valid = new bool[numPages];
  for(int i = 0; i < numPages; i++){
    valid[i] = false;
  }
}

SwapPage::~SwapPage(){
  fileSystem->Remove(swapFileName);
  delete swapFile;
  delete[] swapFileName;
  delete[] valid;
}

void SwapPage::Initialize(){
  printf("in SwapPage--->Initializing swapFile\n");
  // swapFile = new OpenFile(2);
  fileSystem->Create(swapFileName, numPages * PageSize);
  swapFile = fileSystem->Open(swapFileName);
}

// void SwapPage::SaveIntoSwapSpace(int vpn, int ppn){
//   if(swapFile == NULL){
//     Initialize();
//   }
//   int offset = vpn * PageSize;
//   int physAddr = ppn * PageSize;
//
//   swapFile->WriteAt(&machine->mainMemory[physAddr], PageSize, offset);
//   stats->numSwapPagesSavedIn++;
//   valid[vpn] = true;
// }

void SwapPage::SaveIntoSwapSpace(TranslationEntry *entry){
  if(swapFile == NULL){
    Initialize();
  }
  int offset = entry->virtualPage * PageSize;
  int physAddr = entry->physicalPage * PageSize;
  printf("In SwapPage---->Writing to file\n");
  swapFile->WriteAt(&machine->mainMemory[physAddr], PageSize, offset);
  // swapFile->Write(&machine->mainMemory[physAddr], PageSize);
  stats->numSwapPagesSavedIn++;
  valid[entry->virtualPage] = true;
  printf("in SwapPAge--->File written in swap file\n");
}

// int SwapPage::LoadFromSwapSpace(int vpn, int ppn){
//   if(valid[vpn]){
//     int offset = vpn * PageSize;
//     int physAddr = ppn * PageSize;
//     swapFile->ReadAt(&machine->mainMemory[physAddr], PageSize, offset);
//     stats->numSwapPagesLoadedFrom++;
//     return 0;
//   }
//   else{
//     return -1;
//   }
// }

int SwapPage::LoadFromSwapSpace(TranslationEntry *entry){
  printf("in SwapPage: LoadFromSwapSpace()\n");
  if(valid[entry->virtualPage]){
    int offset = entry->virtualPage * PageSize;
    int physAddr = entry->physicalPage * PageSize;
    swapFile->ReadAt(&machine->mainMemory[physAddr], PageSize, offset);
    // swapFile->Read(&machine->mainMemory[physAddr], PageSize);
    stats->numSwapPagesLoadedFrom++;
    printf("in SwapPage: returning from LoadFromSwapSpace with 0\n");
    return 0;
  }
  else{
    printf("in SwapPage: returning from LoadFromSwapSpace with -1\n");
    return -1;
  }
}
