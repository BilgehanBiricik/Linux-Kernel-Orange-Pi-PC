// UDP client-server modelinin client tarafı
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <fcntl.h>

#define PORT 8080
#define BUFLEN 256

// MAC adresinden Server'ın IP adresinin bulunması.
const char *get_ip_from_mac(char *macaddr)
{
	// Değişkenler
	FILE *fp;
	char buffer[100];

	// nmap komutuyla local network'teki cihazların taranması
	// system("nmap -sP 192.168.1.0/24");
	// sleep(3);

	// arp komutuyla belirlenen mac adresinin ipsinin getirilmesi
	char command[30];
	memset(command, '\0', 30);
	strcat(command, "arp | grep ");
	strcat(command, macaddr);

	// popen fonksiyonu ile bu komutun çalıştırılması
	if ((fp = popen(command, "r")) == NULL)
	{
		printf("Başarısız.\n");
		exit(1);
	}
	// Komutun çıktısının buffer değişkenine atanması
	fgets(buffer, sizeof(buffer), fp);

	// IP'nin pbuf char pointer'ının içine atanması
	int i = 0;
	for (i = 0; buffer[i] != ' '; i++);
	char *pbuf = (char *)malloc(sizeof(char) * (i + 1));
	for (i = 0; buffer[i] != ' '; i++)
		pbuf[i] = buffer[i];
	pbuf[i + 1] = '\0';

	pclose(fp);

	return pbuf;
}

int main(int argc, char const *argv[])
{
	// socket programlama için gerekli değişkenlerin tanımlanması.
	// Değişkenler ve fonksiyonların parametreleri hakkında detaylı bilgi: https://www.geeksforgeeks.org/udp-server-client-implementation-c/
	int sockfd, fd;
	struct sockaddr_in servaddr;
	char buffer[BUFLEN];

	// socket file descriptor'ünün oluşturulması.
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("Socket oluşturulması başarısız.");
		exit(EXIT_FAILURE);
	}

	// buffer'ın ve servaddr'nin içinin sıfırlanması.
	bzero(&servaddr, sizeof(servaddr));
	memset(buffer, '\0', BUFLEN);

	// Server'a ait bilgilerin atanması.
	servaddr.sin_family = AF_INET;												// IPv4 için
	servaddr.sin_addr.s_addr = inet_addr(get_ip_from_mac("d8:cb:8a:73:8b:6a")); // Server'ın IP adresi
	servaddr.sin_port = htons(PORT);											// Server'ın port numarası

	// DHT11_Device'ının açılması.
	if ((fd = open("/dev/DHT11_Device", O_RDWR)) < 0)
	{
		perror("Device açılamadı.\n");
		exit(1);
	}

	// Server'a 1 numaralı client olduğuna dair mesaj gönderiliyor.
	strcpy(buffer, "client1");
	sendto(sockfd, (const char *)buffer, BUFLEN, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));

	// Device'dan gelen mesajların Server'a gönderilmesi bu kod bloğu içinde gerçekleşiyor.
	while (1)
	{
		// Device'daki sıcaklık değerleri buffer'a okunuyor.
		read(fd, buffer, BUFLEN);
		// Ve Server'a gönderiliyor.
		sendto(sockfd, (const char *)buffer, BUFLEN, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
		printf("Sıcaklık: %s\n", buffer);
		sleep(2);
	}

	close(sockfd);
	close(fd);
	return 0;
}