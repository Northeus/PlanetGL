// ################################################################################
// Common Framework for Computer Graphics Courses at FI MUNI.
// 
// Copyright (c) 2021-2022 Visitlab (https://visitlab.fi.muni.cz)
// All rights reserved.
// ################################################################################

#pragma once

#include "iapplication.h" // to get GLAD
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

std::string load_file(std::filesystem::path file_path);

GLuint create_shader(std::filesystem::path file_path, GLenum shader_type);

GLuint create_program(std::filesystem::path vertex_path, std::filesystem::path fragment_path);
