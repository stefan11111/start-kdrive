.POSIX:

all:
	${CC} ${CFLAGS} ${LDFLAGS} start-kdrive.c -o start-kdrive

install:
	mkdir -p ${DESTDIR}/usr/bin
	cp start-kdrive ${DESTDIR}/usr/bin/start-kdrive

clean:
	rm start-kdrive

uninstall:
	rm ${DESTDIR}/usr/bin/start-kdrive

.PHONY: all clean install uninstall
