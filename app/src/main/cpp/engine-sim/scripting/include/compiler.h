#ifndef ATG_ENGINE_SIM_COMPILER_H
#define ATG_ENGINE_SIM_COMPILER_H

#include "language_rules.h"

#include "engine_sim.h"
#include "piranha.h"

#include <vector>
#include <string>

namespace es_script {

    class Compiler {
    public:
        struct Output {
            Engine *engine = nullptr;
            Vehicle *vehicle = nullptr;
            Transmission *transmission = nullptr;
            Simulator::Parameters simulatorParameters;
            ApplicationSettings applicationSettings;

            std::vector<Function *> functions;
        };

    private:
        static Output *s_output;
        // readable error text from the last compile survives the app
        // object so the load fallback can show it
        static std::string s_lastErrors;

    public:
        Compiler();
        ~Compiler();

        static Output *output();
        static const std::string &lastErrors();

        void initialize();
        bool compile(const piranha::IrPath &path);
        Output execute();
        void destroy();

    private:
        void printError(const piranha::CompilationError *err, std::ostream &file) const;

    private:
        LanguageRules m_rules;
        piranha::Compiler *m_compiler;
        piranha::NodeProgram m_program;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_COMPILER_H  */