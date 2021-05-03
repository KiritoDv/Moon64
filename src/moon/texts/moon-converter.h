#ifndef MoonConverter
#define MoonConverter

#include <map>
#include <string>
#include <vector>
extern "C" {
#include "types.h"
}

using namespace std;

extern map<wchar_t, vector<int>> charmap2;
void MoonDrawGenericChar(wchar_t c);
u8* MoonGetTranslatedText(wstring txt);

#endif