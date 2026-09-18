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

#include "engine_sim_application.h"
#include "android_backend.h"
#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cstdarg>
#include <signal.h>
#include <exception>
#include <pthread.h>
#include <dirent.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ESDroid", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "ESDroid", __VA_ARGS__))

extern "C" void esdroid_pre_main(android_app*);

static std::string g_logDir;
static int g_logFd = -1;

// One append mode fd, one write() per line. Safe from any thread and
// from the signal handler: no stdio locks, no heap.
static void log_write(const char* text, size_t len) {
    if (g_logFd >= 0) {
        ssize_t ignored = write(g_logFd, text, len);
        (void)ignored;
    }
}

static void wtflog(const char* fmt, ...) {
    char body[1100];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(body, sizeof(body), fmt, args);
    va_end(args);
    if (n < 0) n = 0;
    if ((size_t)n >= sizeof(body)) n = (int)sizeof(body) - 1;

    auto now = std::chrono::steady_clock::now();
    long long ms = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    char line[1200];
    int len = snprintf(line, sizeof(line), "[%lld] %.*s\n", ms, n, body);
    if (len > 0) log_write(line, (size_t)len);
    __android_log_print(ANDROID_LOG_INFO, "ESDroid", "%.*s", n, body);
}

extern "C" void esdroid_poll_events() {
    android_app* app = esdroid::AndroidBackend::getAndroidApp();
    if (!app) return;
    int events; android_poll_source* source;
    while (ALooper_pollAll(0, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0) {
        if (source) source->process(app, source);
        else break;
    }
}

extern "C" void esdroid_wtflog(const char* fmt, ...) {
    char body[1100];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(body, sizeof(body), fmt, args);
    va_end(args);
    if (n < 0) n = 0;
    if ((size_t)n >= sizeof(body)) n = (int)sizeof(body) - 1;

    char line[1150];
    int len = snprintf(line, sizeof(line), "%.*s\n", n, body);
    if (len > 0) log_write(line, (size_t)len);
    __android_log_print(ANDROID_LOG_INFO, "ESDroid", "%.*s", n, body);
}

static void initCrashLogger() {
    g_logDir = "/data/data/com.esdroid.engine_sim/wtflogs";
    mkdir("/data/data/com.esdroid.engine_sim", 0755);
    mkdir(g_logDir.c_str(), 0755);
    // Fresh log every run. piranha_errors.log is rewritten by every script
    // compile; java_crash.log and last_logcat.txt are managed on the Java
    // side and left alone.
    std::string path = g_logDir + "/wtfhappened.log";
    remove(path.c_str());
    std::string errPath = g_logDir + "/piranha_errors.log";
    remove(errPath.c_str());
    g_logFd = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (g_logFd >= 0) {
        const char* header = "=== ESDroid Crash Log ===\n";
        log_write(header, strlen(header));
    }
}

// Writes only through write() so it stays usable from a signal handler.
static void crash_handler(int sig, siginfo_t* info, void*) {
    const char* signame = "UNKNOWN";
    switch (sig) {
        case SIGSEGV: signame = "SIGSEGV"; break;
        case SIGABRT: signame = "SIGABRT"; break;
        case SIGFPE:  signame = "SIGFPE"; break;
        case SIGILL:  signame = "SIGILL"; break;
        case SIGBUS:  signame = "SIGBUS"; break;
        case SIGTRAP: signame = "SIGTRAP"; break;
    }
    char buf[256];
    int len = snprintf(buf, sizeof(buf),
        "\n*** CRASH: signal %d (%s), fault address %p, thread %d ***\n",
        sig, signame,
        (info != nullptr) ? info->si_addr : nullptr,
        (int)gettid());
    if (len > 0) log_write(buf, (size_t)len);
    signal(sig, SIG_DFL);
    raise(sig);
}

static void terminate_handler() {
    std::exception_ptr p = std::current_exception();
    char buf[128];
    int len = snprintf(buf, sizeof(buf),
        "\n*** std::terminate() called, thread %d ***\n", (int)gettid());
    if (len > 0) log_write(buf, (size_t)len);
    if (p) {
        try { std::rethrow_exception(p); }
        catch (const std::exception& e) {
            len = snprintf(buf, sizeof(buf), "Uncaught exception: %s\n", e.what());
            if (len > 0) log_write(buf, (size_t)len);
        }
        catch (...) {
            log_write("Uncaught unknown exception\n", 27);
        }
    } else {
        log_write("No current exception\n", 21);
    }
    abort();
}

// No crash line and no exit line means an external kill (SIGKILL, ANR,
// low memory); nothing in process can record that.
static void atexit_handler() {
    const char* msg = "=== process exited normally (atexit) ===\n";
    log_write(msg, strlen(msg));
}

static void installCrashHandlers() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crash_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGTRAP, &sa, nullptr);
    std::set_terminate(terminate_handler);
    atexit(atexit_handler);
}

static bool copyAsset(AAssetManager* mgr, const char* assetPath, const std::string& destPath) {
    AAsset* asset = AAssetManager_open(mgr, assetPath, AASSET_MODE_STREAMING);
    if (!asset) return false;
    size_t slash = destPath.rfind('/');
    if (slash != std::string::npos) {
        std::string dir = destPath.substr(0, slash);
        for (size_t i = 1; i <= dir.size(); ++i) {
            if (i == dir.size() || dir[i] == '/') { mkdir(dir.substr(0, i).c_str(), 0755); }
        }
    }
    FILE* fp = fopen(destPath.c_str(), "wb");
    if (!fp) { AAsset_close(asset); return false; }
    char buf[8192]; int n;
    while ((n = AAsset_read(asset, buf, sizeof(buf))) > 0) fwrite(buf, 1, n, fp);
    fclose(fp); AAsset_close(asset); return true;
}

static void copyAllAssets(AAssetManager* mgr, const std::string& assetsDir, const std::string& destBase) {
    AAssetDir* dir = AAssetManager_openDir(mgr, assetsDir.c_str());
    if (!dir) return;
    const char* name;
    while ((name = AAssetDir_getNextFileName(dir)) != nullptr) {
        std::string assetPath = assetsDir.empty() ? name : (assetsDir + "/" + name);
        copyAsset(mgr, assetPath.c_str(), destBase + "/" + name);
    }
    AAssetDir_close(dir);
}

static void extractAssets(android_app* app, const std::string& destBase) {
    AAssetManager* mgr = app->activity->assetManager;
    if (!mgr) return;
    wtflog("Extracting assets to: %s", destBase.c_str());
    copyAllAssets(mgr, "", destBase);
    const char* subdirs[] = {
        "engines", "engines/atg-video-1", "engines/atg-video-2",
        "engines/audi", "engines/bmw", "engines/chevrolet", "engines/kohler",
        "sound-library", "sound-library/smooth", "sound-library/archive", "themes", "part-library",
        "delta-engine-assets", "delta-engine-assets/shaders",
        "delta-engine-assets/shaders/glsl", "delta-engine-assets/fonts",
        "delta-engine-assets/fonts/Silkscreen",
        "delta-engine-assets/fonts/roboto",
        "delta-engine-assets/fonts/josefin-sans",
        "es", "es/actions", "es/constants", "es/infrastructure",
        "es/objects", "es/part-library", "es/part-library/parts",
        "es/settings", "es/sound-library", "es/sound-library/archive",
        "es/sound-library/new", "es/sound-library/sharp", "es/sound-library/smooth",
        "es/types", "es/utilities",
        nullptr
    };
    for (int i = 0; subdirs[i]; ++i)
        copyAllAssets(mgr, subdirs[i], destBase + "/" + subdirs[i]);
    wtflog("Asset extraction complete");
}

static void waitForWindow(android_app* app) {
    wtflog("Waiting for window...");
    int tc = 0;
    while (app->window == nullptr) {
        int events; android_poll_source* source;
        ALooper_pollAll(0, nullptr, &events, reinterpret_cast<void**>(&source));
        if (source) source->process(app, source);
        if (app->destroyRequested != 0) { wtflog("Destroy requested"); return; }
        usleep(10000); tc++;
        if ((tc % 100) == 0) wtflog("Still waiting for window (%d ms)", tc * 10);
        if (tc > 1000) { wtflog("Window timeout"); return; }
    }
    wtflog("Window ready: %p", app->window);
}

// Poll Android events to prevent ANR during long operations
static void pollEvents() {
    android_app* app = esdroid::AndroidBackend::getAndroidApp();
    if (!app) return;
    int events; android_poll_source* source;
    while (ALooper_pollAll(0, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0) {
        if (source) source->process(app, source);
        else break;
    }
}

static EngineSimApplication* createApplication(android_app* app) {
    pollEvents();
    EngineSimApplication* application = new EngineSimApplication();
    try {
        application->initialize(reinterpret_cast<void*>(app), ysContextObject::DeviceAPI::OpenGL4_0);
        // initialize() returns early on core failures (e.g. CreateGameWindow).
        // Such an app has no window or device and destroy() skips its
        // teardown; return null so the caller never touches it.
        if (!application->isCoreReady()) {
            wtflog("  initialize() aborted, core not ready, engine present: %s",
                application->hasEngine() ? "yes" : "no");
            delete application;
            return nullptr;
        }
        wtflog("  initialize() OK, engine present: %s", application->hasEngine() ? "yes" : "no");
        return application;
    } catch (const std::exception& e) { wtflog("EXCEPTION in initialize(): %s", e.what()); }
    catch (...) { wtflog("UNKNOWN EXCEPTION in initialize()"); }
    try { application->destroy(); } catch (...) {}
    delete application;
    return nullptr;
}

extern "C" void android_main(struct android_app* app) {
    initCrashLogger();
    installCrashHandlers();
    wtflog("=== android_main START ===");

    // The native_app_glue thread has a 16MB stack (see
    // android_native_app_glue.c); the looper needs this thread for
    // window/input events.

    wtflog("Step 1: esdroid_pre_main");
    esdroid_pre_main(app);
    wtflog("Step 2: setAndroidApp");
    esdroid::AndroidBackend::setAndroidApp(app);
    wtflog("Step 3: setAssetManager + setFilesDir");
    if (app->activity) {
        esdroid::AndroidBackend::setAssetManager(app->activity->assetManager);
        if (app->activity->internalDataPath) {
            esdroid::AndroidBackend::instance().setFilesDir(app->activity->internalDataPath);
        }
    }

    wtflog("Step 4: waitForWindow");
    waitForWindow(app);
    if (app->window == nullptr) { wtflog("FATAL: No window"); if(g_logFd>=0){close(g_logFd);g_logFd=-1;} return; }

    wtflog("Step 5: Extract assets");
    std::string filesDir = esdroid::AndroidBackend::instance().filesDir();
    if (filesDir.empty()) { filesDir = "/data/data/com.esdroid.engine_sim/files"; esdroid::AndroidBackend::instance().setFilesDir(filesDir); }
    std::string assetsDest = filesDir + "/assets";
    // Extract only when the assets dir is missing or empty, otherwise every
    // launch would recompile the scripts.
    bool needExtract = true;
    DIR *dir = opendir(assetsDest.c_str());
    if (dir) {
        struct dirent *ent;
        int fileCount = 0;
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_name[0] != '.') fileCount++;
        }
        closedir(dir);
        if (fileCount > 0) needExtract = false;
    }
    if (needExtract) {
        mkdir(assetsDest.c_str(), 0755);
        if (app->activity && app->activity->assetManager) {
            wtflog("Extracting assets (first launch or cache cleared)...");
            extractAssets(app, assetsDest);
        }
    } else {
        wtflog("Assets already extracted, skipping");
    }

    wtflog("Step 6: Init EGL");
    if (!esdroid::AndroidBackend::instance().isWindowReady()) esdroid::AndroidBackend::instance().initWindow(0, 0);
    wtflog("Step 7: Init audio");
    esdroid::AndroidBackend::instance().initAudio(44100, 1);

    // Imports are per session, remove any leftover from a previous run.
    {
        std::string staleFiles[] = {
            filesDir + "/assets/imported.mr",
            filesDir + "/assets/imported_main.mr",
            filesDir + "/imported.mr"
        };
        for (const std::string& stale : staleFiles)
            if (remove(stale.c_str()) == 0) wtflog("Removed imported engine from a previous session: %s", stale.c_str());
    }

    bool restart = false;
    do {
        wtflog("Step 8: Create application");
        EngineSimApplication* application = createApplication(app);
        if (application == nullptr) break;

        // An imported engine that failed to compile falls back to the default one
        if (!application->hasEngine() &&
                esdroid::AndroidBackend::instance().activeMrPath() != "assets/main.mr") {
            wtflog("Imported engine failed to load, falling back to the default engine");
            try { application->destroy(); } catch (...) {}
            delete application;
            esdroid::AndroidBackend::instance().resetToDefaultMr();
            esdroid::AndroidBackend::instance().resetAudioRing();
            pollEvents();
            application = createApplication(app);
            if (application == nullptr) break;
        }

        // Touch UI overlay, drawn on top of the engine.
        esdroid::AndroidBackend::instance().initTouchUI();
        wtflog("Touch UI initialized");

        wtflog("Step 9: run()");
        try {
            application->run();
            wtflog("  run() returned OK");
        } catch (const std::exception& e) { wtflog("EXCEPTION in run(): %s", e.what()); }
        catch (...) { wtflog("UNKNOWN EXCEPTION in run()"); }
        restart = application->restartRequested();
        wtflog("run() finished, restart requested: %s", restart ? "yes" : "no");

        if (restart && !esdroid::AndroidBackend::instance().shouldQuit()) {
            // Log the imported file size; an empty copy should be visible.
            {
                std::string candidates[] = {
                    filesDir + "/assets/imported.mr",
                    filesDir + "/imported.mr"
                };
                bool logged = false;
                for (const std::string& candidate : candidates) {
                    struct stat st;
                    if (stat(candidate.c_str(), &st) == 0) {
                        wtflog("Imported engine file: %s (%ld bytes)", candidate.c_str(), (long)st.st_size);
                        logged = true;
                        break;
                    }
                }
                if (!logged) wtflog("Imported engine file not found in either known location");
            }
            // The picker may still be closing; wait for the window before
            // rebuilding so teardown runs with a live GL context.
            wtflog("Restart requested, waiting for the window");
            waitForWindow(app);
            if (esdroid::AndroidBackend::instance().shouldQuit() || app->window == nullptr) {
                wtflog("Restart aborted, window lost or quit requested");
                restart = false;
            }
            else {
                if (!esdroid::AndroidBackend::instance().isWindowReady())
                    esdroid::AndroidBackend::instance().initWindow(0, 0);
                esdroid::AndroidBackend::instance().resetAudioRing();
                esdroid::AndroidBackend::instance().clearAllKeys();
                pollEvents();
            }
        }

        wtflog("Step 10: destroy()");
        try { application->destroy(); wtflog("  destroy() OK"); }
        catch (const std::exception& e) { wtflog("EXCEPTION in destroy(): %s", e.what()); }
        catch (...) { wtflog("UNKNOWN EXCEPTION in destroy()"); }
        delete application;

        // Teardown may take the EGL surface down with it; bring it back
        // before the next application builds against it.
        if (restart && !esdroid::AndroidBackend::instance().shouldQuit()
                && app->window != nullptr
                && !esdroid::AndroidBackend::instance().isWindowReady()) {
            esdroid::AndroidBackend::instance().initWindow(0, 0);
        }
    } while (restart && !esdroid::AndroidBackend::instance().shouldQuit());

    wtflog("=== android_main END ===");
    if (g_logFd >= 0) { close(g_logFd); g_logFd = -1; }
}
