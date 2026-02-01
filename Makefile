## Haiku Generic Makefile v2.6 ##

NAME = HaikuChat
TYPE = APP
APP_MIME_SIG = application/x-vnd.HaikuChat

SRCS = \
	src/App.cpp \
	src/MainWindow.cpp \
	src/ChatView.cpp \
	src/MessageBubble.cpp \
	src/InputView.cpp \
	src/SettingsWindow.cpp \
	src/Settings.cpp \
	src/LLMClient.cpp \
	src/ChatMessage.cpp \
	src/ChatSession.cpp \
	src/SidebarView.cpp \
	src/Log.cpp \
	src/Theme.cpp

RDEFS = resources/chat.rdef

LIBS = be bnetapi netservices shared tracker localestub stdc++
LIBPATHS =
SYSTEM_INCLUDE_PATHS = /boot/system/develop/headers/private/netservices
LOCAL_INCLUDE_PATHS = src
OPTIMIZE := FULL
LOCALES =
DEFINES =
WARNINGS = ALL
SYMBOLS := FALSE
DEBUGGER := FALSE
COMPILER_FLAGS =
LINKER_FLAGS = -lstdc++

DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
