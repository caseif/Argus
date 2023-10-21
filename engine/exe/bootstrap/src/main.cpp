/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"
#include "argus/core.hpp"

#include <cstdio>

static argus::Logger g_logger("Bootstrap");

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Invalid arguments\nUsage: %s <namespace>", argc >= 1 ? argv[0] : "argus_bootstrap");
    }

    argus::load_client_config(argv[1]);
    g_logger.debug("Loaded client config");

    argus::initialize_engine();
    g_logger.debug("Engine initialized");

    g_logger.debug("Starting engine...");
    argus::start_engine(nullptr);
}
