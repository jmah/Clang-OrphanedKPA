# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.

CLANG_LEVEL := ../..
LIBRARYNAME = OrphanedKeyPathsAffecting

# If we don't need RTTI or EH, there's no reason to export anything
# from the plugin.
ifneq ($(REQUIRES_RTTI), 1)
ifneq ($(REQUIRES_EH), 1)
EXPORTED_SYMBOL_FILE = $(PROJ_SRC_DIR)/OrphanedKeyPathsAffecting.exports
endif
endif

LINK_LIBS_IN_SHARED = 0
SHARED_LIBRARY = 1

include $(CLANG_LEVEL)/Makefile

ifeq ($(OS),Darwin)
  LDFLAGS=-Wl,-undefined,dynamic_lookup
endif
