lisp:		main.o io.o gc.o eval.o func.o
			gcc -o lisp main.o io.o gc.o eval.o func.o
main.o:		MAIN.c
			gcc -c MAIN.c
io.o:		IO.c
			gcc -c IO.c
gc.o:		GC.c
			gcc -c GC.c
eval.o:		EVAL.c
			gcc -c EVAL.c
func.o:		FUNC.c
			gcc -c FUNC.c
clean:;		rm -f *.o
