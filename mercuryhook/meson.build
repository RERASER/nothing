shared_library(
    'mercuryhook',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    vs_module_defs : 'mercuryhook.def',
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
        capnhook.get_variable('hooklib_dep'),
    ],
    link_with : [
        aimeio_lib,
        board_lib,
        hooklib_lib,
        mercuryio_lib,
        platform_lib,
        util_lib,
    ],
    sources : [
        'config.c',
        'config.h',
        'dllmain.c',
        'io4.c',
        'io4.h',
        'mercury-dll.c',
        'mercury-dll.h',
        'elisabeth.h',
        'elisabeth.c'
    ],
)
