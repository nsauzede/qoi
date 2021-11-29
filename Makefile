
all: ./qoidec
	touch qoiconv.c ; make qoiconv && ./qoiconv thumb.png thumb.qoi > ref.log
	./qoidec thumb.qoi thumb_qoi.ppm > local.log
	diff -u ref.log local.log

./qoidec: qoidec.c qoidec.h
	gcc -o $@ -g -O0 $^ -Wall -Werror -Wcast-align
