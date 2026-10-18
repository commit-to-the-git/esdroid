#ifndef ESDROID_NEW_MR_DROID_H
#define ESDROID_NEW_MR_DROID_H

#include <string>

namespace esdroid {

// fixes newer mr files so the old 0112a parser takes them
// files that need parts this engine sim never had get skipped
class NewMRDroid {
public:
    enum Result {
        Pass,       // the file compiled as is
        Fixed,      // the file was rewritten and now compiles
        Rejected    // the file needs a newer engine sim
    };

    // what the caller learns about a finished translation
    struct ResultInfo {
        // the parsers own error lines from the last failed trial
        // shown in the unsupported dialog when the file is rejected
        std::string errorText;
        // trial compiles that ran one known file needs a single round
        int rounds = 0;
    };

    // path is the picked mr copy assetsBase is the es library root
    // on device that is the files dir plus assets
    // info is optional diagnostics for the caller
    static Result translate(const std::string &path,
            const std::string &assetsBase, ResultInfo *info = nullptr);
};

} // namespace esdroid

#endif
