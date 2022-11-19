#pragma once
#include <vector>
#include <windows.h>

using u32 = unsigned int;
using byte = unsigned char;
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
struct readfile
{
	u32 size;
	u32 ofset;
	std::string name;
	std::string archive;
	readfile()
	{
		size = ofset = 0;
	}
};
class ArchiveReader
{
	std::vector<readfile> files;
public:
	void addArchive(const char* path)
	{
		auto f = fopen(path, "rb");
		char buf[5] = { 0 };
		fread(buf, 1, 4, f);
		if (strcmp(buf, "KAR1") != 0)
		{
			//invalid archive;
			return;
		}
		header h;
		fread(&h, sizeof(header), 1, f);

		std::vector<file> files;
		std::vector<std::string> names;
		for (int i = 0; i < h.num_files; i++)
		{
			file rf;
			fread(&rf, sizeof(file), 1, f);
			files.push_back(rf);
		}
		for (int i = 0; i < h.num_files; i++)
		{
			readfile rf;
			rf.archive = path;
			rf.ofset = files[i].ofset;
			rf.size = files[i].size;

			u32 read = 0;
			auto nl = files[i].namelen;
			byte *buf = new byte[nl + 1]();
			fread(buf, 1, nl, f);
			buf[nl] = 0;
			rf.name = (const char*)(buf);
			this->files.push_back(rf);
		}
		fclose(f);
	}
	const readfile* getFile(std::string name)
	{
		for (int i = 0; i < name.length(); i++)
		{
			if (name[i] == '\\')
				name[i] = '/';
		}
		for (const auto& f : this->files)
		{
			if (name == f.name)
				return &f;
		}
		return nullptr;
	}
	byte* loadFileData(const char *name,size_t& out_size)
	{
		auto fs = getFile(name);
		if (fs == nullptr)
			return nullptr;
		out_size = fs->size;
		byte* ret = new byte[out_size];
		auto f = fopen(fs->archive.c_str(), "rb");
		fseek(f, fs->ofset, 0);
		fread(ret, 1, out_size, f);
		fclose(f);
		return ret;
	}
	void loadResourcesFromKhArchive(std::function<void()> update)
	{
		int res = h3dQueryUnloadedResource(0);
		while (res != 0)
		{
			auto resname = h3dGetResName(res);
			size_t sz;
			char* data = (char*)loadFileData(resname, sz);
			h3dLoadResource(res, data, sz);
			update();
			res = h3dQueryUnloadedResource(0);
		}
	}

	void addDirectory(const char* path)
	{
		std::vector<std::string> files;
		std::vector<std::string> pruned_filenames;
		getFilesRecursive((TCHAR*)path, files);
		for (const auto& f : files)
		{
			auto pruned_fn = f.substr(strlen(path) + 1);
			readfile rf;
			rf.archive = f;
			rf.ofset = 0;
			rf.size = getFileSize(f.c_str());
			rf.name = pruned_fn;
			this->files.push_back(rf);
		}
	}
	u32 getFileSize(const char* fn)
	{
		auto f = fopen(fn, "rb");
		fseek(f, 0L, SEEK_END);
		auto sz = ftell(f);
		fclose(f);
		return sz;
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
					sprintf(path_dir, "%s/%s", path, data.cFileName);
					getFilesRecursive(path_dir, files);
				}
				else
				{
					TCHAR path_file[MAX_PATH];
					sprintf(path_file, "%s/%s", path, data.cFileName);
					files.push_back(path_file);
				}
			} while (FindNextFile(hFind, &data));
			FindClose(hFind);
		}
	}
};
extern ArchiveReader g_archive_reader;

