#include <windows.h> // DWORD, HANDLE, WINAPI
#include <iostream> // cerr, endl
using namespace std; // this removes the need of writing "std::"" every time.

constexpr auto MAXUSUARIOS = 10;
constexpr auto MAXPETICIONES = 10;

int numUsuarios;
int numPeticiones;
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

	threadInfo[numHilo].contPet = 0;

	for (int i = 0; i < numPeticiones; i++) {
		printf("[DEBUG] Peticion: %d, usuario: %d\n", i, numHilo);

		SOCKET s = socket(AF_INET, SOCK_STREAM, 0);

		if (s == INVALID_SOCKET) {
			errorMessage("Ha ocurrido un error al inicializar el socket.");
		}

		sockaddr_in serv;
		serv.sin_family = AF_INET;
		serv.sin_addr.s_addr = inet_addr("156.35.103.10"); // placeholder, not the true server ip
		serv.sin_port = htons(57000);
		//cout << "Utilizando IP " << inet_ntoa(serv.sin_addr) << "..." << endl;
		/*
		int codigo = connect(s, (struct sockaddr *) &serv, sizeof(serv));
		if (codigo == SOCKET_ERROR) {
			errorMessage("Error al conectar al servidor.");
		}
		*/

		// send
		// receive

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



int main() {
	HANDLE handleThread[MAXUSUARIOS];
	int parametro[MAXUSUARIOS];

	cout << "Introducir num. usuarios: ";
	cin >> numUsuarios;
	cout << "Introducir num. peticiones: ";
	cin >> numPeticiones;
	cout << "Tiempo de reflexion despues de cada peticion: ";
	cin >> tReflex;

	WORD wVersionRequested = MAKEWORD(2, 0);
	WSAData wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		errorMessage("Ha ocurrido un error al inicializar el uso de sockets.");
	}

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

	// guardar y recopilar resultados

	WSACleanup();
}

