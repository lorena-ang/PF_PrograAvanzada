#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#define PORT 8080

char sequences[1000][20000];
char resSequences[1000][20000];
int sequencesCont = 0;
char *reference;

struct _ThreadArgs
{
	int tid;
	char *sequence;
	int pos;
};

void *threadFunc(void *arg)
{
	struct _ThreadArgs *args = ((struct _ThreadArgs *)arg);
	int tid = args->tid;
	char *sequence = args->sequence;
	int result = -1;

	char *compare;
	compare = strstr(reference, sequence);
	if (compare != NULL)
	{
		int position = compare - reference;
		result = position;
	}
	args->pos = result;
	pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[2048];

	// SOCKET CREATION
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}
	printf("Socket has been created.\n");

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	// BIND
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}
	printf("Bound.\n");

	// LISTEN
	if (listen(server_fd, 3) < 0)
	{
		perror("Listen");
		exit(EXIT_FAILURE);
	}
	printf("Waiting for connections...\n");

	// ACCEPT
	while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)))
	{
		printf("Connection accepted.\n");

		while ((valread = read(new_socket, buffer, 2048)))
		{
			char *sp = ";";
			int option = atoi(strtok(buffer, sp));
			printf("Option => %d\n", option);

			if (option == 1)
			{
				// Salvar length del archivo.
				long int fileLengthReference = atoi(strtok(NULL, sp));
				memset(buffer, 0, sizeof(buffer));
				printf("fileLengthReference => %ld\n", fileLengthReference);

				// Asignar memoria a la variable 'reference', donde se guardarán los datos de reference.txt
				reference = malloc(fileLengthReference * sizeof(char));

				// Todos los datos que se reciban hasta que se llene la variable 'reference' pertenecen a reference.txt
				char data[2048];
				while (strlen(reference) < fileLengthReference)
				{
					read(new_socket, data, 2048);
					strcat(reference, data);
					memset(data, 0, sizeof(data));
				}

				// Confirmar al cliente la correcta recepción de los datos.
				send(new_socket, "Reference data stored successfully.", strlen("Reference data stored successfully."), 0);
			}
			else if (option == 2)
			{
				// Salvar length del archivo.
				int fileLengthSequences = atoi(strtok(NULL, sp));
				memset(buffer, 0, sizeof(buffer));
				printf("fileLengthSequences => %d\n", fileLengthSequences);

				sequencesCont = 0;
				char data[2049] = "";
				int p = 0;
				int seg = 0;

				// !! CAMBIAR 3 POR 1000 AL USAR EL ARCHIVO FINAL.
				while (sequencesCont < fileLengthSequences)
				{
					recv(new_socket, data, 2048, 0);
					data[2048] = '\0';

					for (int i = 0; i < strlen(data); i++)
					{
						if (data[i] == '\n')
						{
							sequences[sequencesCont][p] = '\0';
							sequencesCont = sequencesCont + 1;
							p = 0;
						}
						else
						{
							sequences[sequencesCont][p] = data[i];
							p = p + 1;
						}
					}

					if (strlen(data) < 2048)
					{
						sequencesCont = sequencesCont + 1;
					}

					memset(data, 0, sizeof(data));
				}
				// Confirmar al cliente la correcta recepción de los datos.
				send(new_socket, "Sequences data stored successfully.", strlen("Sequences data stored successfully."), 0);
			}
			else if (option == 3)
			{
				// Retornar los resultados.
				// PRUEBAS DE VARIABLES
				// printf("Reference => %s\n", reference);

				int sizeRef = strlen(reference);
				int *arrSeq = malloc(sequencesCont * sizeof(int));
				int *arrRef = malloc(sizeRef * sizeof(int));
				int dividir = sequencesCont / 10;
				int lef = sequencesCont % 10;
				int mux;
				for (int j = 0; j < dividir; j++)
				{
					mux = 10 * j;
					pthread_t threads[10];
					struct _ThreadArgs thread_args[10];
					int rc;

					/* spawn the threads */
					for (int i = 0; i < 10; i++)
					{
						thread_args[i].tid = i + mux;
						thread_args[i].sequence = sequences[i + mux];
						rc = pthread_create(&threads[i], NULL, threadFunc, (void *)&thread_args[i]);
					}

					/* wait for threads to finish */
					for (int i = 0; i < 10; ++i)
					{
						rc = pthread_join(threads[i], NULL);
					}

					for (int i = 0; i < 10; ++i)
					{
						// printf("%d,  %d \n", j + 1, i + mux);
						arrSeq[i + mux] = thread_args[i].pos;
					}
				}
				if (lef > 0)
				{
					mux = dividir * 10;
					pthread_t threads[lef];
					struct _ThreadArgs thread_args[lef];
					int rc;

					/* spawn the threads */
					for (int i = 0; i < lef; i++)
					{
						thread_args[i].tid = i + mux;
						thread_args[i].sequence = sequences[i + mux];
						rc = pthread_create(&threads[i], NULL, threadFunc, (void *)&thread_args[i]);
					}

					/* wait for threads to finish */
					for (int i = 0; i < lef; ++i)
					{
						rc = pthread_join(threads[i], NULL);
					}

					for (int i = 0; i < lef; ++i)
					{
						printf("a %d \n", i + mux);
						arrSeq[i + mux] = thread_args[i].pos;
					}
				}

				int contSecMap = 0;
				for (int i = 0; i < sequencesCont; i++)
				{
					if (arrSeq[i] != -1)
					{
						contSecMap += 1;
						int from = arrSeq[i];
						int to = arrSeq[i] + strlen(sequences[i]);
						for (int j = from; j < to; j++)
						{
							arrRef[j] = 1;
						}
					}
				}
				int contFound = 0;
				for (int i = 0; i < sizeRef; i++)
				{
					if (arrRef[i] == 1)
					{
						contFound += 1;
					}
				}
				double res = contFound;
				res /= sizeRef;

				// Almacenar resultados en arreglo sequencesSingle
				for (int i = 0; i < sequencesCont; i++)
				{
					char resSequence[20000] = "";

					if (arrSeq[i] != -1)
					{
						snprintf(resSequence, sizeof resSequence, "%s a partir del caracter: %d\n", sequences[i], arrSeq[i]);
						strcpy(resSequences[i], resSequence);
						// printf("%s", resSequences[i]);
					}
					else
					{
						snprintf(resSequence, sizeof resSequence, "%s no se encontró.\n", sequences[i]);
						strcpy(resSequences[i], resSequence);
						// printf("%s", resSequences[i]);
					}
				}

				int segCont = 0;
				int count2 = 0;
				char segment2[2048] = "";

				// Mandar resultados por partes
				while (segCont < sequencesCont)
				{
					int seqSize = strlen(resSequences[segCont]);

					for (int i = 0; i < seqSize; i++)
					{
						if (count2 == 2048)
						{
							count2 = 0;
							// printf("%s", segment2);
							memset(segment2, 0, sizeof(segment2));
						}

						segment2[count2] = resSequences[segCont][i];
						count2++;

						if (count2 == 2047)
						{
							send(new_socket, segment2, strlen(segment2), 0);
							printf("%s", segment2);
							i--;
						}
					}

					send(new_socket, segment2, strlen(segment2), 0);
					printf("%s", segment2);
					segCont++;
					count2 = 0;
				}

				char resPercentage[200] = "";
				snprintf(resPercentage, sizeof resPercentage, "El archivo cubre el %.2f%% del genoma de referencia.\n%d secuencias mapeadas.\n%d secuencias no mapeadas.\n", (res * 100), contSecMap, sequencesCont - contSecMap);
				printf("%s", resPercentage);
				send(new_socket, resPercentage, strlen(resPercentage), 0);
			}
			else if (option == 0)
			{
				// Salir.
				printf("Sesión terminada.\n");
			}
		}
	}

	if (new_socket < 0)
	{
		perror("Accept failed.");
	}

	// closing the connected socket
	close(new_socket);
	// closing the listening socket
	shutdown(server_fd, SHUT_RDWR);
	return 0;
}
