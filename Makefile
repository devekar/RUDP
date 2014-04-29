client := gamma
server := beta
tcpd_in_port := 5554
tcpd_out_port := 5556
troll_in_port := 5557
tcpd_recv_port := 5558
file := 1.jpg
ccpp = g++ -std=c++0x
cc = gcc

all:
	$(cc) ftps.c -o ftps
	$(cc) ftpc.c -o ftpc
	$(ccpp) tcpd.cpp tcp.cpp delta-timer/functions.cpp -o tcpd
	make -C delta-timer
	mkdir server_dir
	
CTROLL:
	troll -C $(server) -S localhost -a $(tcpd_recv_port) -b $(tcpd_out_port) -c 10000 $(troll_in_port) -t -x 25 -m 25 -g 25

STROLL:
	troll -C $(client) -S localhost -a $(tcpd_recv_port) -b $(tcpd_out_port) -c 10000 $(troll_in_port) -t -x 25 -m 25 -g 25

TCPD:
	tcpd

FTPS:
	ftps 3333

FTPC:
	ftpc localhost 3333 $(file)

TIMER:
	delta-timer/timer
	
clean:
	rm -rf ftpc ftps tcpd
	make clean -C delta-timer
	rm -rf server_dir/*
	rmdir server_dir
	
