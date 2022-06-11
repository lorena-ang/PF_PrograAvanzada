#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#define _OPEN_SYS_ITOA_EXT
#define PORT 8080

long int findSize(char file_name[])
{
	FILE *fp = fopen(file_name, "r");

	if (fp == NULL)
	{
		printf("File Not Found!\n");
		return -1;
	}

	fseek(fp, 0L, SEEK_END);
	long int size = ftell(fp);

	fclose(fp);

	return size;
}

int countLines(char *filename)
{
	FILE *fp = fopen(filename, "r");
	int ch = 0;
	int lines = 0;

	while (!feof(fp))
	{
		ch = fgetc(fp);
		if (ch == '\n')
		{
			lines++;
		}
	}
	lines++;

	fclose(fp);
	return lines;
}

int main(int argc, char const *argv[])
{
	int sock = 0, opcion, valread, client_fd;
	struct sockaddr_in serv_addr;
	char buffer[2048] = {};
	char fileNameReference[] = {"reference.txt"};
	char fileNameSequence[] = {"sequences.seq"};
	char line[500];
	char ch;
	long size;
	int size2;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	// Convert IPv4 and IPv6 addresses from text to binary form.
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	if ((client_fd = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
	{
		printf("\nConnection Failed \n");
		return -1;
	}

	printf("\nConnected.\n");

	printf("\nAnálisis de ADN\n");
	printf("Opciones:\n");
	printf("1. Leer archivo de referencia.\n");
	printf("2. Leer archivo de secuencias.\n");
	printf("3. Recibir resultados.\n");
	printf("0. Salir.\n");
	printf("Ingrese la opción deseada [0-3]: ");
	scanf("%d", &opcion);

	do
	{
		switch (opcion)
		{
		case 0:
			printf("Sesión terminada.\n");
			break;
		case 1:
			// Tamaño total del archivo.
			size = findSize(fileNameReference);

			char instruction[20];
			snprintf(instruction, sizeof instruction, "1;%ld;", size);

			send(sock, instruction, strlen(instruction), 0);

			// Leer archivo de referencia.
			FILE *ptrFileReference;
			ptrFileReference = fopen(fileNameReference, "r");

			// Si el archivo no está disponible:
			if (ptrFileReference == NULL)
			{
				printf("reference.txt error.\n");
			};

			int count = 0;
			char segment[2048];
			while ((ch = fgetc(ptrFileReference)) != EOF)
			{
				if (count == 2048)
				{
					if (send(sock, segment, strlen(segment), 0) < 0)
					{
						puts("Send failed.");
						return 1;
					}
					count = 0;
					memset(segment, 0, sizeof(segment));
				}
				strcpy(&segment[count], &ch);
				count++;
			}
			if (send(sock, segment, strlen(segment), 0) < 0)
			{
				puts("Send failed.");
				return 1;
			}

			fclose(ptrFileReference);

			// Recibir confirmación del servidor.
			read(sock, buffer, 2048);
			printf("\n%s\n", buffer);

			break;
		case 2:
			// Tamaño total del archivo.
			size2 = countLines(fileNameSequence);

			char instruction2[20];
			snprintf(instruction2, sizeof instruction2, "2;%d;", size2);

			send(sock, instruction2, strlen(instruction2), 0);

			// Leer archivo de sequences.
			FILE *ptrFileSequences;
			ptrFileSequences = fopen(fileNameSequence, "r");

			// Si el archivo no está disponible:
			if (ptrFileSequences == NULL)
			{
				printf("sequences.seq error.\n");
			};

			// Lectura del archivo.
			int count2 = 0;
			char segment2[2048] = "";
			while ((ch = fgetc(ptrFileSequences)) != EOF)
			{
				if (count2 == 2048)
				{
					if (send(sock, segment2, strlen(segment2), 0) < 0)
					{
						puts("Send failed.");
						return 1;
					}
					count2 = 0;
					memset(segment2, 0, sizeof(segment2));
				}
				strcpy(&segment2[count2], &ch);
				count2++;
			}
			if (send(sock, segment2, strlen(segment2), 0) < 0)
			{
				puts("Send failed.");
				return 1;
			}

			fclose(ptrFileSequences);

			// Recibir confirmación del servidor.
			read(sock, buffer, 2048);
			printf("\n%s\n", buffer);

			break;
		case 3:
			// Recibir resultados.
			send(sock, "3;", strlen("3;"), 0);

			char segment3[2049] = "";
			int secCont = 0;

			// Mandar resultados por partes
			while (secCont < size2)
			{
				// printf("\nsecCONT: %d\n", secCont);
				recv(sock, segment3, 2048, 0);
				segment3[2048] = '\0';

				for (int i = 0; i < strlen(segment3); i++)
				{
					if (segment3[i] == '\n')
					{
						secCont++;
						printf("\n");
					}
					else
					{
						printf("%c", segment3[i]);
					}
				}

				// if (strlen(segment3) < 2048)
				// {
				// 	secCont++;
				// }

				memset(segment3, 0, sizeof(segment3));
			}

			char resPercentage[201] = "";
			recv(sock, resPercentage, 200, 0);
			resPercentage[200] = '\0';
			printf("\n%s", resPercentage);

			break;
		default:
			printf("Opción inválida, intente de nuevo.\n");
		}
		printf("\nAnálisis de ADN\n");
		printf("Opciones:\n");
		printf("1. Leer archivo de archivo de referencia.\n");
		printf("2. Leer archivo de secuencias.\n");
		printf("3. Recibir resultados.\n");
		printf("0. Salir.\n");
		printf("Ingrese la opción deseada [0-3]: ");
		scanf("%d", &opcion);
	} while (opcion != 0);

	send(sock, "0;", strlen("0;"), 0);

	// Closing the connected socket.
	close(client_fd);
	return 0;
}
