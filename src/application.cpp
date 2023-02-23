// Display images inside a terminal
// Copyright (C) 2023  JustKidding
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "application.hpp"
#include "process_info.hpp"
#include "os.hpp"
#include "version.hpp"
#include "dimensions.hpp"
#include "util.hpp"

#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/basic_file_sink.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

Application::Application():
terminal(ProcessInfo(os::get_pid()))
{
    setup_logger();
    logger->info("Started ueberzug++ {}.{}.{}", ueberzugpp_VERSION_MAJOR,
            ueberzugpp_VERSION_MINOR, ueberzugpp_VERSION_PATCH);
    canvas = Canvas::create(terminal, *logger);
    auto cache_path = util::get_cache_path();
    if (!fs::exists(cache_path)) fs::create_directories(cache_path);
}

Application::~Application()
{
    canvas->clear();
    if (f_stderr) std::fclose(f_stderr);
}

auto Application::execute(const std::string& cmd) -> void
{
    json j;
    try {
        j = json::parse(cmd);
    } catch (const json::parse_error& e) {
        logger->error("There was an error parsing the command.");
        return;
    }
    logger->info("Command received: {}", j.dump());
    if (j["action"] == "add") {
        Dimensions dimensions(terminal, j["x"], j["y"], j["max_width"], j["max_height"]);
        image = Image::load(terminal, dimensions, j["path"], *logger);
        canvas->init(dimensions, image);
        if (!image) {
            logger->warn("Unable to load image file.");
            return;
        }
        canvas->draw();
    } else if (j["action"] == "remove") {
        logger->info("Removing image.");
        canvas->clear();
    } else {
        logger->warn("Command not supported.");
    }
}

auto Application::setup_logger() -> void
{
    std::string log_tmp = "ueberzug_" + os::getenv("USER").value() + ".log";
    fs::path log_path = fs::temp_directory_path() / log_tmp;
    try {
        logger = spdlog::basic_logger_mt("main", log_path);
        logger->flush_on(spdlog::level::info);
    } catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Log init failed: " << ex.what() << std::endl;
    }
}

auto Application::command_loop(const std::atomic<bool>& stop_flag) -> void
{
    std::string cmd;
    while (std::getline(std::cin, cmd)) {
        if (stop_flag.load()) break;
        execute(cmd);
    }
}

auto Application::set_silent(bool silent) -> void
{
    if (!silent) return;
    f_stderr = freopen("/dev/null", "w", stderr);
}