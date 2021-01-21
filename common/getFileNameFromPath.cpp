#include "getFileNameFromPath.h"


#ifdef _WIN32

#include <io.h>

#ifdef WIN32
std::vector<std::string> GetFileNameFromPath(const char* pPathName)
{
	std::vector<std::string> flist;
	//HANDLE file;
	//WIN32_FIND_DATA fileData;
	//char line[1024];
	//wchar_t fn[1000];
	//mbstowcs(fn, pPathName, 999);
	//file = FindFirstFile(fn, &fileData);
	////FindNextFile(file, &fileData);
	//while (FindNextFile(file, &fileData)) {
	//	wcstombs(line, (const wchar_t*)fileData.cFileName, 259);
	//	flist.push_back(line);
	//}


	_finddata_t file;
	long lf;

	std::string strbuff = pPathName;
	strbuff  += "*.*";
	if ((lf = _findfirst(strbuff.c_str(), &file)) == -1l)
	{

	}
	else
	{
		while (_findnext(lf, &file) == 0)
		{
			if (file.attrib == _A_SUBDIR)
			{
				//if (strcmp(file.name, "..") == 0)
				//{
				//	continue;
				//}
				//string newdir = dir + file.name + "\\";
				//LoadDir(newdir);
			}
			else
			{
				//string filename = dir + file.name;
				//m_CodeLoader.LoadFile(filename.c_str());
				flist.push_back(file.name);
			}
		}
	}
	_findclose(lf);

	return flist;
}
#else
std::vector<std::string> GetFileNameFromPath(const char* pPathName)
{
	std::vector<std::string> flist;

	__finddata64_t file;
	__int64 lf;

	std::string strbuff = pPathName;
	strbuff += "*.*";
	if ((lf = _findfirst64(strbuff.c_str(), &file)) == -1l)
	{

	}
	else
	{
		while (_findnext64(lf, &file) == 0)
		{
			if (file.attrib == _A_SUBDIR)
			{
}
			else
			{
				flist.push_back(file.name);
			}
		}
	}
	_findclose(lf);

	return flist;
}
#endif
#endif

#ifndef _WIN32
#include <sys/stat.h>
#include "dirent.h"
#include "unistd.h"
std::vector<std::string> GetFileNameFromPath(const char* pPathName)
{
	std::vector<std::string> path_vec;    
	if (pPathName == nullptr)
	{
		return path_vec;
	}

	DIR* dp;    
	struct dirent* entry;    
	struct stat statbuf;         
	if ((dp = opendir(pPathName)) == nullptr)
	{ 
		//fprintf(stderr, "cannot open %s", filePath.c_str());
		return path_vec;
	}    
	chdir(filePath.c_str());
	while ((entry = readdir(dp)) != nullptr)
	{ 
		stat(entry->d_name, &statbuf);
		if (!S_ISREG(statbuf.st_mode))
			continue;
		path_vec.push_back(StringUtils::format("%s", entry->d_name));
	}    
	return path_vec;

}

#endif