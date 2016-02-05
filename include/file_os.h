//
//  file_os.h
//  Simulazione
//
//  Created by Gabriele on 04/02/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#pragma once
#include <string>

extern bool save_string(const std::string& text,const std::string& path);
extern bool create_dir(const std::string& path,size_t mode);
extern bool rename_dir(const std::string& path_form,const std::string& path_to);
extern bool exists_dir(const std::string& path);