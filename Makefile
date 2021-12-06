PNG := thumb.png
QOI := thumb.qoi

 OPT:=-g -O0
# OPT:=-O2
#OPT := -std=gnu99 -O3

all: ./qoiconv ./qoidneuf

test: ./qoiconv ./qoidneuf
	./qoiconv $(PNG) $(QOI) > qoiconv.log
	./qoidneuf $(QOI) thumb_d.pam > qoidec.log
	convert $(PNG) thumb.pam
	@#diff -u qoiconv.log qoidec.log
	./qoidneuf thumb.pam thumb_e.qoi > qoienc.log
	./qoidneuf thumb_e.qoi thumb_ed.pam > qoienc_dec.log
	@# diff -u qoidec.log qoienc_dec.log |less
	diff -q thumb_d.pam thumb.pam
	diff -q $(QOI) thumb_e.qoi
	diff -q thumb_ed.pam thumb.pam

bench: ./qoibench
	./qoibench 5 images/screenshots

bench2: ./qoibench2
	./qoibench2 5 images/screenshots
	# ./qoibench2 10 images
	# gdb -q -nx -ex r --args ./qoibench2 10 images
	# valgrind ./qoibench2 10 images

./qoibench: qoibench.c qoi.h
	gcc -o $@ $(OPT) $^ -lpng

./qoibench2: qoibench2.c qoi.h qoidneuf.h
	gcc -o $@ $(OPT) $^ -lpng

./qoidneuf: qoidneuf.c qoidneuf.h
	gcc -o $@ $(OPT) $^ -Wall -Werror -Wcast-align -DDEBUG

./qoiconv: qoiconv.c qoi.h
	gcc -o $@ $(OPT) $^ -Wall -Werror -Wcast-align -DDEBUG

mrproper:
	$(RM) qoibench2 qoibench qoidneuf qoiconv
