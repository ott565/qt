load(qttest_p4)
SOURCES += tst_qzip.cpp

wince*|symbian: {
   addFiles.sources = testdata
   addFiles.path = .
   DEPLOYMENT += addFiles
   !symbian:DEFINES += SRCDIR=\\\".\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD\\\"
}
