CLANG = clang
CC = clang
TESTS := $(shell find $(SOURCEDIR) -name '*.c')


test: $(TESTS:%.c=%.run)
	@echo "*******************************************************************************"
	@echo "*                                All tests pass!                              *"
	@echo "*******************************************************************************"

%.ll: %.c
	$(CLANG) -w -Xclang -disable-O0-optnone -S -emit-llvm $<
	opt -stats -scalarrepl-kgk2 -S $@ -o $@

%.bc: %.ll
	llvm-as $< -o $@
    
%.s: %.bc
	llc --filetype=asm $< -o $@

%.exe: %.s
	$(CC) -g $< -o $@

%.run: %.exe
	./$<

clean:
	-rm -f *.ll *.exe *.bc .ll *.out *.s *.o
