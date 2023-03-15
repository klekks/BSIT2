#pragma once

#include <regex>
#include <string>
#include <vector>

#include "params.h"

#include <zip.h>
#pragma comment(lib, "zip.lib")

void InitArchiveManager();
std::vector<std::string> GetFilesToBackup();
void BackupFiles(std::vector<std::string> files);
