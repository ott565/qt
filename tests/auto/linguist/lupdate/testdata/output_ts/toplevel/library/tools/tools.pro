TEMPLATE = app
LANGUAGE = C++

SOURCES += main.cpp

TRANSLATIONS        += translations/project.ts

exists( $$TRANSLATIONS ) {
    win32: system(del $$TRANSLATIONS)
    unix:  system(rm -f $$TRANSLATIONS)
}

