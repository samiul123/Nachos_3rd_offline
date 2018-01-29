#ifndef SWAPPAGE_H
#define SWAPPAGE_H

#include "filesys.h"
#include "translate.h"
#include "openfile.h"
// class AddrSpace;
class SwapPage{
private:
  OpenFile *swapFile;
  int numPages;
  char *swapFileName;

  int pId;
public:
  bool *valid;
  SwapPage(int numPages);
  SwapPage();
  ~SwapPage();
  void Initialize();
  // void SaveIntoSwapSpace(int vpn, int ppn);
  // int LoadFromSwapSpace(int vpn, int ppn);
  void SaveIntoSwapSpace(TranslationEntry *entry);
  int LoadFromSwapSpace(TranslationEntry *entry);
};
#endif
