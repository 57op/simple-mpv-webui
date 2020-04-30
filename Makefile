LIB=ws-webui.so

all: main.o commands.o
	gcc -o $(LIB) $^ -s -Os -lwebsockets -shared -fPIC

%.o: %.c
	gcc -c $< -o $@ -s -Os -std=c11 -fPIC -D_GNU_SOURCE -Wall -Wno-switch

clean:
	rm -f *.o
	rm -f $(LIB)
