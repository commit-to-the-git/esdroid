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

import android.app.Dialog;
import android.app.NativeActivity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemClock;
import android.text.InputType;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.InputStream;
import java.io.PrintWriter;

public class ESDroidActivity extends NativeActivity {
    private static final String TAG = "ESDroid";
    private static final int REQUEST_CODE_OPEN_MR = 42;

    private Dialog mValueDialog = null;
    private Typeface mSilk = null;
    private Typeface mSilkBold = null;
    // which import the picker result belongs to
    private String mPickerKind = "engine";

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
        // imports are per session every launch starts on the default engine
        deleteIfExists(new File(getFilesDir(), "assets/imported.mr"));
        deleteIfExists(new File(getFilesDir(), "assets/imported_main.mr"));
        deleteIfExists(new File(getFilesDir(), "assets/imported_theme.mr"));
        deleteIfExists(new File(getFilesDir(), "imported.mr"));
        getWindow().setBackgroundDrawable(new ColorDrawable(Color.BLACK));
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

    // keep one generation of the previous java crash log
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

    // uncaught java exceptions leave no native signal so log them here
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

    // dump logcat at every launch the previous process lines are still
    // in the buffer
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

    // plain file append into the native log file no jni
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
        startMrPicker("engine", "Select .mr engine file");
    }

    public void openThemePicker() {
        startMrPicker("theme", "Select .mr theme file");
    }

    private void startMrPicker(final String kind, final String title) {
        // must run on ui thread native code runs elsewhere
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    mPickerKind = kind;
                    Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    intent.setType("*/*");
                    intent.putExtra(Intent.EXTRA_TITLE, title);
                    startActivityForResult(intent, REQUEST_CODE_OPEN_MR);
                    jlog("file picker started (" + kind + ")");
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
            // the copy lands in the assets dir so the compiler finds it
            File assetsDir = new File(getFilesDir(), "assets");
            assetsDir.mkdirs();
            boolean theme = "theme".equals(mPickerKind);
            File outFile = new File(assetsDir, theme ? "imported_theme.mr" : "imported.mr");
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
            // throwable not exception an unsatisfiedlinkerror must not
            // kill the app
            try {
                if (theme) {
                    nativeOnThemePicked(outFile.getAbsolutePath());
                } else {
                    nativeOnFilePicked(outFile.getAbsolutePath());
                }
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

    @Override
    protected void onPause() {
        super.onPause();
        // a paused dialog cannot be dismissed later
        dismissValueDialog();
    }

    // the same silkscreen the native ui bakes
    private Typeface silkFont(boolean bold) {
        try {
            if (bold) {
                if (mSilkBold == null) {
                    mSilkBold = Typeface.createFromAsset(getAssets(),
                            "delta-engine-assets/fonts/Silkscreen/slkscrb.ttf");
                }
                return mSilkBold == null ? Typeface.MONOSPACE : mSilkBold;
            }
            if (mSilk == null) {
                mSilk = Typeface.createFromAsset(getAssets(),
                        "delta-engine-assets/fonts/Silkscreen/slkscr.ttf");
            }
            return mSilk == null ? Typeface.MONOSPACE : mSilk;
        } catch (Throwable t) {
            return Typeface.MONOSPACE;
        }
    }

    // value entry for the settings panel a real dialog owns its own
    // window so its buttons and the ime work over the native input queue
    public void showValueInput(final int index, final String label, final String current) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    dismissValueDialog();
                    final Context ctx = ESDroidActivity.this;
                    final float dp = getResources().getDisplayMetrics().density;

                    LinearLayout panel = new LinearLayout(ctx);
                    panel.setOrientation(LinearLayout.VERTICAL);
                    panel.setBackgroundColor(0xFFFFFFFF);
                    int pad = (int) (14 * dp);
                    panel.setPadding(pad, pad, pad, pad);

                    TextView title = new TextView(ctx);
                    title.setText(label);
                    title.setTextColor(0xFF000000);
                    title.setTypeface(silkFont(true));
                    title.setTextSize(20);
                    panel.addView(title);

                    final EditText input = new EditText(ctx);
                    input.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
                    input.setText(current);
                    input.setTextColor(0xFF000000);
                    input.setTypeface(silkFont(false));
                    input.setTextSize(18);
                    input.setImeOptions(EditorInfo.IME_ACTION_DONE);
                    GradientDrawable field = new GradientDrawable();
                    field.setColor(0xFFFFFFFF);
                    field.setStroke((int) (2 * dp), 0xFF000000);
                    input.setBackground(field);
                    input.setSelection(input.getText().length());
                    panel.addView(input, new LinearLayout.LayoutParams(
                            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

                    LinearLayout row = new LinearLayout(ctx);
                    row.setOrientation(LinearLayout.HORIZONTAL);
                    int top = (int) (12 * dp);
                    LinearLayout.LayoutParams rowParams = new LinearLayout.LayoutParams(
                            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
                    rowParams.topMargin = top;

                    Button ok = new Button(ctx);
                    ok.setText("OK");
                    Button cancel = new Button(ctx);
                    cancel.setText("CANCEL");
                    for (Button b : new Button[] { ok, cancel }) {
                        b.setTextColor(0xFF000000);
                        b.setTypeface(silkFont(true));
                        b.setTextSize(16);
                        GradientDrawable bg = new GradientDrawable();
                        bg.setColor(0xFFFFFFFF);
                        bg.setStroke((int) (2 * dp), 0xFF000000);
                        b.setBackground(bg);
                        LinearLayout.LayoutParams bp = new LinearLayout.LayoutParams(
                                0, ViewGroup.LayoutParams.WRAP_CONTENT);
                        bp.weight = 1;
                        bp.leftMargin = top / 2;
                        bp.rightMargin = top / 2;
                        row.addView(b, bp);
                    }
                    panel.addView(row, rowParams);

                    final Dialog dialog = new Dialog(ctx);
                    dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
                    dialog.setContentView(panel);
                    dialog.setCancelable(true);
                    dialog.getWindow().setSoftInputMode(
                            WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE
                            | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
                    dialog.getWindow().setBackgroundDrawable(new GradientDrawable());

                    ok.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            dialog.dismiss();
                            commitValue(index, input.getText().toString());
                        }
                    });
                    cancel.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            dialog.dismiss();
                        }
                    });
                    input.setOnEditorActionListener(new TextView.OnEditorActionListener() {
                        @Override
                        public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                            if (actionId == EditorInfo.IME_ACTION_DONE) {
                                dialog.dismiss();
                                commitValue(index, v.getText().toString());
                                return true;
                            }
                            return false;
                        }
                    });

                    dialog.show();
                    dialog.getWindow().setLayout((int) (320 * dp),
                            ViewGroup.LayoutParams.WRAP_CONTENT);
                    input.requestFocus();
                    InputMethodManager imm =
                            (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
                    if (imm != null) imm.showSoftInput(input, InputMethodManager.SHOW_IMPLICIT);
                    mValueDialog = dialog;
                } catch (Throwable t) {
                    Log.e(TAG, "value dialog failed", t);
                    jlog("value dialog failed: " + t);
                }
            }
        });
    }

    public void hideValueInput() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                dismissValueDialog();
            }
        });
    }

    private void dismissValueDialog() {
        if (mValueDialog != null) {
            try {
                if (mValueDialog.isShowing()) mValueDialog.dismiss();
            } catch (Throwable ignored) {
            }
            mValueDialog = null;
        }
    }

    private void commitValue(int index, String text) {
        if (text == null) return;
        String trimmed = text.trim();
        if (trimmed.isEmpty()) {
            jlog("value entry empty, ignored");
            return;
        }
        double value = parseNumericPrefix(trimmed);
        if (Double.isNaN(value)) {
            jlog("value entry not a number: " + trimmed);
            return;
        }
        try {
            nativeOnValueInput(index, value);
        } catch (Throwable t) {
            Log.e(TAG, "nativeOnValueInput failed", t);
            jlog("nativeOnValueInput failed: " + t);
        }
    }

    // 2000 2000 hz and 60% all commit take the leading numeric run and
    // ignore the rest returns nan when there is no number
    private static double parseNumericPrefix(String s) {
        int end = 0;
        if (end < s.length() && (s.charAt(end) == '-' || s.charAt(end) == '+')) end++;
        int digits = 0;
        boolean dot = false;
        while (end < s.length()) {
            char c = s.charAt(end);
            if (c >= '0' && c <= '9') { end++; digits++; }
            else if (c == '.' && !dot) { end++; dot = true; }
            else break;
        }
        if (digits == 0) return Double.NaN;
        try {
            return Double.parseDouble(s.substring(0, end));
        } catch (NumberFormatException e) {
            return Double.NaN;
        }
    }

    public native void nativeOnFilePicked(String path);

    public native void nativeOnThemePicked(String path);

    public native void nativeOnValueInput(int index, double value);
}
