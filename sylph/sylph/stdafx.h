/**
 * @file     stdafx.h
 * @brief    sylph precompile header 
 * @author   M.Horigome
 * @version  1.0.0.0000
 * @date     2016-03-10 
 *
 * https://github.com/horigome/sylph
 */
#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <conio.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
#include <atlstr.h>

using namespace ATL;

#include <algorithm>
#include <vector>
#include <functional>

#include "SylphCommonLog.h"
#include "SylphCommon.h"