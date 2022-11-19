#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <ctype.h>
struct file
{
	uint32_t size;
	uint32_t ofset;
};
struct header
{
	uint32_t num_files;
};
void getFilesRecursive(TCHAR* path, std::vector<std::string>& files);
void makeFileArchive();
int main()
{
	makeFileArchive();
}
void makeFileArchive()
{
	std::vector<std::string> files;
	getFilesRecursive("D:/alv/content", files);
	for (const auto& f : files)
	{
	}
	auto f = fopen("arch.bag", "wb");
	fwrite("KAR1", 1, 4, f);
	header h;
	h.num_files = files.size();
	fwrite(&h, sizeof(header), 1, f);
	for (const auto& fs : files)
	{
		file ff;
		ff.size = 0;
		ff.ofset = 0;
		fwrite(&ff, sizeof(file), 1, f);
	};
	byte z = 0;
	for (const auto& fs : files)
	{
		fwrite(fs.c_str(), 1, fs.size(), f);
		fwrite(&z, 1, 1, f);
	}
	for (const auto& s : files)
	{
		auto f2 = fopen(s.c_str(), "rb");
		byte buffer[4096];
		int l = fread(buffer, sizeof(byte), 4096, f2);
		while (l > 0)
		{
			fwrite(buffer, sizeof(byte), l, f);
			l = fread(buffer, sizeof(byte), 4096, f2);
		}
		fclose(f2);
	}
	fclose(f);
	std::cout << "Done" << std::endl;
}

void getFilesRecursive(TCHAR* path, std::vector<std::string>& files)
{
	TCHAR path_ast[MAX_PATH];
	sprintf(path_ast, "%s/*", path);
	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(path_ast, &data);//DIRECTORY
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (strcmp(data.cFileName, ".") == 0 || strcmp(data.cFileName, "..") == 0)
				continue;
			bool is_dir = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
			if (is_dir)
			{
				TCHAR path_dir[MAX_PATH];
				sprintf(path_dir, "%s/%s", path,data.cFileName);
				getFilesRecursive(path_dir, files);
			}
			else
			{
				TCHAR path_file[MAX_PATH];
				sprintf(path_file, "%s/%s", path, data.cFileName);
				files.push_back(path_file);
			}
		}
		while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}
}