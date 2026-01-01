#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>

#include <unordered_map>
#include <algorithm>

#include "ImporterMathHelper.h"

#ifdef _DEBUG
#pragma comment(lib, "assimp-vc143-mtd")
#else
#pragma comment(lib, "assimp-vc143-mt")
#endif