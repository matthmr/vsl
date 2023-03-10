all: libvsl.a

-include make/Flags.mk
-include make/Objects.mk
-include make/Libraries.mk
-include make/Targets.mk
-include make/Cgen.mk

MAKEFILES:=         \
	make/Flags.mk     \
	make/Objects.mk   \
	make/Libraries.mk \
	make/Targets.mk   \
	make/Cgen.mk      \
	make/Deps.mk

PRE_SRC:=lisp.c
PRE_BIN:=lisp

TARGETS += $(PRE_BIN)

$(PRE_BIN): prevsl $(PRE)
ifndef $(PRE)
	@echo "[ !! ] Missing \`PRE'. Did you set it in \`./configure'?"
	@exit 1
endif
	@echo "PREVSL " $(PRE)
	@./prevsl < $(PRE) > $(PRE_SRC) || exit 1
	@echo "CC lisp"
	@$(CC) -c $(CFLAGS) $(CFLAGSADD) -o $(PRE_BIN) -L. -lvsl $(PRE_SRC)

clean-obj:
	@echo "RM " $(OBJECTS)
	@rm -rfv $(OBJECTS)

clean-cgen:
	@echo "RM " $(CGEN_SRC) $(CGEN_BIN)
	@rm -rfv $(CGEN_SRC) $(CGEN_BIN)

clean-targets:
	@echo "RM " $(TARGETS) $(LIBRARIES) $(PRE_SRC)
	@rm -rfv $(TARGETS) $(LIBRARIES) $(PRE_SRC)

clean-make:
	@echo "RM " $(MAKEFILES)
	@rm -rfv $(MAKEFILES)

clean-tags:
	@echo "RM TAGS"
	@rm -rfv TAGS

clean-dev: clean-obj clean-cgen clean-targets
clean: clean-obj clean-cgen clean-tags clean-targets clean-make

tags:
	@echo "ETAGS TAGS"
	@find -type f -name '*.[ch]' | xargs etags -o TAGS

help:
	@echo -e "\
make: makes the current dependency of \`all' (libvsl.a)\n\
make lisp PRE=<vsl-file> makes the lisp, with \`PRE' as the pvsl file.\n\
                         \`PRE' can also be defined at \`./configure' time\n\
make clean-obj: cleans objects files\n\
make clean-tags: cleans tags\n\
make clean-targets: cleans targets\n\
make clean-cgen: cleans auto-generated C files and programs\n\
make clean-make: cleans makefiles\n\
make clean: cleans all"

.PHONY: clean clean-tags clean-obj clean-targets clean-make \
        tags help lisp $(PRE)
