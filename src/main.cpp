// Juan Mier (UO283319) '22

#include <windows.h> // DWORD, HANDLE, WINAPI
#include <iostream> // cout, cerr, endl
#include <fstream> // save results to file
#include <time.h> // time, time_t, QueryPerformance
#include <math.h> // HighPart, LowPart, QuadPart
using namespace std; // this removes the need of writing "std::"" every time.

constexpr unsigned int MAXUSERS = 10000;
constexpr unsigned int MAXPETITIONS = 10000;
constexpr unsigned int PORT = 57000;
constexpr unsigned int PETITION_SIZE = 1250;
constexpr unsigned int RESPONSE_SIZE = 1250;
constexpr auto SERVERIP = "192.168.203.231"; // 192.168.203.231

unsigned int totalUsers;
float reflexTime;
float ticksPerMs;
LARGE_INTEGER tickBase;
LARGE_INTEGER tickStart;
LARGE_INTEGER tickEnd;

typedef struct {
	int petitionCounter;
	float reflex[MAXPETITIONS];
	float responseTime[MAXPETITIONS];
	unsigned long ciclosIniPeticion[MAXPETITIONS];
	unsigned long ciclosFinPeticion[MAXPETITIONS];

} threadParams;

threadParams threadInfo[MAXUSERS];

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

double TimeDiff(LARGE_INTEGER start, LARGE_INTEGER end) {
	LARGE_INTEGER diff;

	diff.QuadPart = end.QuadPart - start.QuadPart;
	float ms = diff.LowPart / ticksPerMs;
	if (diff.HighPart != 0) {
		ms += (float)ldexp((double)diff.HighPart, 32) / ticksPerMs;
	}
	return ms;
}

// ---

DWORD WINAPI Usuario(LPVOID parameter) {
	int threadNum = *((int*) parameter);
	char petition[PETITION_SIZE];
	char response[RESPONSE_SIZE];
	LARGE_INTEGER timeStart, timeEnd;

	threadInfo[threadNum].petitionCounter = 0;

	do {
		//printf("[DEBUG] Peticion: %d, usuario: %d\n", i, numHilo);

		SOCKET s = socket(AF_INET, SOCK_STREAM, 0);

		if (s == INVALID_SOCKET) {
			WSACleanup();
			errorMessage("Ha ocurrido un error al inicializar el socket.");
		}

		QueryPerformanceCounter(&timeStart);

		sockaddr_in serv;
		serv.sin_family = AF_INET;
		serv.sin_addr.s_addr = inet_addr(SERVERIP);
		serv.sin_port = htons(PORT + threadNum);

		// connect
		if (connect(s, (struct sockaddr*) & serv, sizeof(serv)) == SOCKET_ERROR) {
			WSACleanup();
			errorMessage("Error al conectar al servidor.");
		}

		// send
		if (send(s, petition, sizeof(petition), 0) == SOCKET_ERROR) {
			WSACleanup();
			errorMessage("Error al enviar una cadena.");
		}

		// receive
		if (recv(s, response, sizeof(response), 0) != RESPONSE_SIZE) {
			WSACleanup();
			errorMessage("Error al recibir la respuesta.");
		}


		// close
		if (closesocket(s) != 0) {
			WSACleanup();
			errorMessage("Error al cerrar el socket.");
		}

		float tiempo = GenerateExponentialDistribution((float)reflexTime);
		QueryPerformanceCounter(&timeEnd);


		if (timeStart.QuadPart > tickStart.QuadPart&& timeEnd.QuadPart < tickEnd.QuadPart) {
			threadInfo[threadNum].responseTime[threadInfo[threadNum].petitionCounter] = (float)TimeDiff(timeStart, timeEnd);
			threadInfo[threadNum].reflex[threadInfo[threadNum].petitionCounter] = tiempo;
			threadInfo[threadNum].ciclosIniPeticion[threadInfo[threadNum].petitionCounter] = (unsigned)(timeStart.QuadPart - tickBase.QuadPart);
			threadInfo[threadNum].ciclosFinPeticion[threadInfo[threadNum].petitionCounter] = (unsigned)(timeEnd.QuadPart - tickBase.QuadPart);

			threadInfo[threadNum].petitionCounter++;
			if (threadInfo[threadNum].petitionCounter > MAXPETITIONS) {
				errorMessage("Superado el limite de peticiones por hilo.");
			}
		}

		Sleep((int) (tiempo*1000));

	} while (timeStart.QuadPart < tickEnd.QuadPart);

	return 0;
}



int main(int argc, char *argv[]) {
	HANDLE handleThread[MAXUSERS];
	int parametro[MAXUSERS];
	int segCal;
	int segMed;

	time_t timeIniExp;
	time_t timeIniMed;
	time_t timeFinMed;

	if (argc != 5) {
		cout << "Introducir num. usuarios: ";
		cin >> totalUsers;
		cout << "Tiempo de reflexion despues de cada peticion: ";
		cin >> reflexTime;
		cout << "Introduzca el tiempo de calentamiento (en segundos): ";
		cin >> segCal;
		cout << "Introduzca el tiempo de medicion (en segundos): ";
		cin >> segMed;
	}
	else {
		totalUsers = atoi(argv[1]);
		reflexTime = atof(argv[2]);
		segCal = atoi(argv[3]);
		segMed = atoi(argv[4]);

		cout << "Num. usuarios: " << totalUsers << endl;
		cout << "Tiempo de reflexion: " << reflexTime << endl;
		cout << "Tiempo de calentamiento: " << segCal << endl;
		cout << "Tiempo de medicion: " << segMed << endl;
	}

	cout << "Utilizando IP " << SERVERIP << endl;

	if (totalUsers > MAXUSERS ||
			totalUsers <= 0 || reflexTime <= 0 ||
				segCal < 0 || segMed <= 0) {
		errorMessage("Arumentos invalidos.");
	}

	time(&timeIniExp);
	timeIniMed = timeIniExp + segCal;
	timeFinMed = timeIniMed + segMed;

	LARGE_INTEGER ticksPerSec;
	if (!QueryPerformanceFrequency(&ticksPerSec)) {
		cout << "No esta disponible el contador de alto rendimiento." << endl;
		exit(EXIT_FAILURE);
	}

	ticksPerMs = (float)(ticksPerSec.LowPart / 1E3);

	QueryPerformanceCounter(&tickBase);
	tickStart.QuadPart = tickBase.QuadPart + (LONGLONG)(segCal * 1000 * ticksPerMs);
	tickEnd.QuadPart = tickStart.QuadPart + (LONGLONG)(segMed * 1000 * ticksPerMs);

	// init socket connection
	WORD wVersionRequested = MAKEWORD(2, 0);
	WSAData wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		errorMessage("Ha ocurrido un error al inicializar el uso de sockets.");
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0) {
		errorMessage("La liberia no soporta la version 2.0");
	}

	cout << "Transmitiendo...";
	for (int i = 0; i < totalUsers; i++) {
		parametro[i] = i;
		handleThread[i] = CreateThread(NULL, 0, Usuario, &parametro[i], 0, NULL);
		if (handleThread[i] == NULL) {
			errorMessage("Error al lanzar el hilo.");
		}
	}

	for (int i = 0; i < totalUsers; i++) { // el hilo principal espera por sus hijos.
		WaitForSingleObject(handleThread[i], INFINITE);
	}

	WSACleanup();

	ofstream output("output.csv");
	output << "User,Petition,Reflex,Tstart,Tend,Tres" << endl;

	float responseTime = 0;
	float responseTime2 = 0;
	float taux1, taux2;
	int totalPetitions = 0;


	for (int i = 0; i < totalUsers; i++) {
		totalPetitions += threadInfo[i].petitionCounter;
	
		for (int j = 0; j < threadInfo[i].petitionCounter; j++) {
			auto reflex = threadInfo[i].reflex[j];
			responseTime += threadInfo[i].responseTime[j];
			taux1 = threadInfo[i].ciclosIniPeticion[j] / ticksPerMs;
			taux2 = threadInfo[i].ciclosFinPeticion[j] / ticksPerMs;
			responseTime2 = taux2 - taux1;

			output << i << "," << j << "," << reflex << "," << taux1 << "," << taux2 << "," << responseTime2 << endl;
		}
	}

	cout << endl << "RESULTADOS:" << endl;
	cout << "Num. peticiones: " << totalPetitions << endl;
	cout << "Segmento de medicion: " << segMed << endl;
	cout << "Productividad: " << (float)totalPetitions / segMed << endl;
	cout << "Tiempo de respuesta: " << (float)responseTime / totalPetitions << endl;

	output.close();
}
