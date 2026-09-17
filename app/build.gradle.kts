plugins { id("com.android.application") }
android {
    namespace = "com.esdroid.engine_sim"
    compileSdk = 34
    ndkVersion = "26.3.11579264"
    defaultConfig {
        applicationId = "com.esdroid.engine_sim"
        minSdk = 24; targetSdk = 34; versionCode = 1; versionName = "0.1.12a-mobile"
        ndk { abiFilters += listOf("arm64-v8a", "armeabi-v7a") }
        externalNativeBuild {
            cmake {
                cppFlags += listOf("-std=c++17", "-fexceptions", "-frtti")
                arguments += listOf("-DANDROID_STL=c++_shared")
            }
        }
    }
    externalNativeBuild {
        cmake { path = file("src/main/cpp/CMakeLists.txt"); version = "3.22.1" }
    }
    buildTypes {
        release { isMinifyEnabled = false }
        debug { isMinifyEnabled = false; isDebuggable = true; isJniDebuggable = true }
    }
    compileOptions { sourceCompatibility = JavaVersion.VERSION_17; targetCompatibility = JavaVersion.VERSION_17 }
    packaging { jniLibs { useLegacyPackaging = true } }
}
dependencies {}
