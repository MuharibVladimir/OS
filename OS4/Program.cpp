#include "pch.h"
#include "stdafx.h"
#include "ThreadPool.h"


struct FileMapping {
	HANDLE hFile;
	HANDLE hMapping;
	size_t fsize;
	unsigned char* dataPtr;
};

std::vector<char> getFileBytes(std::string filename);

using namespace std;

void currentDateTime() {
	SYSTEMTIME lpSysT;
	GetSystemTime(&lpSysT);
	printf("%i-%i-%i %i:%i:%i:%i\n", lpSysT.wYear, lpSysT.wMonth,
		lpSysT.wDay, lpSysT.wHour, lpSysT.wMinute, lpSysT.wSecond, lpSysT.wMilliseconds);
}

int main()
{
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	const std::string filename = "initial.txt";
	int threadCount;
	cout << "Введите количество потоков: ";
	cin >> threadCount;
	int partsCount;
	cout << "Введите количество частей: ";
	cin >> partsCount;
	currentDateTime();

	auto bytes = getFileBytes(filename);
	vector<vector<char>> partBytes(partsCount);
	const int partSize = ceil(bytes.size() / static_cast<float>(partsCount));

	ThreadPool threadPool{ threadCount };
	for (auto i = 0; i < partsCount; i++)
	{
		auto lastPartDelta = 0;
		if (i == partsCount - 1)
		{
			lastPartDelta = partSize * partsCount - bytes.size();
		}

		partBytes[i] = vector<char>(partSize - lastPartDelta);
		memcpy(partBytes[i].data(), bytes.data() + i * partSize, partSize - lastPartDelta);

		auto pPartBytes = &partBytes[i];
		threadPool.pushRoutine([i, pPartBytes]
			{
				sort(pPartBytes->begin(), pPartBytes->end());
				cout << "Сортировка " + to_string(i + 1) + " части завершена\n";
			});
	}
	threadPool.finish();
	cout << "Слияние начато..." << endl;
	vector<char> result;


	ThreadPool threadPool2{ threadCount };

	int chunkID = 0;

	for (int k = 0; k < partBytes.size(); k += 2) {

		if (partBytes[k].capacity() != 0 && partBytes[k + 1].capacity() != 0) {
			auto pPartBytes1 = &partBytes[k];
			auto pPartBytes2 = &partBytes[k + 1];

			threadPool2.pushRoutine([pPartBytes1, pPartBytes2, chunkID]
				{
					std::vector <char> dst;
					std::merge(pPartBytes1->begin(), pPartBytes1->end(), pPartBytes2->begin(), pPartBytes2->end(), std::back_inserter(dst));
					std::ofstream output("temp/" + std::to_string(chunkID) + ".txt");
					copy(dst.begin(), dst.end(), ostream_iterator<char>(output));
					dst.clear();
				});

			chunkID++;
		}
		else k = partBytes.size();
	}
	threadPool2.finish();
	partBytes.clear();
	vector<FileMapping*> files(chunkID);

	ofstream fout("result.txt");
	for (int i = 0; i < chunkID; i++) {

		ifstream file_stream;
		file_stream.open("temp/" + std::to_string(i) + ".txt");
		string s;

		while (getline(file_stream, s)) {
			fout << s;
		}
		file_stream.close();

	}
	fout.close();
	cout << "Слияние завершено." << endl;

	currentDateTime();
	return 0;
}

std::vector<char> getFileBytes(const string filename)
{
	ifstream file(filename, ios::ate | ios::binary);

	if (!file.is_open())
	{
		throw runtime_error("Ошибка открытия:" + filename);
	}

	const auto fileSize = static_cast<size_t>(file.tellg());
	vector<char> stagingBuffer(fileSize);

	file.seekg(0);
	file.read(&stagingBuffer.front(), fileSize);
	file.close();

	return stagingBuffer;
}