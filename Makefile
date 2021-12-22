concat_strings: concat_strings.c
	gcc -Wall -g -o concat_strings concat_strings.c

run: concat_strings
	./concat_strings
