#include "new_mr_droid.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../piranha/include/compiler.h"
#include "../piranha/include/error_list.h"
#include "../piranha/include/compilation_error.h"
#include "../engine-sim/scripting/include/language_rules.h"

namespace esdroid {
namespace NewMRDroidNS {

// eight rewrites is plenty for any community file
static constexpr int kMaxRounds = 8;

static bool readWholeFile(const std::string &path, std::string &out) {
    FILE *fp = fopen(path.c_str(), "rb");
    if (fp == nullptr) return false;
    fseek(fp, 0, SEEK_END);
    const long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (size < 0 || size > 4 * 1024 * 1024) { fclose(fp); return false; }
    out.resize((size_t)size);
    const bool ok = size == 0 ||
        fread(&out[0], 1, (size_t)size, fp) == (size_t)size;
    fclose(fp);
    return ok;
}

static bool writeWholeFile(const std::string &path, const std::string &text) {
    FILE *fp = fopen(path.c_str(), "wb");
    if (fp == nullptr) return false;
    const bool ok = fwrite(text.data(), 1, text.size(), fp) == text.size();
    fclose(fp);
    return ok;
}

// line count matches the compiler line numbers exactly
static std::vector<std::string> splitLines(const std::string &text) {
    std::vector<std::string> lines;
    std::string current;
    for (char c : text) {
        if (c == '\n') { lines.push_back(current); current.clear(); }
        else current += c;
    }
    lines.push_back(current);
    return lines;
}

static std::string joinLines(const std::vector<std::string> &lines) {
    std::string out;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i != 0) out += '\n';
        out += lines[i];
    }
    return out;
}

static std::string trimCopy(const std::string &s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) ++a;
    while (b > a && isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}

static bool isIdentChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9') || c == '_';
}

// marks every char that sits inside a double quoted string or a
// comment so scans never match text the parser ignores
static std::vector<bool> stringMask(const std::string &text) {
    std::vector<bool> mask(text.size(), false);
    bool inString = false;
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (inString) {
            mask[i] = true;
            if (c == '\\' && i + 1 < text.size()) { mask[i + 1] = true; ++i; }
            else if (c == '"') inString = false;
        }
        else if (c == '"') { inString = true; mask[i] = true; }
        else if (c == '/' && i + 1 < text.size() && text[i + 1] == '/') {
            while (i < text.size() && text[i] != '\n') { mask[i] = true; ++i; }
            if (i < text.size()) --i;
        }
    }
    return mask;
}

// finds the close paren matching the open paren at open
static long matchParen(const std::string &text, long open,
        const std::vector<bool> &str) {
    int depth = 0;
    for (long i = open; i < (long)text.size(); ++i) {
        if (str[(size_t)i]) continue;
        if (text[i] == '(') ++depth;
        else if (text[i] == ')') {
            --depth;
            if (depth == 0) return i;
        }
    }
    return -1;
}

// splits the inside of a call on top level commas
static std::vector<std::string> splitArgs(const std::string &args,
        const std::vector<bool> &str, size_t offset) {
    std::vector<std::string> parts;
    std::string cur;
    int depth = 0;
    for (size_t i = 0; i < args.size(); ++i) {
        const char c = args[i];
        const bool inStr = (offset + 1 + i < str.size()) && str[offset + 1 + i];
        if (c == '(' && !inStr) { ++depth; cur += c; }
        else if (c == ')' && !inStr) { --depth; cur += c; }
        else if (c == ',' && !inStr && depth == 0) {
            parts.push_back(cur);
            cur.clear();
        }
        else cur += c;
    }
    parts.push_back(cur);
    return parts;
}

// the newer format drives the whole sim through one run call
// the old format wants the three setters instead
static bool rewriteRunCalls(std::string &text) {
    bool changed = false;
    const std::vector<bool> str = stringMask(text);
    size_t search = 0;
    while (search < text.size()) {
        const size_t at = text.find("run", search);
        if (at == std::string::npos) break;
        search = at + 3;
        if (str[at]) continue;
        if (at != 0 && isIdentChar(text[at - 1])) continue;
        size_t p = at + 3;
        while (p < text.size() && isspace((unsigned char)text[p])) ++p;
        if (p >= text.size() || text[p] != '(' || str[p]) continue;
        // the call must start its own line
        size_t lineStart = text.rfind('\n', at);
        lineStart = (lineStart == std::string::npos) ? 0 : lineStart + 1;
        if (!trimCopy(text.substr(lineStart, at - lineStart)).empty()) continue;
        const long close = matchParen(text, (long)p, str);
        if (close < 0) continue;

        const std::string args = text.substr(p + 1, (size_t)close - p - 1);
        std::string engineVal, vehicleVal, transVal;
        const std::vector<std::string> parts = splitArgs(args, str, p);
        for (const std::string &part : parts) {
            const size_t colon = part.find(':');
            if (colon == std::string::npos) continue;
            const std::string key = trimCopy(part.substr(0, colon));
            const std::string val = trimCopy(part.substr(colon + 1));
            if (key == "engine") engineVal = val;
            else if (key == "vehicle") vehicleVal = val;
            else if (key == "transmission") transVal = val;
        }
        if (engineVal.empty() && vehicleVal.empty() && transVal.empty())
            continue;

        const std::string indent = text.substr(lineStart, at - lineStart);
        std::string replacement;
        if (!engineVal.empty()) replacement += "set_engine(" + engineVal + ")";
        if (!vehicleVal.empty()) {
            if (!replacement.empty()) replacement += "\n" + indent;
            replacement += "set_vehicle(" + vehicleVal + ")";
        }
        if (!transVal.empty()) {
            if (!replacement.empty()) replacement += "\n" + indent;
            replacement += "set_transmission(" + transVal + ")";
        }
        text.replace(lineStart, (size_t)close + 1 - lineStart, replacement);
        changed = true;
        search = lineStart + replacement.size();
    }
    return changed;
}

// structural words that must never be treated as a strippable attr
static bool hasKeywordPrefix(const std::string &line) {
    static const char *words[] = {
        "input ", "output ", "label ", "alias ", "node ", "import ",
        "public ", "private ", "function ", "run ", "set_", "units ",
        "constants ", "main ", "return "
    };
    const std::string t = trimCopy(line);
    for (const char *w : words) {
        if (t.compare(0, strlen(w), w) == 0) return true;
    }
    return false;
}

// true for a single name value pair like bore: 36 units.mm
static bool isAttributeLine(const std::string &line) {
    const std::string t = trimCopy(line);
    if (t.empty()) return false;
    size_t i = 0;
    if (!(isalpha((unsigned char)t[0]) || t[0] == '_')) return false;
    while (i < t.size() && isIdentChar(t[i])) ++i;
    if (i >= t.size() || t[i] != ':') return false;
    return true;
}

// the name before the colon of an attribute line empty when the
// line is not one
static std::string attributeLineName(const std::string &line) {
    if (!isAttributeLine(line)) return std::string();
    const std::string t = trimCopy(line);
    return t.substr(0, t.find(':'));
}

// ports that only exist in newer closed engine sim builds nothing
// in the shipped library defines any of these names
// turbulence_to_flame_speed_ratio is deliberately absent it is a
// real fuel port here so a file may legitimately use it
static const char *kUnambiguousNewAttrs[] = {
    "convolution", "fluid_simulation_steps", "max_brake_force",
    "clutch_stiffness", "clutch_damping", "max_clutch_flex",
    "limit_clutch_flex", "simulate_flex"
};

// short flex disc ports that old scripts could legally reuse for
// their own nodes so they only go when a marker above proves the
// file is new format
static const char *kShortNewAttrs[] = {
    "stiffness", "damping", "max_flex", "limit_flex"
};

static bool nameInTable(const std::string &name, const char * const *table,
        size_t count) {
    for (size_t i = 0; i < count; ++i)
        if (name == table[i]) return true;
    return false;
}

// drops a trailing separator comma ignoring comments strings and
// nested parens returns true when the line changed
static bool removeTrailingComma(std::string &line) {
    int depth = 0;
    bool inString = false;
    size_t lastTopComma = std::string::npos;
    for (size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (inString) {
            if (c == '\\') { ++i; continue; }
            if (c == '"') inString = false;
            continue;
        }
        if (c == '"') { inString = true; continue; }
        if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') break;
        if (c == '(') ++depth;
        else if (c == ')') --depth;
        else if (c == ',' && depth == 0) lastTopComma = i;
    }
    if (lastTopComma == std::string::npos) return false;
    line.erase(lastTopComma, 1);
    return true;
}

// drops the marked lines and repairs the comma chain
// returns the dropped count or -1 when a line was refused
static int stripLines(std::vector<std::string> &lines,
        const std::vector<int> &lineNos) {
    std::vector<bool> drop(lines.size(), false);
    int dropped = 0;
    for (int no : lineNos) {
        const size_t idx = (size_t)(no - 1);
        if (idx >= lines.size() || drop[idx]) continue;
        if (!isAttributeLine(lines[idx]) || hasKeywordPrefix(lines[idx]))
            return -1;
        drop[idx] = true;
        ++dropped;
    }
    if (dropped == 0) return 0;
    std::vector<std::string> out;
    out.reserve(lines.size());
    for (size_t i = 0; i < lines.size(); ++i) {
        if (!drop[i]) out.push_back(lines[i]);
    }
    // when the next kept line closes the call the previous kept line
    // cannot keep its trailing comma
    for (size_t i = 0; i + 1 < out.size(); ++i) {
        const std::string next = trimCopy(out[i + 1]);
        if (!next.empty() && next[0] == ')') removeTrailingComma(out[i]);
    }
    lines.swap(out);
    return dropped;
}

// drops the known newer format attribute lines up front so a
// community file settles in one trial compile instead of a slow
// discovery loop returns true when lines were dropped
static bool stripKnownNewAttrs(std::vector<std::string> &lines) {
    bool marker = false;
    std::vector<int> drop;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string name = attributeLineName(lines[i]);
        if (name.empty() || hasKeywordPrefix(lines[i])) continue;
        if (nameInTable(name, kUnambiguousNewAttrs,
                sizeof(kUnambiguousNewAttrs) / sizeof(kUnambiguousNewAttrs[0]))) {
            marker = true;
            drop.push_back((int)(i + 1));
        }
    }
    if (!marker) return false;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string name = attributeLineName(lines[i]);
        if (name.empty() || hasKeywordPrefix(lines[i])) continue;
        if (nameInTable(name, kShortNewAttrs,
                sizeof(kShortNewAttrs) / sizeof(kShortNewAttrs[0])))
            drop.push_back((int)(i + 1));
    }
    return stripLines(lines, drop) > 0;
}

enum class CheckResult { Clean, Errors, IoFail };

// the same shape the engine sim error log prints one line per error
// capped so a broken file cannot flood the dialog
static void formatErrorText(const piranha::ErrorList *errors,
        std::string &out) {
    const int count = errors->getErrorCount();
    int written = 0;
    for (int i = 0; i < count && written < 10; ++i) {
        const piranha::CompilationError *e = errors->getCompilationError(i);
        const piranha::ErrorCode_struct &code = e->getErrorCode();
        char line[512];
        snprintf(line, sizeof(line), "%s(%d): error %s%s: %s\n",
            e->getCompilationUnit()->getPath().getStem().c_str(),
            e->getErrorLocation()->lineStart,
            code.stage.c_str(), code.code.c_str(), code.info.c_str());
        out += line;
        ++written;
    }
    if (count > written) {
        char tail[64];
        snprintf(tail, sizeof(tail), "... and %d more\n", count - written);
        out += tail;
    }
}

// compiles the file through the same rules the app ships
// badLines collects the lines the parser rejected as unknown ports
// errorText collects the readable error lines for the dialog
static CheckResult compileCheck(const std::string &path,
        const std::string &assetsBase, std::vector<int> &badLines,
        std::string *errorText) {
    if (assetsBase.empty()) return CheckResult::IoFail;

    es_script::LanguageRules rules;
    piranha::Compiler compiler(&rules);
    compiler.setFileExtension(".mr");
    // the same list es script compiler initialize builds on android
    const char *rel[] = {
        "../../es/", "../es/", "es/", "../assets/", "assets/", "./", nullptr
    };
    for (const char **d = rel; *d != nullptr; ++d)
        compiler.addSearchPath(piranha::IrPath(*d));
    const char *sub[] = {
        "es/", "es/part-library/", "es/part-library/parts/",
        "es/sound-library/", "es/sound-library/archive/",
        "es/sound-library/new/", "es/sound-library/sharp/",
        "es/sound-library/smooth/", "es/types/", "es/constants/",
        "es/actions/", "es/objects/", "es/infrastructure/",
        "es/settings/", "es/utilities/", "themes/",
        "engines/", "engines/atg-video-2/", "engines/atg-video-1/",
        "part-library/", nullptr
    };
    for (const char **s = sub; *s != nullptr; ++s)
        compiler.addSearchPath(piranha::IrPath(assetsBase + "/" + *s));
    compiler.addSearchPath(piranha::IrPath(assetsBase + "/"));
    rules.initialize();

    piranha::IrCompilationUnit *unit =
        compiler.compile(piranha::IrPath(path));
    if (unit == nullptr) {
        compiler.free();
        return CheckResult::IoFail;
    }
    const piranha::ErrorList *errors = compiler.getErrorList();
    if (errorText != nullptr) formatErrorText(errors, *errorText);
    const int count = errors->getErrorCount();
    for (int i = 0; i < count; ++i) {
        const piranha::CompilationError *e = errors->getCompilationError(i);
        // only errors inside the imported file itself can be fixed
        if (e->getCompilationUnit() != unit) continue;
        const piranha::ErrorCode_struct &code = e->getErrorCode();
        const bool portNotFound =
            code.stage == piranha::ErrorCode::PortNotFound.stage
            && code.code == piranha::ErrorCode::PortNotFound.code;
        if (portNotFound)
            badLines.push_back(e->getErrorLocation()->lineStart);
    }
    compiler.free();
    return (count == 0) ? CheckResult::Clean : CheckResult::Errors;
}

} // namespace NewMRDroidNS

NewMRDroid::Result NewMRDroid::translate(const std::string &path,
        const std::string &assetsBase, ResultInfo *info) {
    using namespace NewMRDroidNS;

    std::string original;
    if (!readWholeFile(path, original)) return Rejected;

    std::string text = original;
    rewriteRunCalls(text);
    {
        // the known newer constructs go before any compile so the
        // community files settle in one round
        std::vector<std::string> lines = splitLines(text);
        if (stripKnownNewAttrs(lines)) text = joinLines(lines);
    }

    // the scratch sits next to the picked copy so imports resolve
    // the same way they will inside the app
    std::string scratch = path;
    {
        const size_t slash = scratch.rfind('/');
        const size_t dot = scratch.rfind('.');
        if (dot != std::string::npos && dot > slash)
            scratch.replace(dot, std::string::npos, "_check.mr");
        else scratch += "_check.mr";
    }

    Result result = Rejected;
    std::vector<int> badLines;
    std::string lastErrorText;
    int rounds = 0;
    // a hostile file can throw inside the parser the worker thread
    // must never die from one
    try {
        for (int round = 0; round < kMaxRounds; ++round) {
            ++rounds;
            if (!writeWholeFile(scratch, text)) break;
            badLines.clear();
            std::string errorText;
            const CheckResult check =
                compileCheck(scratch, assetsBase, badLines, &errorText);
            if (check == CheckResult::IoFail) break;
            if (check == CheckResult::Clean) {
                lastErrorText.clear();
                if (text == original) { result = Pass; break; }
                // the picked copy is scratch so the fix lands right on it
                if (writeWholeFile(path, text)) result = Fixed;
                break;
            }
            lastErrorText = errorText;
            bool changed = rewriteRunCalls(text);
            if (!badLines.empty()) {
                std::vector<std::string> lines = splitLines(text);
                const int stripped = stripLines(lines, badLines);
                if (stripped < 0) break;
                if (stripped > 0) { text = joinLines(lines); changed = true; }
            }
            // errors that no rewrite can touch mean the file needs a
            // newer engine sim than this one
            if (!changed) break;
        }
    }
    catch (...) {
        result = Rejected;
    }

    remove(scratch.c_str());
    if (info != nullptr) {
        info->errorText = lastErrorText;
        info->rounds = rounds;
    }
    return result;
}

} // namespace esdroid
