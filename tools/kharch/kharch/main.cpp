#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <ctype.h>

using u32 = uint32_t;

struct file
{
	u32 size;
	u32 ofset;
	u32 namelen;
};
struct header
{
	u32 num_files;
};



void getFilesRecursive(TCHAR* path, std::vector<std::string>& files);
void makeFileArchive(const std::vector<std::string>& roots);
int main()
{
	std::vector<std::string> roots;
	roots.push_back(getenv("ALVAHSHI_CONTENT"));
	roots.push_back("D:/Alvahshi/game/sources/DanGameware/DanGameware/content/");
	makeFileArchive(roots);

}
u32 getFileSize(const char* fn)
{
	auto f = fopen(fn, "rb");
	fseek(f, 0L, SEEK_END);
	auto sz = ftell(f);
	fclose(f);
	return sz;
}
void makeFileArchive(const std::vector<std::string>& roots)
{
	std::vector<std::string> files;
	std::vector<std::string> pruned_filenames;
	for (const auto r : roots)
	{
		std::vector<std::string> _files;
		getFilesRecursive((TCHAR*)r.c_str(), _files);
		for (const auto& f : _files)
		{
			pruned_filenames.push_back(f.substr(r.length() + 1));
			files.push_back(f);
		}
	}
		


	
	
	auto f = fopen("D:/Alvahshi/game/sources/DanGameware/Package/arch.bag", "wb");
	
	fwrite("KAR1", 1, 4, f);

	header h;
	h.num_files = files.size();
	fwrite(&h, sizeof(header), 1, f);

	u32 strings_size = 0;
	for (const auto& fs : pruned_filenames)
	{
		strings_size += fs.length();
	}
	u32 file_data_ofset = 4+sizeof(header)+sizeof(file)*files.size()+ strings_size;

	int ind = 0;
	for (const auto& fs : files)
	{
		file ff;
		ff.size = getFileSize(fs.c_str());
		ff.ofset = file_data_ofset;
		ff.namelen = pruned_filenames[ind].size();
		file_data_ofset += ff.size;
		fwrite(&ff, sizeof(file), 1, f);
		ind++;
	};
	for (const auto& fs : pruned_filenames)
	{
		fwrite(fs.c_str(), 1, fs.size(), f);
	}
	for (const auto& s : files)
	{
		std::cout << "Archiving " << s<<std::endl;
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