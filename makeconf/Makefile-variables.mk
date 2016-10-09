# NOCDDL
#
CND_BASEDIR=`pwd`
CND_BUILDDIR=build
CND_DISTDIR=dist
# Debug configuration
CND_PLATFORM_Debug=GNU-Linux
CND_ARTIFACT_DIR_Debug=dist/Debug/GNU-Linux
CND_ARTIFACT_NAME_Debug=chat
CND_ARTIFACT_PATH_Debug=dist/Debug/GNU-Linux/chat
CND_PACKAGE_DIR_Debug=dist/Debug/GNU-Linux/package
CND_PACKAGE_NAME_Debug=chat.tar
CND_PACKAGE_PATH_Debug=dist/Debug/GNU-Linux/package/chat.tar
# Release configuration
CND_PLATFORM_Release=GNU-Linux
CND_ARTIFACT_DIR_Release=dist/Release/GNU-Linux
CND_ARTIFACT_NAME_Release=chat
CND_ARTIFACT_PATH_Release=dist/Release/GNU-Linux/chat
CND_PACKAGE_DIR_Release=dist/Release/GNU-Linux/package
CND_PACKAGE_NAME_Release=chat.tar
CND_PACKAGE_PATH_Release=dist/Release/GNU-Linux/package/chat.tar
#
# include compiler specific variables
#
# dmake command
ROOT:sh = test -f makeconf/private/Makefile-variables.mk || \
	(mkdir -p makeconf/private && touch makeconf/private/Makefile-variables.mk)
#
# gmake command
.PHONY: $(shell test -f makeconf/private/Makefile-variables.mk || (mkdir -p makeconf/private && touch makeconf/private/Makefile-variables.mk))
#
