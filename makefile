all: ping 

ping: ping.c
	gcc -o ping ping.c

receiver: Receiver.c
	gcc -o receiver Receiver.c

clean:
	rm -f *.o sender receiver

runping:
	./ping

runr:
	./receiver