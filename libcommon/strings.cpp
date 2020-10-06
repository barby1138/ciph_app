
#include "strings.h"

#ifdef _WIN32
#include <windows.h>
#elif defined _LINUX
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <dirent.h>
#endif

void getDirFileList(const std::string& directory, const std::string& cfilter, std::set<std::string>& files)
{
#ifdef WIN32
	HANDLE dir;
	WIN32_FIND_DATA file_data;
	std::string filter("\\*" + cfilter);

	if ((dir = FindFirstFile((directory + filter).c_str(), &file_data)) == INVALID_HANDLE_VALUE)
		return; /* No files found */

	do {
		const std::string file_name = file_data.cFileName;
		const std::string full_file_name = directory + "\\" + file_name;
		const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		if (file_name[0] == '.')
			continue;

		if (is_directory)
			continue;

		//TRACE_INFO("%s", full_file_name.c_str());
		printf("%s", full_file_name.c_str());

		files.insert(full_file_name);
	} while (FindNextFile(dir, &file_data));

	FindClose(dir);
#elif defined _LINUX
	DIR *dir;
	class dirent *ent;
	class stat st;
	std::string filter(cfilter);

	// TODO handle error
	dir = opendir(directory.c_str());
	if (NULL == dir)
	{
		printf("opendir failed %s", directory.c_str());
		return;
	}

	while ((ent = readdir(dir)) != NULL)
	{
		const std::string file_name = ent->d_name;
		const std::string full_file_name = directory + "/" + file_name;

		if (!strstr(file_name.c_str(), filter.c_str()))
			continue;

		if (file_name[0] == '.')
			continue;

		if (stat(full_file_name.c_str(), &st) == -1)
			continue;

		const bool is_directory = (st.st_mode & S_IFDIR) != 0;

		if (is_directory)
			continue;

		//printf("%s\n", full_file_name.c_str());

		files.insert(full_file_name);
	}

	printf("%lu\n", files.size());

	closedir(dir);
#endif
}

