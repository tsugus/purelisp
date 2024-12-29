lisp:		main.o io.o eval.o gc.o
			gcc -o lisp main.o io.o eval.o gc.o
main.o:		MAIN.c
			gcc -c MAIN.c
io.o:		IO.c
			gcc -c IO.c
eval.o:		EVAL.c
			gcc -c EVAL.c
gc.o:		GC.c
			gcc -c GC.c
clean:;		rm -f *.o
