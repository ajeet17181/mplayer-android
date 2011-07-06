/android/android-ndk-r5b/toolchains/arm-eabi-4.4.0/prebuilt/linux-x86/bin/arm-eabi-gcc test_omx.c -I /android/android-ndk-r5b/platforms/android-8/arch-arm/usr/include/ -nostdlib -ldl -L /android/android-ndk-r5b/platforms/android-8/arch-arm/usr/lib/ -lc -llog  -o libtest_omx.so -I. $1
cp -f  libtest_omx.so /root/workspace/test_omx/libs/armeabi/libtest_omx.so
