# Convenience wrapper around the meson/ninja build for the QtHash app.
# Everything here just drives meson; `meson`/`ninja` remain the source of truth.

BUILDDIR   ?= build
PREFIX     ?= /usr/local
# Extra meson options, e.g. optional hash libs:  make MESON_ARGS=-Dnettle=true
MESON_ARGS ?=
# Parallel jobs for compile and test; defaults to every available thread.
JOBS       ?= $(shell nproc)
APP        := $(BUILDDIR)/qt/qthash

.DEFAULT_GOAL := help

.PHONY: help build run test install uninstall clean

help: ## Show make targets
	@grep -hE '^[0-9a-zA-Z/_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

$(BUILDDIR)/build.ninja:
	meson setup $(BUILDDIR) --prefix $(PREFIX) $(MESON_ARGS)

build: $(BUILDDIR)/build.ninja ## Configure (if needed) and build the app
	meson compile -C $(BUILDDIR) -j $(JOBS)

run: build ## Build and run the app
	$(APP)

test: build ## Build and run the test suite
	meson test -C $(BUILDDIR) --num-processes $(JOBS)

install: build ## Install the app (honours PREFIX and DESTDIR)
	meson install -C $(BUILDDIR)

uninstall: ## Remove the files installed by `make install`
	ninja -C $(BUILDDIR) uninstall

clean: ## Remove the build directory
	rm -rf $(BUILDDIR)
