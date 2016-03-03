all: virtualmem virtualmem_2 virtualmem_3

virtualmem: virtualmem.c 
	gcc -g -o virtualmem virtualmem.c -pthread

virtualmem_2: virtualmem_2.c 
	gcc -g -o virtualmem_2 virtualmem_2.c -pthread

virtualmem_3: virtualmem_3.c 
	gcc -g -o virtualmem_3 virtualmem_3.c -pthread

clean:
	rm -rf virtualmem virtualmem_2 virtualmem_3
