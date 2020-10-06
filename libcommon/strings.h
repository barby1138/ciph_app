
#pragma once

#include <string>
#include <set>
#include <stdio.h>

void getDirFileList(const std::string& directory, const std::string& cfilter, std::set<std::string>& files);
