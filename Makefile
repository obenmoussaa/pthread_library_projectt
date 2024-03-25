
main:
	rm main  && gcc main.c thread.c -o main
main_pthread:
	gcc -DUSE_PTHREAD main.c   -o main_pthread