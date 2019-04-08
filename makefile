all: msg_client msg_server
	

msg_client: msg_client.c decl_client.h
	gcc -g -Wall -std=gnu99 -o msg_client -include decl_client.h msg_client.c -lpthread

msg_server: msg_server.c decl_server.h
	gcc -g -Wall -std=gnu99 -o msg_server -include decl_server.h msg_server.c -lpthread

clean:  
	rm msg_client msg_server