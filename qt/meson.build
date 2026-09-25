add_languages('cpp', native: false)

qt6 = import('qt6')
qt6_core = dependency('qt6', modules: ['Core'])
qt6_widgets = dependency('qt6', modules: ['Core', 'Widgets'])

qt_incdirs = [root_include, include_directories('../src')]

# Qt's signals/slots/emit macros collide with GLib/GIO struct fields
# (e.g. GDBusInterfaceInfo::signals). Use the Q_* spellings instead.
qt_cpp_args = [
    '-DQT_NO_KEYWORDS',
    '-DQTHASH_VERSION="@0@"'.format(meson.project_version()),
]

# Shared, toolkit-agnostic glue: the hashcore seam and the GLib<->Qt event-loop
# pump.
qthash_glue = ['hashcore.c', 'glib_qt.cpp']

moc_glue = qt6.preprocess(
    moc_headers: ['glib_qt.hpp'],
    include_directories: qt_incdirs,
    dependencies: qt6_core,
)

moc_gui = qt6.preprocess(
    moc_headers: ['mainwindow.hpp'],
    include_directories: qt_incdirs,
    dependencies: qt6_widgets,
)

# The Qt6 app.
qthash = executable(
    'qthash',
    'main.cpp',
    'mainwindow.cpp',
    'checkfile.cpp',
    qthash_glue,
    moc_glue,
    moc_gui,
    include_directories: qt_incdirs,
    cpp_args: qt_cpp_args,
    dependencies: [glib, gio, qt6_widgets],
    link_with: [libhash, libutil],
    install: true,
)

# Headless proof driver from phases 0-2 (QtCore only). Kept as a smoke test.
qthash_smoke = executable(
    'qthash-smoke',
    'smoke.cpp',
    qthash_glue,
    moc_glue,
    include_directories: qt_incdirs,
    cpp_args: qt_cpp_args,
    dependencies: [glib, gio, qt6_core],
    link_with: [libhash, libutil],
)

# Unit tests for the units the Qt port adds (hashcore seam + checksum parser).
qthash_test = executable(
    'qthash-test',
    'qthash-test.cpp',
    'hashcore.c',
    'checkfile.cpp',
    include_directories: qt_incdirs,
    cpp_args: qt_cpp_args,
    dependencies: [glib, gio, qt6_core],
    link_with: [libhash, libutil],
)
test('qthash-test', qthash_test)
