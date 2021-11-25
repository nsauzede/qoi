

all:
	touch qoiconv.c ; make qoiconv && ./qoiconv thumb.png thumb.qoi
	gcc -o qoidec -g -O0 qoidec.c -Wall -Werror -Wcast-align && ./qoidec thumb.qoi thumb_qoi.ppm
