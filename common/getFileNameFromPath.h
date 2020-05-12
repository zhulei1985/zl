#pragma once

#include <vector>
#include <string>

//不能在中文目录下
std::vector<std::string> GetFileNameFromPath(const char* pPathName);