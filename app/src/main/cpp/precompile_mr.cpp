/*
 * Copyright 2026 F² Cyanic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <cstring>

#include "compiler.h"
#include "path.h"
#include "ir_compilation_unit.h"
#include "error_list.h"
#include "compilation_error.h"
#include "engine_context.h"
#include "language_rules.h"
#include "actions.h"
#include "application_settings.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <assets_dir> <output.bin>" << std::endl;
        return 1;
    }

    std::string assetsDir = argv[1];
    std::string outputPath = argv[2];
    std::string mrPath = assetsDir + "/main.mr";

    std::cerr << "Precompiling: " << mrPath << std::endl;

    es_script::Compiler compiler;
    compiler.initialize();

    // Add ALL search paths
    std::string dirs[] = {
        "/es/", "/es/part-library/", "/es/part-library/parts/",
        "/es/sound-library/", "/es/sound-library/archive/",
        "/es/sound-library/new/", "/es/sound-library/sharp/",
        "/es/sound-library/smooth/", "/es/types/", "/es/constants/",
        "/es/actions/", "/es/objects/", "/es/infrastructure/",
        "/es/settings/", "/es/utilities/", "/themes/",
        "/engines/", "/engines/atg-video-2/", "/engines/atg-video-1/",
        "/part-library/", "/"
    };
    for (auto &d : dirs) {
        compiler.addSearchPath(assetsDir + d);
    }

    bool compiled = compiler.compile(piranha::Path(mrPath));
    if (!compiled) {
        std::cerr << "Compilation FAILED" << std::endl;
        const piranha::ErrorList *errors = compiler.getErrorList();
        for (int i = 0; i < errors->getErrorCount(); ++i) {
            auto *err = errors->getCompilationError(i);
            std::cerr << "Error " << (i+1) << "/" << errors->getErrorCount() << ": ";
            if (err->getErrorLocation()) {
                std::cerr << err->getErrorLocation()->getFile().getStem() << "("
                          << err->getErrorLocation()->getLineStart() << "): ";
            }
            std::cerr << err->getErrorMessage() << std::endl;
        }
        return 1;
    }

    std::cerr << "Compilation OK, executing..." << std::endl;
    auto output = compiler.execute();

    std::cerr << "Engine: " << (output.engine ? "YES" : "NULL") << std::endl;
    std::cerr << "Vehicle: " << (output.vehicle ? "YES" : "NULL") << std::endl;
    std::cerr << "Transmission: " << (output.transmission ? "YES" : "NULL") << std::endl;

    // Write binary cache
    std::ofstream out(outputPath, std::ios::binary);
    uint32_t magic = 0x4553524E, version = 1;
    out.write(reinterpret_cast<char*>(&magic), 4);
    out.write(reinterpret_cast<char*>(&version), 4);

    uint8_t hasEngine = output.engine ? 1 : 0;
    uint8_t hasVehicle = output.vehicle ? 1 : 0;
    uint8_t hasTransmission = output.transmission ? 1 : 0;
    out.write(reinterpret_cast<char*>(&hasEngine), 1);
    out.write(reinterpret_cast<char*>(&hasVehicle), 1);
    out.write(reinterpret_cast<char*>(&hasTransmission), 1);

    // Write application settings
    out.write(reinterpret_cast<char*>(&output.applicationSettings),
              sizeof(output.applicationSettings));

    out.close();
    std::cerr << "Cache written to: " << outputPath << std::endl;

    compiler.destroy();
    return 0;
}
