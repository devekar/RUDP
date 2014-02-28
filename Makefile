client := gamma
server := beta
tcpd_in_port := 5555
tcpd_out_port := 5556
troll_in_port := 5557
tcpd_recv_port := 5558
file := 1.jpg

all:
	gcc ftps.c -o ftps
	gcc ftpc.c -o ftpc
	gcc tcpd.c -o tcpd
	mkdir server_dir
	
client_troll:
	troll -C $(server) -S localhost -a $(tcpd_recv_port) -b $(tcpd_out_port) -x 0 -g 0 -l 0 -m 0 -r -c 10000 $(troll_in_port)

server_troll:
	troll -C $(client) -S localhost -a $(tcpd_recv_port) -b $(tcpd_out_port) -x 0 -g 0 -l 0 -m 0 -r -c 10000 $(troll_in_port)

run_tcpd:
	tcpd

run_ftps:
	ftps 3333

run_ftpc:
	ftpc localhost 3333 $(file)

clean:
	rm -rf ftpc ftps tcpd
	rm -rf server_dir/*
	rmdir server_dir
	