ifndef ECOS_SDK_HOME
$(error ECOS_SDK_HOME is not set; activate the SDK environment first)
endif

PYTHON ?= python3
ECOS := $(ECOS_SDK_HOME)/tools/ecos.py

all:
	@$(PYTHON) $(ECOS) --sdk $(ECOS_SDK_HOME) build --project $(CURDIR)

clean:
	@$(PYTHON) $(ECOS) --sdk $(ECOS_SDK_HOME) build --project $(CURDIR) --clean

.PHONY: all clean
