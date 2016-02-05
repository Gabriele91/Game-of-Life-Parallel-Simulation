//
//  file_os.cpp
//  Simulazione
//
//  Created by Gabriele on 04/02/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#include <file_os.h>
#include <sys/stat.h>

#if defined(_WIN32)
	#include <direct.h>
	#define mkdir(x,y) _mkdir(x) 
	#define S_ISDIR(ST_MODE) (((ST_MODE)& S_IFMT) == S_IFDIR)
    #define S_ISREG(ST_MODE) (((ST_MODE)& S_IFMT) == S_IFREG)
#endif

bool save_string(const std::string& text,const std::string& path)
{
    //..
    FILE* file = fopen(path.c_str(), "w");
    //file..
    if(file)
    {
        fwrite(text.c_str(), text.size(), 1, file);
        fclose(file);
        return true;
    }
    //return..
    return false;
}

bool create_dir(const std::string& path,size_t mode)
{
    return mkdir(path.c_str(),mode) == 0;
}

bool rename_dir(const std::string& path_form,const std::string& path_to)
{
    return rename(path_form.c_str(),path_to.c_str());
}

bool exists_dir(const std::string& path)
{
    struct stat sb;
    return (stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
}