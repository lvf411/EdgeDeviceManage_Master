#include "file.hpp"

char SizeUnit[5] = {' ' , 'K', 'M', 'G', 'T'};

//FileInfo 实例初始化
void FileInfoInit(FileInfo *info)
{
    info->fname.clear();
    info->filesize = 0;
    info->exatsize = 0;
    info->unit = ' ';
    info->md5.clear();
}

//根据给定的文件路径获取文件的元数据
bool FileInfoGet(std::string path, FileInfo *info)
{
    std::ifstream f(path.c_str());
    if(!f.good())
    {
        printf("FileInfoGet open file error\n");
        return false;
    }
    info->fname = path;
    f.seekg(0, std::ios::end);
    info->exatsize = f.tellg();
    int u = 0;
    uint32_t size = info->exatsize;
    while(size > 10)
    {
        size /= 1000;
        u = u + 1;
    }
    info->filesize = size;
    info->unit = SizeUnit[u];
    info->md5 = FileDigest(path);
    return true;
}
