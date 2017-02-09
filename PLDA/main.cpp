#include <iostream>
#include <mpi.h>


using namespace std;

int main(int argc, char *argv[]) {
	MPI_Init(&argc, &argv);
	cout << "hello world!" << endl;
	MPI_Finalize();
}