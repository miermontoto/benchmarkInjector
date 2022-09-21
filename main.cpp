#include <windows.h> // DWORD, HANDLE, WINAPI
#include <iostream> // cerr, endl
#include <fstream> // save results to file
using namespace std; // this removes the need of writing "std::"" every time.

constexpr auto MAXUSUARIOS = 10;
constexpr auto MAXPETICIONES = 10;
constexpr auto PUERTO = 57000;
constexpr auto TAM_PET = 1250;
constexpr auto TAM_RES = 1250;
constexpr auto SERVERIP = "127.0.0.1";

unsigned int numUsuarios;
unsigned int numPeticiones;
float tReflex;

typedef struct {
	int contPet;
	float reflex[MAXPETICIONES];
} threadParams;

threadParams threadInfo[MAXUSUARIOS];

// ---

float GenerateRandomFloat(float lowerLimit, float upperLimit) {
	float num = (float)rand();
	num = num * (upperLimit - lowerLimit) / RAND_MAX;
	num += lowerLimit;
	return num;
}

float GenerateExponentialDistribution(float average) {
	float num = GenerateRandomFloat(0, 1);
	while (num == 0 || num == 1) {
		num = GenerateRandomFloat(0, 1);
	}
	return (-average) * logf(num);
}

void errorMessage(string message) {
	cerr << message << endl;
	exit(EXIT_FAILURE);
}

// ---

DWORD WINAPI Usuario(LPVOID parametro) {
	DWORD dwResult = 0;
	int numHilo = *((int*) parametro);
	float tiempo;
	SOCKET s;
	char peticion[TAM_PET];
	char respuesta[TAM_RES];

	threadInfo[numHilo].contPet = 0;

	for (int i = 0; i < numPeticiones; i++) {
		//printf("[DEBUG] Peticion: %d, usuario: %d\n", i, numHilo);

		s = socket(AF_INET, SOCK_STREAM, 0);

		if (s == INVALID_SOCKET) {
			errorMessage("Ha ocurrido un error al inicializar el socket.");
		}

		sockaddr_in serv;
		serv.sin_family = AF_INET;
		serv.sin_addr.s_addr = inet_addr(SERVERIP); // placeholder, not the true server ip
		serv.sin_port = htons(57000 + numHilo);
		//cout << "Utilizando IP " << inet_ntoa(serv.sin_addr) << "..." << endl;

		// connect
		if (connect(s, (struct sockaddr*) & serv, sizeof(serv)) == SOCKET_ERROR) {
			errorMessage("Error al conectar al servidor.");
		}

		// send
		if (send(s, peticion, sizeof(peticion), 0) == SOCKET_ERROR) {
			errorMessage("Error al enviar una cadena.");
		}

		// receive
		if (recv(s, respuesta, sizeof(respuesta), 0) != TAM_RES) {
			errorMessage("Error al recibir la respuesta.");
		}

		cout << ". ";

		// close
		if (closesocket(s) != 0) {
			errorMessage("Error al cerrar el socket.");
		}

		tiempo = GenerateExponentialDistribution((float)tReflex);

		threadInfo[numHilo].reflex[i] = tiempo;
		threadInfo[numHilo].contPet++;

		Sleep(tiempo);
	}
	return dwResult;
}



int main(int argc, char *argv[]) {
	HANDLE handleThread[MAXUSUARIOS];
	int parametro[MAXUSUARIOS];

	if (argc != 4) {
		cout << "Introducir num. usuarios: ";
		cin >> numUsuarios;
		cout << "Introducir num. peticiones: ";
		cin >> numPeticiones;
		cout << "Tiempo de reflexion despues de cada peticion: ";
		cin >> tReflex;
	}
	else {
		numUsuarios = atoi(argv[1]);
		numPeticiones = atoi(argv[2]);
		tReflex = atoi(argv[3]);
	}

	// init socket connection
	WORD wVersionRequested = MAKEWORD(2, 0);
	WSAData wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		errorMessage("Ha ocurrido un error al inicializar el uso de sockets.");
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0) {
		errorMessage("La liberia no soporta la version 2.0");
	}

	cout << "Transmitiendo ";
	for (int i = 0; i < numUsuarios; i++) {
		parametro[i] = i;
		handleThread[i] = CreateThread(NULL, 0, Usuario, &parametro[i], 0, NULL);
		if (handleThread[i] == NULL) {
			errorMessage("Error al lanzar el hilo.");
		}
	}

	for (int i = 0; i < numUsuarios; i++) { // el hilo principal espera por sus hijos.
		WaitForSingleObject(handleThread[i], INFINITE);
	}

	WSACleanup();

	ofstream output("output.csv");
	output << "User,Counter,";
	for (int i = 0; i < numUsuarios; i++) {
		output << "Time" << i << ",";
	}
	output << "Total" << endl;

	cout << endl << "RESULTADOS:" << endl;
	for (int i = 0; i < numUsuarios; i++) {
		auto cont = threadInfo[i].contPet;
		cout << "Usuario: " << i << ", contador: " << cont;
		output << i << "," << cont << ",";
		
		cout << ", tiempos: ";
		float totalReflex = 0;
		for (int j = 0; j < numPeticiones; j++) {
			auto reflex = threadInfo[i].reflex[j];

			cout << reflex << " ";
			output << reflex << ",";

			totalReflex += reflex;
		}

		cout << ", tiempo total: " << totalReflex << endl;
		output << totalReflex << endl;
	}

	output.close();
}

