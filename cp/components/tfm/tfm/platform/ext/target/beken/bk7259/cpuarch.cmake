# cpuarch.cmake is used to set things that related to the platform that are both
# immutable and global, which is to say they should apply to any kind of project
# that uses this platform. In practise this is normally compiler definitions and
# variables related to hardware.

# Set architecture and CPU
# BK7259 CP core is Cortex-M52 (Armv8.1-M Main).
set(TFM_SYSTEM_PROCESSOR cortex-m52)
set(TFM_SYSTEM_ARCHITECTURE armv8.1-m.main)
set(CRYPTO_HW_ACCELERATOR_TYPE beken)
add_compile_definitions(CONFIG_SOC_BK7259)
