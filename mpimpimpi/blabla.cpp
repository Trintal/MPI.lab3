#define _CRT_SECURE_NO_WARNINGS
/*
LAB #3: MSD RADIX SORT FOR DOUBLE NUMBERS
NON PARALLEL AND PARALLEL VERSIONS
DESIGNED BY NIKOLAY KOMAROV
*/

#include "iostream"
#include <string>
#include <ctime>
#include <conio.h>
#include "mpi.h"

#include <queue>

using namespace std;

int procSize;
int procRank;

enum sortingType { INCREASE, DECREASE };

sortingType orderOfSorting;

union BinaryDouble
{
	double d;
	unsigned char c[sizeof(double)];

	BinaryDouble()
	{
		d = 0;
	}
	BinaryDouble(double doub)
	{
		d = doub;
	}
	void showBits()
	{
		for (int j = sizeof(double) - 1; j >= 0; j--) // 7..0
		{
			for (int i = 128; i; i = i >> 1) // 10000000 01000000 00100000 00010000 .. 00000001
			{
				if (i & c[j])
					cout << "1";
				else
					cout << "0";
			}
		}
		cout << "\n";
	}
};

void welcomeWords(char *argv[], int *size)
{
	cout << "\n \nLAB #3: MSD RADIX SORT FOR DOUBLE NUMBERS\n \n";

	if (string(argv[1]) == "-d")
		orderOfSorting = DECREASE;
	else
	{
		if (string(argv[1]) == "-i")
			orderOfSorting = INCREASE;
	}
	*size = atoi(argv[2]);
}

void initData(BinaryDouble **dataArray, int *size)
{
	*dataArray = new BinaryDouble[*size];
	srand(time(NULL));
	rand();
	for (int i = 0; i < *size; i++)
	{
		(*dataArray)[i] = BinaryDouble((static_cast<double>(rand()) / RAND_MAX) * rand());
	}
}

void copyData(BinaryDouble **src, BinaryDouble **dest, int *size)
{
	*dest = new BinaryDouble[*size];
	for (int i = 0; i < *size; i++)
		(*dest)[i] = BinaryDouble((*src)[i].d);
}

void printArray(BinaryDouble **array, int *size)
{
	if (orderOfSorting == INCREASE)
	{
		for (int i = 0; i < *size - 1; i++)
		{
			cout << (*array)[i].d << ", ";
		}
		cout << (*array)[*size - 1].d << "\n";
	}
	else if (orderOfSorting == DECREASE)
	{
		for (int i = *size - 1; i >= 0; i--)
		{
			cout << (*array)[i].d << ", ";
		}
		cout << (*array)[*size - 1].d << "\n";
	}
}

void outputMessage(double *time1, double *time2, bool isRight)
{
	cout << "\n--- RESULTS ---\n\nTime of non-parallel algorythm: " << *time1 <<
		" ms\nTime of parallel algorythm:     " << *time2 << " ms\n" <<
		"Speedup: " << *time1 / *time2 << "\n" <<
		"Results are the same: " << isRight << "\n";
}

void RadixSort(queue<BinaryDouble> *data, queue<BinaryDouble> *sortedData, int *numOfByte, int *numOfBitInByte)
{
	queue<BinaryDouble> queueZero;
	queue<BinaryDouble> queueOne;

	if (*numOfByte != -1)
	{
		while ((*data).size() != 0)
		{
			if (!(*numOfBitInByte & (*data).front().c[*numOfByte]))
			{
				queueZero.push((*data).front());
				(*data).pop();
			}
			else
			{
				queueOne.push((*data).front());
				(*data).pop();
			}
		}
		// recursive call must be outside of while loop above
		*numOfByte = *numOfBitInByte == 1 ? --*numOfByte : *numOfByte;
		*numOfBitInByte = *numOfBitInByte == 1 ? *numOfBitInByte = 128 : *numOfBitInByte = *numOfBitInByte / 2;
		int numOfByteCopy2 = *numOfByte;
		int numOfBitInByteCopy2 = *numOfBitInByte;

		if (queueZero.size() > 1)
			RadixSort(&queueZero, sortedData, numOfByte, numOfBitInByte);
		while (queueZero.size() != 0)
		{
			(*sortedData).push(queueZero.front());
			queueZero.pop();
		}

		if (queueOne.size() > 1)
			RadixSort(&queueOne, sortedData, &numOfByteCopy2, &numOfBitInByteCopy2);
		while (queueOne.size() != 0)
		{
			(*sortedData).push(queueOne.front());
			queueOne.pop();
		}
	}
}

BinaryDouble* merge(BinaryDouble **firstArray, BinaryDouble **secondArray, int *sizeFirst, int *sizeSecond)
{
	BinaryDouble *sortedArray = new BinaryDouble[*sizeFirst + *sizeSecond];
	int indexFirst = 0;
	int indexSecond = 0;
	int index = 0;
	while ((indexFirst < *sizeFirst) && (indexSecond < *sizeSecond))
	{
		BinaryDouble elementFirst = (*firstArray)[indexFirst];
		BinaryDouble elementSecond = (*secondArray)[indexSecond];
		if (elementFirst.d < elementSecond.d)
		{
			sortedArray[index] = elementFirst;
			indexFirst++;
		}
		else
		{
			sortedArray[index] = elementSecond;
			indexSecond++;
		}
		index++;
	}

	while (indexFirst < *sizeFirst)
	{
		sortedArray[index] = (*firstArray)[indexFirst];
		indexFirst++;
		index++;
	}

	while (indexSecond < *sizeSecond)
	{
		sortedArray[index] = (*secondArray)[indexSecond];
		indexSecond++;
		index++;
	}
	return sortedArray;
}

void setResult(queue<BinaryDouble> *sortedData, BinaryDouble **data)
{
	int count = (*sortedData).size();
	for (int i = 0; i < count; i++)
	{
		(*data)[i] = (*sortedData).front();
		(*sortedData).pop();
	}
}

bool checkResult(BinaryDouble **nonParallel, BinaryDouble **parallel, int *size)
{
	for (int i = 0; i < *size; i++)
	{
		if ((*nonParallel)[i].d != (*parallel)[i].d)
			return false;
	}
	return true;
}

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);
	cout.precision(15);
	int size;
	BinaryDouble *nonParallel = nullptr;
	BinaryDouble *parallel = nullptr;
	BinaryDouble *parallelCopy = nullptr;
	int *sendCounts = nullptr;
	int *displs = nullptr;
	MPI_Comm_size(MPI_COMM_WORLD, &procSize); // procSize = total number of processes
	MPI_Comm_rank(MPI_COMM_WORLD, &procRank); // procRank = the index of process which is processing this code
	double startTime = 0;
	double endTime = 0;
	double timeOfNonParallel = 0;
	double timeOfParallel = 0;
	if (procRank == 0)
	{
		welcomeWords(argv, &size);
		initData(&nonParallel, &size);
		cout << "[TRACE] Data was initialized \n";
		copyData(&nonParallel, &parallel, &size);
		cout << "[TRACE] Data was copied \n";
		cout << "[INFO ] Size equals: " << size << "\n";

		/* ---------- START OF NON PARALLEL ALGORITHM ------------------ */
		startTime = clock(); //MPI_Wtime() * (double)1000;
		cout << "[TRACE] Start time of non-parallel algorythm: " << startTime << "\n";

		queue<BinaryDouble> queueData;
		queue<BinaryDouble> sortedData;
		for (int i = 0; i < size; i++) {
			queueData.push(nonParallel[i]);
		}
		int u = 7; int o = 128;
		RadixSort(&queueData, &sortedData, &u, &o);
		cout << "[TRACE] Array was sorted by non-parallel algorythm \n";
		setResult(&sortedData, &nonParallel);

		endTime = clock(); //MPI_Wtime() * (double)1000;
		cout << "[TRACE] End time of non-parallel algorythm: " << endTime << "\n";
		timeOfNonParallel = endTime - startTime;
		cout << "[TRACE] Total time of non-parallel algorythm: " << timeOfNonParallel << " ms\n";
		printArray(&nonParallel, &size);
			/* ----------- END OF NON PARALLEL ALGORITHM ------------------- */

			/* ------------ START OF PARALLEL ALGORITHM -------------------- */
			startTime = MPI_Wtime() * (double)1000;
			cout << "[TRACE] Start time of parallel algorythm: " << startTime << "\n";
		}
		MPI_Bcast(&size, 1, MPI_INT, 0, MPI_COMM_WORLD);
		cout << "[TRACE] Process [" << procRank << "] has received size of array \n";

		/* Calculating parameters for MPI_Scatterv */
		int nSize = size / procSize;
		if (procRank == procSize - 1)
			nSize = size - (procSize - 1) * (size / procSize);
		sendCounts = new int[procSize];
		displs = new int[procSize];

		for (int i = 0; i < procSize - 1; i++)
		{
			sendCounts[i] = nSize;
			displs[i] = 0;
		}
		displs[procSize - 1] = 0;
		sendCounts[procSize - 1] = (size - (procSize - 1) * (size / procSize));
		for (int i = 1; i < procSize; i++)
			displs[i] += i * sendCounts[i - 1];

		/* Printing claculeted parameters */
		if (procRank == 0)
		{
			for (int i = 0; i < procSize; i++)
				cout << "[INFO ] sendCounts[" << i << "] = " << sendCounts[i] << "\n";
			for (int i = 0; i < procSize; i++)
				cout << "[INFO ] displs[" << i << "] = " << displs[i] << "\n";
		}

		parallelCopy = new BinaryDouble[nSize];

		/* Creating new MPI data type */
		MPI_Datatype BinaryDoubleType;
		MPI_Datatype type[2] = { MPI_DOUBLE, MPI_UNSIGNED_CHAR };
		int blocklen[2] = { 1, 1 };
		MPI_Aint indices[2] = { 0, 7 };
		MPI_Type_create_struct(2, blocklen, indices, type, &BinaryDoubleType);
		MPI_Type_commit(&BinaryDoubleType);
		cout << "[TRACE] Process [" << procRank << "] has commited new data type \n";

		/* Sending parts of data to processes */
		MPI_Scatterv(parallel, sendCounts, displs, BinaryDoubleType, parallelCopy, sendCounts[procRank], BinaryDoubleType, 0, MPI_COMM_WORLD);
		cout << "[TRACE] Process [" << procRank << "] has received part of array after MPI_Scatterv \n";

		queue<BinaryDouble> queueDataPar;
		queue<BinaryDouble> sortedDataPar;
		for (int i = 0; i < nSize; i++)
			queueDataPar.push(parallelCopy[i]);
		int uu = 7;	int oo = 128;
		RadixSort(&queueDataPar, &sortedDataPar, &uu, &oo);
		cout << "[TRACE] Process [" << procRank << "] has sorted its part of array \n";
		setResult(&sortedDataPar, &parallelCopy); // get sorted part of whole data from process


		int offset = 2;
		int numberOfPairs = procSize;
		int numberOfPairsPrev = procSize;
		int coeff = 1;
		MPI_Status status;
		while (numberOfPairs > 1)
		{
			numberOfPairs = numberOfPairs / 2 + numberOfPairs % 2;
			int remainder = procRank % offset;

			if (remainder == coeff)
			{
				int sendSize = sendCounts[procRank];
				MPI_Send(&sendSize, 1, MPI_INT, procRank - offset / 2, 9, MPI_COMM_WORLD);
				MPI_Send(parallelCopy, sendSize, BinaryDoubleType, procRank - offset / 2, 17, MPI_COMM_WORLD);
				cout << "[INFO ] procRank = " << procRank << "; numberOfPairs = " << numberOfPairs << "; offset = " << offset << "; remainder = " << remainder << "; receiver = " << procRank - offset / 2 << "; sendSize = " << sendSize << "; SEND \n";
			}
			else if (remainder == 0)
			{
				if (numberOfPairsPrev % numberOfPairs == 0 || procRank != (numberOfPairs - 1) * offset)
				{
					int receiveSize;
					MPI_Recv(&receiveSize, 1, MPI_INT, procRank + offset / 2, 9, MPI_COMM_WORLD, &status);
					BinaryDouble* recvBuf = new BinaryDouble[receiveSize];
					MPI_Recv(recvBuf, receiveSize, BinaryDoubleType, procRank + offset / 2, 17, MPI_COMM_WORLD, &status);
					parallelCopy = merge(&parallelCopy, &recvBuf, &sendCounts[procRank], &receiveSize);
					sendCounts[procRank] += receiveSize;
					cout << "[INFO ] procRank = " << procRank << "; numberOfPairs = " << numberOfPairs << "; offset = " << offset << "; remainder = " << remainder << "; sender = " << procRank + offset / 2 << "; receiveSize = " << receiveSize << "; RECV \n";
				}
			}

			numberOfPairsPrev = numberOfPairs;
			offset *= 2;
			coeff *= 2;
		}

		if (procRank == 0)
		{
			endTime = MPI_Wtime() * (double)1000;
			cout << "[TRACE] End time of parallel algorythm: " << endTime << "\n";
			timeOfParallel = endTime - startTime;
			cout << "[TRACE] Total time of parallel algorythm: " << timeOfParallel << " ms \n";
		/* ------------- END OF PARALLEL ALGORITHM --------------------- */
		bool isRight = checkResult(&nonParallel, &parallelCopy, &size);
		outputMessage(&timeOfNonParallel, &timeOfParallel, true);
		delete[] nonParallel;
		delete[] parallel;
	}
	delete[] sendCounts;
	delete[] displs;
	delete[] parallelCopy;
	cout << "[TRACE] Memory free... \n";
	MPI_Finalize();
	return 0;
}



