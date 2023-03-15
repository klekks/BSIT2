#include <Windows.h>
#include <filesystem>
#include <map>
#include "archive_manager.h"

#include <iostream>
#include <fstream>

std::string backup_path,
			source_path,
			backup_name;

namespace fs = std::filesystem;


template <typename T>
void log_message(T msg)
{
	std::ofstream log_file(LOG_PATH, std::ios_base::app);
	log_file << msg << std::endl;
	log_file.close();
}

void InitArchiveManager()
{
	char* value = new char[256];
	GetPrivateProfileString("main", "source_path", "./", value, 255, CONFIG_FILE_NAME);
	log_message(value);
	source_path = value;
	GetPrivateProfileString("main", "backup_path", "./", value, 255, CONFIG_FILE_NAME);
	log_message(value);
	backup_path = value;
	GetPrivateProfileString("main", "backup_name", "archive.zip", value, 255, CONFIG_FILE_NAME);
	log_message(value);
	backup_name = value;
	delete[] value;
}

std::vector<std::regex>& GetFilterList()
{
	static std::vector<std::regex> filters;

	filters.clear();
	char* value = new char[512];
	
	GetPrivateProfileString("main", "file_masks", "/UNDEFINED/a1efdsf8se4fc/", value, 511, CONFIG_FILE_NAME);
	std::string masks = value;
	
	while (!masks.empty())
	{
		size_t coma_index = masks.find(',');
		coma_index = coma_index == std::string::npos ? masks.size() : coma_index;

		std::string regexp_string = masks.substr(0, coma_index);
		regexp_string = std::regex_replace(regexp_string, std::regex("\\*"), ".*");
		regexp_string = std::regex_replace(regexp_string, std::regex("\\?"), ".?");

		filters.push_back(std::regex(regexp_string));

		if (coma_index == masks.size())
			masks.clear();
		else
			masks.erase(0, coma_index + 1);
	}

	delete[] value;
	return filters;
}

bool IsSuitableFile(const std::vector<std::regex>& filters, const std::string& file)
{
	std::smatch match;
	for (const auto& filter : filters)
	{
		std::regex_search(file, match, filter);
		if (!match.empty()) return true;
	}
	return false;
}


std::vector<std::string>& GetSuitableFiles(const std::string& directory)
{
	static std::vector<std::string> suitable_files;
	std::vector<std::regex>& filters = GetFilterList();

	suitable_files.clear();
	for (const auto& dirEntry : fs::recursive_directory_iterator(directory))
	{
		std::string file_name = dirEntry.path().string();
		file_name = std::regex_replace(file_name, std::regex("\\\\"), "/"); // converts "C:\\dir\\dir" to "C:/dir/dir"

		if (!dirEntry.is_directory() && IsSuitableFile(filters, file_name))
		{
			suitable_files.push_back(file_name);
		}
	}
	return suitable_files;
}

void RemoveUnmodifiedFiles(std::vector<std::string>& files)
{
	static std::map<std::string, int64_t> modification_history;

	for (auto file = files.begin(); file != files.end(); )
	{
		fs::file_time_type modification_file_time = fs::last_write_time(*file);
		int64_t modification_time = modification_file_time.time_since_epoch().count();

		if (modification_history[*file] < modification_time)
		{
			modification_history[*file++] = modification_time;
		}
		else
		{
			file = files.erase(file);
		}
	}
}

std::string GetRelativeFileName(const std::string filename)
{
	return filename.substr(source_path.size() + 1, filename.size());
}

std::vector<std::string> GetFilesToBackup()
{
	std::vector<std::string>& files = GetSuitableFiles(source_path);
	RemoveUnmodifiedFiles(files);
	return files;
}

void ZipRewriteFile(zip_t *archive, const std::string& file)
{
	zip_source_t* src = zip_source_file(archive, file.c_str(), 0, 0);
	zip_int64_t file_id = zip_name_locate(archive, GetRelativeFileName(file).c_str(), 0);
	int64_t res = zip_file_replace(archive, file_id, src, 0);
}

void ZipCreateFile(zip_t* archive, const std::string& file)
{
	zip_source_t* src = zip_source_file(archive, file.c_str(), 0, 0);
	int64_t res = zip_file_add(archive, GetRelativeFileName(file).c_str(), src, 0);
}

bool CheckIfFileExist(zip_t *archive, const std::string& filename)
{
	zip_file_t* file = zip_fopen(archive, GetRelativeFileName(filename).c_str(), 0);
	if (file == NULL) return false;
	zip_fclose(file);
	return true;
}

void BackupFiles(std::vector<std::string> files)
{
	int errn;
	zip_t* archive = zip_open((backup_path + backup_name).c_str(), ZIP_CREATE, &errn);

	for (const auto& file : files)
	{
		
		if (CheckIfFileExist(archive, file))
		{
			log_message("Modified: " + file);
			ZipRewriteFile(archive, file);
		}
		else
		{
			log_message("New: " + file);
			ZipCreateFile(archive, file);
		}
	}

	zip_close(archive);
}
