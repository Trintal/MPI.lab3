#pragma once
#include <stdio.h> 
#include "mpi.h" 
#include "time.h"
#include <vector>
#include <sstream>
#include <iostream>
using namespace std;

struct Request
{
	int procRank;
	int whatDoYouNeed;
};



 const int READER_OPEN = 0;
 const int READER_CLOSE = 1;
 const int WRITER_OPEN = 2;
 const int WRITER_CLOSE = 3;

 int RESPONCE_READER_YES = 4;
 int RESPONCE_READER_WAIT = 5;
 int RESPONCE_WRITER_YES = 6;
 int RESPONCE_WRITER_NO = 7;
 int RESPONCE_WRITER_WAIT = 8;

const int ERROR = -1;



class Repository
{
	int currentnNumberReaders;
	int numberWriters;
	int writerRank;
	bool writerIsWaiting;
	vector<string> *aboutProc;
	vector<int> *readerWait;
public:
	Repository(int procNum, int numberWriters)
	{
		currentnNumberReaders = 0;
		writerIsWaiting = false;
		writerRank = -1;
		this->numberWriters = numberWriters;
		readerWait = new vector<int>;
		aboutProc = new vector<string>();
		aboutProc->push_back((string)"0 Repository");
		int i;
		for (i = 1; (i < procNum) && (i <= numberWriters); i++)
		{
			stringstream oss;
			oss << i;
			aboutProc->push_back((string)(oss.str() + " Writer - not write"));
		}
		for (; i < procNum; i++)
		{
			stringstream oss;
			oss << i;
			aboutProc->push_back((string)(oss.str() + " Reader - not read"));
		}
	}
	void Start()
	{
		stringstream oss;
		MPI_Status status;
		Request request;
		string responce = "";
		printAbout();
		while (true)
		{
			MPI_Recv(&request, 2, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
			oss.str(std::string());
			oss.clear();
			oss << request.procRank;
			switch (request.whatDoYouNeed)
			{
			case READER_OPEN:
			{
				if (writerRank == -1)
				{
					currentnNumberReaders++;
					aboutProc->at(request.procRank) = oss.str() + " Reader - read";
					responce = oss.str() + " OPEN - YES";
					MPI_Send(&RESPONCE_READER_YES, 1, MPI_INT, request.procRank, 0, MPI_COMM_WORLD);
				}
				else
				{
					aboutProc->at(request.procRank) = oss.str() + " Reader - wait";
					responce = oss.str() + " OPEN - WAIT";
					readerWait->push_back(request.procRank);
					MPI_Send(&RESPONCE_READER_WAIT, 1, MPI_INT, request.procRank, 0, MPI_COMM_WORLD);
				}
				break;
			}
			case READER_CLOSE:
			{
				currentnNumberReaders--;
				aboutProc->at(request.procRank) = oss.str() + " Reader - not read";
				responce = oss.str() + " CLOSE - YES";
				MPI_Send(&RESPONCE_READER_YES, 1, MPI_INT, request.procRank, 0, MPI_COMM_WORLD);
				if ((writerIsWaiting) && (currentnNumberReaders == 0))
				{
					oss.str(std::string());
					oss.clear();
					oss << writerRank;
					aboutProc->at(writerRank) = oss.str() + " Writer - write";
					responce += "     " + oss.str() + " WRITER - OPEN";
					writerIsWaiting = false;
					MPI_Send(&RESPONCE_WRITER_YES, 1, MPI_INT, writerRank, 0, MPI_COMM_WORLD);
				}
				break;
			}
			case WRITER_OPEN:
			{
				if ((currentnNumberReaders == 0) && (writerRank == -1))
				{
					aboutProc->at(request.procRank) = oss.str() + " Writer - write";
					responce = oss.str() + " OPEN - YES";
					writerRank = request.procRank;
					MPI_Send(&RESPONCE_WRITER_YES, 1, MPI_INT, request.procRank, 0, MPI_COMM_WORLD);
				}
				else
				{
					if (writerRank == -1)
					{
						aboutProc->at(request.procRank) = oss.str() + " Writer - wait";
						responce = oss.str() + " OPEN - WAIT";
						writerRank = request.procRank;
						writerIsWaiting = true;
						MPI_Send(&RESPONCE_WRITER_WAIT, 1, MPI_INT, writerRank, 0, MPI_COMM_WORLD);
					}
					else
					{
						aboutProc->at(request.procRank) = oss.str() + " Writer - not write";
						responce = oss.str() + " OPEN - NO";
						MPI_Send(&RESPONCE_WRITER_NO, 1, MPI_INT, request.procRank, 0, MPI_COMM_WORLD);
					}
				}
				break;
			}
			case WRITER_CLOSE:
			{
				aboutProc->at(request.procRank) = oss.str() + " Writer - not write";
				responce = oss.str() + " CLOSE - YES";
				writerRank = -1;
				MPI_Send(&RESPONCE_WRITER_YES, 1, MPI_INT, writerRank, 0, MPI_COMM_WORLD);
				for (int i = 0; i < readerWait->size(); i++)
				{
					oss.str(std::string());
					oss.clear();
					oss << readerWait->at(i);
					aboutProc->at(readerWait->at(i)) = oss.str() + " Reader - read";
					MPI_Send(&RESPONCE_READER_YES, 1, MPI_INT, readerWait->at(i), 0, MPI_COMM_WORLD);
				}
				currentnNumberReaders = readerWait->size();
				readerWait->clear();
				break;
			}
			default:
			{
				break;
			}
			}
			printAbout("\n\n         " + responce);
		}
	}
	void printAbout(string responce = "")
	{
		cout << responce.c_str() << endl;
		cout << aboutProc->at(0).c_str() << endl;
		int i;
		for (i = 1; (i < aboutProc->size()) && (i <= numberWriters); i++)
		{
			cout << aboutProc->at(i).c_str() << endl;
		}
		for (; i < aboutProc->size(); i++)
		{
			cout << aboutProc->at(i).c_str() << endl;
		}
		cout << endl;
	}
};



class Reader
{
	int readerRank;
	bool isOpen;
public:
	Reader(int readerRank)
	{
		this->readerRank = readerRank;
		isOpen = false;
	}
	int Open()
	{
		if (!isOpen)
		{
			MPI_Status status;
			int responce;
			Request request;
			isOpen = true;
			request.procRank = readerRank;
			request.whatDoYouNeed = READER_OPEN;
			MPI_Send(&request, 2, MPI_INT, 0, 1, MPI_COMM_WORLD);
			MPI_Recv(&responce, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
			if (responce == RESPONCE_READER_WAIT)
			{
				MPI_Recv(&responce, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
			}
			return responce;
		}
		return ERROR;
	}
	int Close()
	{
		if (isOpen)
		{
			MPI_Status status;
			int responce;
			Request request;
			request.procRank = readerRank;
			request.whatDoYouNeed = READER_CLOSE;
			MPI_Send(&request, 2, MPI_INT, 0, 1, MPI_COMM_WORLD);
			MPI_Recv(&responce, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
			isOpen = false;
			return responce;
		}
		return ERROR;
	}
};



class Writer
{
	int writerRank;
	bool isOpen;
public:
	Writer(int writerRank)
	{
		isOpen = false;
		this->writerRank = writerRank;
	}
	int Open()
	{
		if (!isOpen)
		{
			MPI_Status status;
			int responce;
			Request request;
			request.procRank = writerRank;
			request.whatDoYouNeed = WRITER_OPEN;
			MPI_Send(&request, 2, MPI_INT, 0, 1, MPI_COMM_WORLD);
			MPI_Recv(&responce, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
			if ((responce == RESPONCE_WRITER_WAIT) || (responce == RESPONCE_WRITER_YES))
			{
				isOpen = true;
			}
			if (responce == RESPONCE_WRITER_WAIT)
			{
				MPI_Recv(&responce, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
			}
			return responce;
		}
		return ERROR;

	}
	int Close()
	{
		if (isOpen)
		{
			MPI_Status status;
			int responce;
			Request request;
			isOpen = false;
			request.procRank = writerRank;
			request.whatDoYouNeed = WRITER_CLOSE;
			MPI_Send(&request, 2, MPI_INT, 0, 1, MPI_COMM_WORLD);
			MPI_Recv(&responce, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
			isOpen = false;
			return responce;
		}
		return ERROR;
	}
};