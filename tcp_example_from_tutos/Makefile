

PORT=$(shell awk 'BEGIN{srand();printf("%d", 100*rand()+5900)}')
cstest:
	gcc server.c -o server
	gcc client.c -o client
	./server $(PORT) &
	echo "my message" | ./client 127.0.0.1 $(PORT)
