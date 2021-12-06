PNG := thumb.png
QOI := thumb.qoi

 OPT:=-g -O0
# OPT:=-O2
#OPT := -std=gnu99 -O3

all: ./qoiconv ./qoidec

test: ./qoiconv ./qoidec
	./qoiconv $(PNG) $(QOI) > qoiconv.log
	./qoidec $(QOI) thumb_qoidec.ppm > qoidec.log
	ppmtobmp thumb_qoidec.ppm > thumb_qoidec.bmp
	bmptopnm thumb_qoidec.bmp > thumb_qoidec.pnm
	pngtopnm $(PNG) > thumb_qoi.pnm
	@#diff -u qoiconv.log qoidec.log
	./qoidec $(QOI) thumb_qoienc.qoi > qoienc.log
	./qoidec thumb_qoienc.qoi thumb_qoienc.ppm > qoienc_dec.log
	@# diff -u qoidec.log qoienc_dec.log |less
	diff -q thumb_qoidec.pnm thumb_qoi.pnm
	diff -q $(QOI) thumb_qoienc.qoi

bench: ./qoibench
	./qoibench 5 images/screenshots

bench2: ./qoibench2
	./qoibench2 5 images/screenshots
	# ./qoibench2 10 images
	# gdb -q -nx -ex r --args ./qoibench2 10 images
	# valgrind ./qoibench2 10 images

./qoibench: qoibench.c qoi.h
	gcc -o $@ $(OPT) $^ -lpng

./qoibench2: qoibench2.c qoi.h qoidec.h
	gcc -o $@ $(OPT) $^ -lpng

./qoidec: qoidec.c qoidec.h
	gcc -o $@ $(OPT) $^ -Wall -Werror -Wcast-align -DDEBUG

./qoiconv: qoiconv.c qoi.h
	gcc -o $@ $(OPT) $^ -Wall -Werror -Wcast-align -DDEBUG

mrproper:
	$(RM) qoibench2 qoibench qoidec qoiconv
