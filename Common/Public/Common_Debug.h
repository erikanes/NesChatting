#pragma once

// -------------------------------------------
#ifdef _DEBUG
#include <crtdbg.h>
#define _CRTDBG_MAP_ALLOC

#ifndef DBG_NEW 
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ ) 
#define new DBG_NEW 
#endif

#define _CRTSETDBGFLAG_ _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

// -------------------------------------------
#else

#define CRTSETDBGFLAG

#endif