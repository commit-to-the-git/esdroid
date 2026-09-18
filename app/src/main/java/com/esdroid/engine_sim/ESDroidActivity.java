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

package com.esdroid.engine_sim;

import android.app.NativeActivity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.InputStream;
import java.io.PrintWriter;

public class ESDroidActivity extends NativeActivity {
    private static final String TAG = "ESDroid";
    private static final int REQUEST_CODE_OPEN_MR = 42;

    static {
        System.loadLibrary("esdroid");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        File logDir = new File(getFilesDir().getParentFile(), "wtflogs");
        rotateJavaCrashLog(logDir);
        installCrashHandler(logDir);
        dumpLogcat(logDir);
        // Imports are per session, every launch starts on the default engine.
        deleteIfExists(new File(getFilesDir(), "assets/imported.mr"));
        deleteIfExists(new File(getFilesDir(), "assets/imported_main.mr"));
        deleteIfExists(new File(getFilesDir(), "imported.mr"));
    }

    private void deleteIfExists(File file) {
        if (file.exists() && file.delete()) {
            Log.i(TAG, "Deleted stale " + file.getName());
        }
    }

    private File logFile(File logDir, String name) {
        logDir.mkdirs();
        return new File(logDir, name);
    }

    // Keep one generation of the previous java crash log.
    private void rotateJavaCrashLog(File logDir) {
        try {
            File current = new File(logDir, "java_crash.log");
            if (current.exists()) {
                File previous = new File(logDir, "java_crash_last.log");
                previous.delete();
                current.renameTo(previous);
            }
        } catch (Exception e) {
            Log.w(TAG, "crash log rotate failed: " + e.getMessage());
        }
    }

    // Uncaught java exceptions leave no native signal; log them before the
    // default handler runs.
    private void installCrashHandler(final File logDir) {
        final Thread.UncaughtExceptionHandler previous = Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread thread, Throwable throwable) {
                try {
                    PrintWriter writer = new PrintWriter(new FileWriter(logFile(logDir, "java_crash.log"), true));
                    writer.println("=== Java uncaught exception, thread " + thread.getName() + " ===");
                    throwable.printStackTrace(writer);
                    writer.close();
                } catch (Exception ignored) {
                }
                if (previous != null) previous.uncaughtException(thread, throwable);
            }
        });
    }

    // Dump logcat at every launch; the previous process's last lines are
    // still in the buffer.
    private void dumpLogcat(final File logDir) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    File out = logFile(logDir, "last_logcat.txt");
                    Process process = Runtime.getRuntime().exec(new String[]{"logcat", "-d", "-v", "time"});
                    InputStream in = process.getInputStream();
                    FileOutputStream fos = new FileOutputStream(out);
                    byte[] buf = new byte[8192];
                    int n;
                    while ((n = in.read(buf)) > 0) fos.write(buf, 0, n);
                    fos.close();
                    in.close();
                    process.destroy();
                } catch (Exception e) {
                    Log.w(TAG, "logcat dump failed: " + e.getMessage());
                }
            }
        }).start();
    }

    // Plain file append into the native logger's file, no JNI, safe to call
    // when native is in a bad state.
    private void jlog(String message) {
        try {
            File logDir = new File(getFilesDir().getParentFile(), "wtflogs");
            logDir.mkdirs();
            PrintWriter writer = new PrintWriter(new FileWriter(new File(logDir, "wtfhappened.log"), true));
            writer.println("[" + SystemClock.uptimeMillis() + "] [java] " + message);
            writer.close();
        } catch (Throwable ignored) {
        }
        Log.i(TAG, message);
    }

    public void openFilePicker() {
        // MUST run on UI thread, native code runs on a different thread
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    intent.setType("*/*");
                    intent.putExtra(Intent.EXTRA_TITLE, "Select .mr engine file");
                    startActivityForResult(intent, REQUEST_CODE_OPEN_MR);
                    jlog("file picker started");
                } catch (Throwable e) {
                    Log.e(TAG, "Failed to start file picker: " + e.getMessage());
                    jlog("file picker failed to start: " + e.getMessage());
                }
            }
        });
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQUEST_CODE_OPEN_MR) return;
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            jlog("picker closed without a file, result code " + resultCode);
            return;
        }
        Uri uri = data.getData();
        Log.i(TAG, "File picked: " + uri.toString());
        try {
            InputStream in = getContentResolver().openInputStream(uri);
            if (in == null) {
                jlog("openInputStream returned null for " + uri);
                return;
            }
            // The copy lands in the extracted assets dir so the compiler
            // resolves it through the default search paths.
            File assetsDir = new File(getFilesDir(), "assets");
            assetsDir.mkdirs();
            File outFile = new File(assetsDir, "imported.mr");
            FileOutputStream out = new FileOutputStream(outFile);
            byte[] buf = new byte[8192];
            long total = 0;
            int n;
            while ((n = in.read(buf)) > 0) {
                out.write(buf, 0, n);
                total += n;
            }
            out.close();
            in.close();
            Log.i(TAG, "Copied " + total + " bytes to " + outFile.getAbsolutePath());
            jlog("copied " + total + " bytes to " + outFile.getAbsolutePath());
            // Throwable, not Exception: an UnsatisfiedLinkError must not
            // kill the app here.
            try {
                nativeOnFilePicked(outFile.getAbsolutePath());
                jlog("nativeOnFilePicked returned");
            } catch (Throwable t) {
                Log.e(TAG, "nativeOnFilePicked failed", t);
                jlog("nativeOnFilePicked failed: " + t);
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to copy file: " + e.getMessage());
            jlog("copy failed: " + e.getMessage());
        }
    }

    public native void nativeOnFilePicked(String path);
}
