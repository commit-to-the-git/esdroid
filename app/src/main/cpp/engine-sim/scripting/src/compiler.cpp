#if defined(__ANDROID__)
extern "C" void esdroid_wtflog(const char*,...);
#endif
#include <string>
#if defined(__ANDROID__)
extern "C" const char *esdroid_get_files_dir();
#endif
#include <string>
#include "../include/compiler.h"

es_script::Compiler::Output *es_script::Compiler::s_output = nullptr;

es_script::Compiler::Compiler() {
    m_compiler = nullptr;
}

es_script::Compiler::~Compiler() {
    assert(m_compiler == nullptr);
}

es_script::Compiler::Output *es_script::Compiler::output() {
    if (s_output == nullptr) {
        s_output = new Output;
    }

    return s_output;
}

void es_script::Compiler::initialize() {
    m_compiler = new piranha::Compiler(&m_rules);
    m_compiler->setFileExtension(".mr");

    m_compiler->addSearchPath("../../es/");
    m_compiler->addSearchPath("../es/");
    m_compiler->addSearchPath("es/");
#if defined(__ANDROID__)
    m_compiler->addSearchPath("../assets/");
    m_compiler->addSearchPath("assets/");
    m_compiler->addSearchPath("./");
    const char *fd = esdroid_get_files_dir();
    if (fd && fd[0]) {
        std::string base = std::string(fd) + "/assets";
        m_compiler->addSearchPath(base + "/");
        m_compiler->addSearchPath(base + "/es/");
        m_compiler->addSearchPath(base + "/es/part-library/");
        m_compiler->addSearchPath(base + "/es/part-library/parts/");
        m_compiler->addSearchPath(base + "/es/sound-library/");
        m_compiler->addSearchPath(base + "/es/sound-library/archive/");
        m_compiler->addSearchPath(base + "/es/sound-library/new/");
        m_compiler->addSearchPath(base + "/es/sound-library/sharp/");
        m_compiler->addSearchPath(base + "/es/sound-library/smooth/");
        m_compiler->addSearchPath(base + "/es/types/");
        m_compiler->addSearchPath(base + "/es/constants/");
        m_compiler->addSearchPath(base + "/es/actions/");
        m_compiler->addSearchPath(base + "/es/objects/");
        m_compiler->addSearchPath(base + "/es/infrastructure/");
        m_compiler->addSearchPath(base + "/es/settings/");
        m_compiler->addSearchPath(base + "/es/utilities/");
        m_compiler->addSearchPath(base + "/themes/");
        m_compiler->addSearchPath(base + "/engines/");
        m_compiler->addSearchPath(base + "/engines/atg-video-2/");
        m_compiler->addSearchPath(base + "/engines/atg-video-1/");
        m_compiler->addSearchPath(base + "/part-library/");
    }
#endif

    m_rules.initialize();
}

bool es_script::Compiler::compile(const piranha::IrPath &path) {
    bool successful = false;

    #if defined(__ANDROID__)
    std::ofstream file("/data/data/com.esdroid.engine_sim/wtflogs/piranha_errors.log", std::ios::out);
#else
    std::ofstream file("error_log.log", std::ios::out);
#endif
    piranha::IrCompilationUnit *unit = m_compiler->compile(path);
    if (unit == nullptr) {
        file << "Can't find file: " << path.toString() << "\n";
    }
    else {
        const piranha::ErrorList *errors = m_compiler->getErrorList();
        if (errors->getErrorCount() == 0) {
            unit->build(&m_program);

            m_program.initialize();

            successful = true;
        }
        else {
            for (int i = 0; i < errors->getErrorCount(); ++i) {
                printError(errors->getCompilationError(i), file);
#if defined(__ANDROID__)
                esdroid_wtflog("Piranha error %d/%d", i+1, errors->getErrorCount());
#endif
            }
        }
    }

    file.close();

    return successful;
}

es_script::Compiler::Output es_script::Compiler::execute() {
    // the output is static so it survives between compiles clear the object
    // pointers so a script that does not call set_engine cannot hand back an
    // engine owned by the previous run
    output()->engine = nullptr;
    output()->vehicle = nullptr;
    output()->transmission = nullptr;

    #if defined(__ANDROID__)
    esdroid_wtflog("execute: calling m_program.execute()");
#endif
    bool result = false;
    try {
        result = m_program.execute();
    } catch (const std::bad_alloc& e) {
        esdroid_wtflog("execute: bad_alloc caught: %s", e.what());
    } catch (const std::exception& e) {
        esdroid_wtflog("execute: exception caught: %s", e.what());
    } catch (...) {
        esdroid_wtflog("execute: unknown exception caught");
    }
#if defined(__ANDROID__)
    esdroid_wtflog("execute: m_program.execute() returned %d", result ? 1 : 0);
#endif

    if (!result) {
        // todo runtime error
    }

    return *output();
}

void es_script::Compiler::destroy() {
    m_program.free();
    m_compiler->free();

    delete m_compiler;
    m_compiler = nullptr;
}

void es_script::Compiler::printError(
    const piranha::CompilationError *err,
    std::ofstream &file) const
{
    const piranha::ErrorCode_struct &errorCode = err->getErrorCode();
    file << err->getCompilationUnit()->getPath().getStem()
        << "(" << err->getErrorLocation()->lineStart << "): error "
        << errorCode.stage << errorCode.code << ": " << errorCode.info << std::endl;

    piranha::IrContextTree *context = err->getInstantiation();
    while (context != nullptr) {
        piranha::IrNode *instance = context->getContext();
        if (instance != nullptr) {
            const std::string instanceName = instance->getName();
            const std::string definitionName = (instance->getDefinition() != nullptr)
                ? instance->getDefinition()->getName()
                : "<Type Error>";
            const std::string formattedName = (instanceName.empty())
                ? "<unnamed> " + definitionName
                : instanceName + " " + definitionName;

            file
                << "       While instantiating: "
                << instance->getParentUnit()->getPath().getStem()
                << "(" << instance->getSummaryToken()->lineStart << "): "
                << formattedName << std::endl;
        }

        context = context->getParent();
    }
}
