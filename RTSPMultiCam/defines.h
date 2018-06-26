#include <cstdio>
#include <cstdlib>
#include <conio.h>
#include <ctime>  
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <winsock2.h>
#include <functional>

#ifndef MEM_ALIGN
#define MEM_ALIGN 64
#endif

//----- bit operation -----
#define  MAKE_BIT(i)       (1U << (i))
///set bit i in variable n
#define  SET_BIT(n, i)     ((n) |= MAKE_BIT(i))
///clear bit i in variable n
#define  CLR_BIT(n, i)     ((n) &= ~MAKE_BIT(i))
///test bit i in variable n
#define  TEST_BIT(n, i)    (static_cast<bool>(((n) & MAKE_BIT(i)) != 0))
