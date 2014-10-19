#pragma once
#include <vector>
#include <utility>
using namespace std;

class ThreadMgrForRows;

struct ThreadArgList
{
	ThreadArgList(int _curRow, ThreadMgrForRows * mgr, size_t _srcWidth, size_t _srcHeight, vector<vector<vec3>> & _srcImg, size_t _destWidth, size_t _destHeight, unsigned char * _destImg)
		: curRow(_curRow), threadMgr(mgr), srcWidth(_srcWidth), srcHeight(_srcHeight), source(_srcImg), destWidth(_destWidth), destHeight(_destHeight), dest(_destImg) {}
	size_t curRow; // the row currently processed by the thread.
	size_t srcWidth;
	size_t srcHeight;
	vector<vector<vec3> > & source;
	size_t destWidth;
	size_t destHeight;
	unsigned char * dest;
	ThreadMgrForRows * threadMgr;
};

class IThreadMgrForRows
{
public:
	virtual ~IThreadMgrForRows() {}
	virtual void Init() = 0;

	virtual bool AllRowsProcessed() = 0;
	virtual void RunThread(void func(const ThreadArgList &)) = 0;
	virtual void JoinSynchronos() = 0;
	virtual void JoinAsynchronos() = 0;
	virtual void SetFinished(int row) = 0;

};

bool * tFinished = NULL;
class ThreadMgrForRows : public IThreadMgrForRows
{
public:
	ThreadMgrForRows(size_t _nMaxThreads, size_t _srcWidth, size_t _srcHeight, vector<vector<vec3>> & _srcImg, size_t _destWidth, size_t _destHeight, unsigned char * _destImg)
		: nMaxThreads(_nMaxThreads), srcWidth(_srcWidth), srcHeight(_srcHeight),  srcImg(_srcImg), destWidth(_destWidth), destHeight(_destHeight), destImg(_destImg) {}
	virtual ~ThreadMgrForRows() {
		for(auto i : threadList) {
			std::thread * t = i.first;

			delete t;
		}
		threadList.clear();
	}


	void Init() {
		for( size_t i = 0 ; i < destHeight ; i++ ) {
			rowsLeft.push_back(i);
		}
	}

	bool AllRowsProcessed() { return rowsLeft.empty(); }
	void RunThread(void func(const ThreadArgList &)) {

		JoinAsynchronos();

		while(!AllRowsProcessed() && threadList.size() < nMaxThreads) {
			int nextRow = GetNextRow();
			if(nextRow == -1)
				break;

			ThreadArgList argList(nextRow, this, srcWidth, srcHeight, srcImg, destWidth, destHeight, destImg);
			std::thread * th = new std::thread(func, argList);
			threadList.push_back( pair<thread *, int> (th, nextRow) );
		}

	}

	void JoinAsynchronos() {
	
		for(auto it = threadList.begin() ; it != threadList.end() ; ) {

			int row = it->second;

			if(tFinished[row]) {
				thread * t = it->first;
				t->join();
				delete t;
				it = threadList.erase(it);
			}
			else
				it++;
		}
	}

	void JoinSynchronos() {

		for(auto it = threadList.begin() ; it != threadList.end() ; ++it) {

			thread * t = it->first;
			t->join();
			delete t;
		}
		threadList.clear();
	}

	virtual void SetFinished(int row) {
		{
			// necessary?
			mutexThreadFinished.lock();
			tFinished[row] = true;
			mutexThreadFinished.unlock();
		}
	}

protected:

	int GetNextRow() {
		if(rowsLeft.size() != 0)
		{
			mutexRowsLeft.lock();
			int nRow = rowsLeft.back();
			rowsLeft.pop_back();
			mutexRowsLeft.unlock();
			return nRow;
		}
		return -1;
	}

	

	// image info.
	size_t nMaxThreads;
	size_t srcWidth;
	size_t srcHeight;
	vector<vector<vec3>> & srcImg;
	size_t destWidth;
	size_t destHeight;
	unsigned char * destImg;

	vector< pair<std::thread *, int /* row_num */> > threadList;
	vector<int> rowsLeft;
	std::mutex mutexRowsLeft;
	std::mutex mutexThreadFinished;
};

