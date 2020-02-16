CLANG = clang
CC = clang

%.ll: %.c
	$(CLANG) -Xclang -disable-O0-optnone -S -emit-llvm $<
	opt -stats -scalarrepl-kgk2 -S $@ -o $@

%.bc: %.ll
	llvm-as $< -o $@
    
%.s: %.bc
	llc --filetype=asm $< -o $@

%.exe: %.s
	$(CC) -g $< -o $@

clean:
	-rm -f *.ll *.exe *.bc .ll *.out *.s *.o
