OBJ = main.o
REQALL =  message.o my_util.o
REQCLI = request_client.o 
SRVFILE = server.o
WRKFILE = worker_client.o
all:	$(OBJ) $(REQCLI) $(SRVFILE) $(REQALL) $(WRKFILE)
		g++ -g -o main $(OBJ) $(REQALL) -lm -lpthread
		g++ -g -o request_client $(REQCLI) $(REQALL) -lm -lpthread         
		g++ -g -o server $(SRVFILE) $(REQALL) -lm -lpthread         
		g++ -g -o worker_client $(WRKFILE) $(REQALL) -lm -lpthread
main.o: main.c message.h
		g++ -g -c main.c
message.o: message.c message.h
		g++ -g -c message.c		
my_util.o: my_util.c my_util.h
		g++ -g -c my_util.c		 
request_client.o: request_client.c message.h my_util.h
		g++ -g -c request_client.c 
server.o: server.c message.h my_util.h
		g++ -g -c server.c 
worker_client.o:worker_client.c message.h my_util.h
		g++ -g -c worker_client.c
clean:
		rm main $(OBJ) request_client $(REQCLI) server $(SRVFILE) worker_client $(WRKFILE) $(REQALL)