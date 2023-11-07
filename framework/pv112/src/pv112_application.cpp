// ################################################################################
// Common Framework for Computer Graphics Courses at FI MUNI.
// 
// Copyright (c) 2021-2022 Visitlab (https://visitlab.fi.muni.cz)
// All rights reserved.
// ################################################################################

#include "pv112_application.hpp"

// ----------------------------------------------------------------------------
// Constructors & Destructors
// ----------------------------------------------------------------------------

PV112Application::PV112Application(int initial_width, int initial_height, std::vector<std::string> arguments)
    : GUIApplication(initial_width, initial_height, arguments) {
    lecture_shaders_path = lecture_folder_path / "shaders";
}

PV112Application::~PV112Application() = default;
