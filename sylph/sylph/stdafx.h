/**
 * @file     stdafx.h
 * @brief    sylph precompile header 
 * @author   M.Horigome
 * @version  1.0.0.0000
 * @date     2016-03-07 
 */
#pragma once

#ifndef STRICT
#define STRICT
#endif

#include "targetver.h"
#define _ATL_FREE_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define ATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW

#include "resource.h"
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