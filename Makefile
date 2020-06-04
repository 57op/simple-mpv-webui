LIBNAME = ws-webui
INCLUDE = $(addprefix -I,$(wildcard include/*))
LIBRARIES = $(addprefix -l:,$(notdir $(wildcard libs/*)))

ifeq ($(OS),Windows_NT)
	LIB = $(LIBNAME).dll
else
	LIB = $(LIBNAME).so
	CFLAGS = -D_GNU_SOURCE
endif

all: main.o commands.o
	gcc -Llibs -o $(LIB) $^ -s -Os $(LIBRARIES) -shared -fPIC

%.o: %.c
	gcc $(INCLUDE) $(CFLAGS) -c $< -o $@ -s -Os -std=c11 -fPIC -Wall -Wno-switch

clean:
	rm -f *.o
	rm -f $(LIB)
