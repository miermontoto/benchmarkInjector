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


// ---

DWORD WINAPI Usuario(LPVOID parametro) {
	DWORD dwResult = 0;
	int numHilo = *((int*) parametro);
	float tiempo;

	threadInfo[numHilo].contPet = 0;

	// ...

	for (int i = 0; i < numPeticiones; i++) {
		printf("[DEBUG] Petición: %d, usuario: %d\n", i, numHilo);
		// hacer la petición

		tiempo = GenerateExponentialDistribution((float)tReflex);

		threadInfo[numHilo].reflex[i] = tiempo; // tiempo de reflexión del hilo
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
	cout << "Tiempo de reflexión después de cada petición: ";
	cin >> tReflex;

	for (int i = 0; i < numUsuarios; i++) {
		parametro[i] = i;
		handleThread[i] = CreateThread(NULL, 0, Usuario, &parametro[i], 0, NULL);
		if (handleThread[i] == NULL) {
			cerr << "Error al lanzar el hilo." << endl;
			exit(EXIT_FAILURE);
		}
		WaitForSingleObject(handleThread[i], INFINITE);
	}

	// guardar y recopilar resultados

}

