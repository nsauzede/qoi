PNG := thumb.png
QOI := thumb.qoi

# OPT:=-g -O0
# OPT:=-O2
OPT := -std=gnu99 -O3

all: ./qoidec
	touch qoiconv.c ; make qoiconv && ./qoiconv $(PNG) $(QOI) > ref.log
	-./qoidec $(QOI) thumb_qoi.ppm > local.log
	diff -u ref.log local.log

bench: ./qoibench
	./qoibench 5 images/screenshots
	# ./qoibench 10 images
	# gdb -q -nx -ex r --args ./qoibench 10 images
	# valgrind ./qoibench 10 images

./qoibench: qoibench.c qoi.h qoidec.h
	gcc -o $@ $(OPT) $^ -lpng

./qoidec: qoidec.c qoidec.h
	gcc -o $@ $(OPT) $^ -Wall -Werror -Wcast-align -DDEBUG

./qoiconv: qoiconv.c qoi.h
	gcc -o $@ $(OPT) $^ -Wall -Werror -Wcast-align -DDEBUG

mrproper:
	$(RM) qoibench qoidec qoiconv
