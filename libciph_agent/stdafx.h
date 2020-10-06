
#pragma once

#include <assert.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <math.h>
#include <fstream>
#include <stdexcept>

#include <cstdint> // uint64_t
#include <memory>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// libcommon
#include "exceptions.h"

#ifdef _LINUX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include <quark/config.h>
#include <quark/singleton.h>

using namespace quark;
