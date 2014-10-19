#pragma once
#include <vector>
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
	virtual void Join() = 0;

};


class ThreadMgrForRows : public IThreadMgrForRows
{
public:
	ThreadMgrForRows(size_t _nMaxThreads, size_t _srcWidth, size_t _srcHeight, vector<vector<vec3>> & _srcImg, size_t _destWidth, size_t _destHeight, unsigned char * _destImg)
		: nMaxThreads(_nMaxThreads), srcWidth(_srcWidth), srcHeight(_srcHeight),  srcImg(_srcImg), destWidth(_destWidth), destHeight(_destHeight), destImg(_destImg) {}
	virtual ~ThreadMgrForRows() {
		for(auto i : threadList) {
			std::thread * t = i;

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

		Join();

		while(!AllRowsProcessed() && threadList.size() < nMaxThreads) {
			int nextRow = GetNextRow();
			if(nextRow == -1)
				break;

			ThreadArgList argList(nextRow, this, srcWidth, srcHeight, srcImg, destWidth, destHeight, destImg);
			std::thread * th = new std::thread(func, argList);
			threadList.push_back(th);
			
		}

	}

	void Join() {
		for(auto i : threadList) {
			i->join(); // Make it with a timer.
		}
		threadList.clear();
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

	vector<std::thread *> threadList;
	vector<int> rowsLeft;
	std::mutex mutexRowsLeft;
};

