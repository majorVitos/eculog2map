TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt
win32:LIBS += -lstdc++fs

SOURCES += \  
    ../config.cpp \
    ../file-cte.cpp \
    ../file-log.cpp \
    ../main.cpp

HEADERS += \
    ../config.h \
    ../cte_log_files.h
