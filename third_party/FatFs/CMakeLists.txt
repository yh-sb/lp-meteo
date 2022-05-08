add_library(fatfs STATIC
    ff.c
    ffsystem.c
    ffunicode.c
)

target_include_directories(fatfs
    PUBLIC
        .
)
