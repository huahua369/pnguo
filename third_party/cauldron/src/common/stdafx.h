﻿// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <SDKDDKVer.h>

#define NOMINMAX    
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <wrl.h>
#include <KnownFolders.h>
#include <shlobj.h>

// C RunTime Header Files
#include <malloc.h>
#include <tchar.h>
#include <cassert>
#include <cstdlib>

// math API
#include <DirectXMath.h>
using namespace DirectX;

#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include <mutex>
#include <limits>
#include <algorithm>
#include <mutex>



// TODO: reference additional headers your program requires here
#include <stb_image.h>
#include <stb_image_write.h>
