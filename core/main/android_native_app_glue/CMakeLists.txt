cmake_minimum_required(VERSION 3.11)
set(PROJECT_ANDROID_NATIVE_GLUE android_native_glue CACHE INTERNAL "")
set(LOCAL_PROJECT ${PROJECT_ANDROID_NATIVE_GLUE})
project(${LOCAL_PROJECT} VERSION 1.0.0 LANGUAGES C)

add_library(${LOCAL_PROJECT} STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
