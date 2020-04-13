LIB=ws-webui.so

all: main.o commands.o
	gcc -o $(LIB) $^ -lwebsockets -shared -fPIC

%.o: %.c
	gcc -c $< -o $@ -fPIC

clean:
	rm -f *.o
	rm -f $(LIB)
