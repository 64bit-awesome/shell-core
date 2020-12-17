all:	sc

sc:	shell-core.c
	cc shell-core.c -o sc

clean:
	rm -f sc core *~