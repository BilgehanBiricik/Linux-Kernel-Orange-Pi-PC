// UDP client-server modelinin server tarafı
// Client-to-Client One Way Data Transfer Over From Server
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFLEN 256

// socket programlama için gerekli değişkenlerin tanımlanması.
// Değişkenler ve fonksiyonların parametreleri hakkında detaylı bilgi: https://www.geeksforgeeks.org/udp-server-client-implementation-c/
int sockfd, cli_arr_port[2], client_len, fd;
struct sockaddr_in servaddr, cliaddr, cli_arr_addr[2];
char buffer[BUFLEN];

// Server'ın oluşturulması.
void udp_server_conn()
{
    client_len = sizeof(cliaddr);

    // socket file descriptor'ünün oluşturulması.
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("[-] Soket oluşturulamadı.\n");
        exit(1);
    }
    else
        printf("[+] Soket oluşturuldu..\n");

    // Değişkenlerin içinlerinin sıfırlanması.
    bzero(&servaddr, sizeof(servaddr));
    bzero(&cliaddr, sizeof(cliaddr));
    bzero(cli_arr_addr, sizeof(cli_arr_addr));
    bzero(cli_arr_port, sizeof(cli_arr_port));

    // Server'ın bilgilerinin tanımlanması.
    servaddr.sin_family = AF_INET;         // IPv4 için.
    servaddr.sin_addr.s_addr = INADDR_ANY; // Server'ın IP adresi. Yani localhost.
    servaddr.sin_port = htons(PORT);       // Server'ın port numarası.

    // Oluşturulan sokcet'e adresin atanması.
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("[-] Bind başarısız oldu.\n");
        exit(1);
    }
    else
        printf("[+] Bind başarılı..\n");

    // buffer'ın içinin sıfırlanması.
    memset(buffer, '\0', BUFLEN);
}

// Temp_Control_Device'ının açılması.
void open_device()
{
    if ((fd = open("/dev/Temp_Control_Device", O_RDWR)) < 0)
    {
        perror("[-] Device açılamadı.\n");
        exit(1);
    }
}

int main(int argc, char const *argv[])
{
    udp_server_conn();
    open_device();

    // Client1'den gelen sıcaklık değerlerinin Client2'ye gönderilmesi bu döngünün içinde gerçekleşiyor.
    while (1)
    {
        // Clientlardan mesajların alınması.
        if (recvfrom(sockfd, (char *)buffer, BUFLEN, 0, (struct sockaddr *)&cliaddr, &client_len) < 0)
        {
            perror("[-] Mesaj alınamadı! \n");
            exit(1);
        }

        // Client1 adresi ve port numarası client dizilerine atanıyor.
        if (strcmp(buffer, "client1") == 0)
        {
            cli_arr_addr[0] = cliaddr;
            cli_arr_port[0] = ntohs(cliaddr.sin_port);
            printf("Client 1  bağlandı. Port: %d\n", cli_arr_port[0]);
        }
        // Client2 adresi ve port numarası client dizilerine atanıyor.
        else if (strcmp(buffer, "client2") == 0)
        {
            cli_arr_addr[1] = cliaddr;
            cli_arr_port[1] = ntohs(cliaddr.sin_port);
            printf("Client 2 bağlandı. Port: %d\n", cli_arr_port[1]);
            // Device Driver'a gelen sıcaklık değeri yazılıyor.
            write(fd, buffer, strlen(buffer));
            // Bu değere karşılık gelen renk kodunu okuyor.
            read(fd, buffer, BUFLEN);
            // Renk kodu rgb led client'a gönderiliyor.
            sendto(sockfd, (const char *)buffer, BUFLEN, MSG_CONFIRM, (const struct sockaddr *)&cliaddr, client_len);
        }
        // Clientlar bağlandıktan sonra Client1'den Client2'ye veri akışı gerçekleştiriliyor.
        else
        {
            // Mesajları gönderenin Client1 olduğu port numarasına bakarak doğrulanıyor.
            if (ntohs(cliaddr.sin_port) == cli_arr_port[0])
            {
                // Device Driver'a gelen sıcaklık değeri yazılıyor.
                write(fd, buffer, strlen(buffer));
                // Bu değere karşılık gelen renk kodunu okuyor.
                read(fd, buffer, BUFLEN);
                // Renk kodu rgb led client'a gönderiliyor.
                sendto(sockfd, (const char *)buffer, BUFLEN, MSG_CONFIRM, (const struct sockaddr *)&cli_arr_addr[1], client_len);
            }
        }
    }
    close(sockfd);
    close(fd);
    return 0;
}
