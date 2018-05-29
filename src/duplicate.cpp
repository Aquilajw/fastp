#include "duplicate.h"
#include <memory.h>

Duplicate::Duplicate(Options* opt) {
    mOptions = opt;
    mKeyLenInBase = mOptions->duplicate.keylen;
    mKeyLenInBit = 1<<(2*mKeyLenInBase);
    mDups = new uint64[mKeyLenInBit];
    memset(mDups, 0, sizeof(uint64)*mKeyLenInBit);
    mCounts = new uint16[mKeyLenInBit];
    memset(mCounts, 0, sizeof(uint16)*mKeyLenInBit);
}

Duplicate::~Duplicate(){
    delete[] mDups;
    delete[] mCounts;
}

uint64 Duplicate::seq2int(const char* data, int start, int keylen, bool& valid) {
    uint64 ret = 0;
    for(int i=0; i<keylen; i++) {
        switch(data[i]) {
            case 'A':
                ret += 0;
                break;
            case 'T':
                ret += 1;
                break;
            case 'C':
                ret += 2;
                break;
            case 'G':
                ret += 3;
                break;
            default:
                valid = false;
                return 0;
        }
        // if it's not the last one, shift it by 2 bits
        if(i != keylen-1)
            ret <<= 2;
    }
    return ret;
}

void Duplicate::addRecord(uint32 key, uint64 kmer32) {
    if(mCounts[key] == 0) {
        mCounts[key] = 1;
        mDups[key] = kmer32;
    } else {
        if(mDups[key] == kmer32)
            mCounts[key]++;
    }
}

void Duplicate::statRead(Read* r) {
    if(r->length() < 32)
        return;

    int start1 = 0;
    int start2 = max(0, r->length() - 32 - 5);

    const char* data = r->mSeq.mStr.c_str();
    bool valid = true;

    uint64 ret = seq2int(data, start1, mKeyLenInBase, valid);
    uint32 key = (uint32)ret;
    if(!valid)
        return;

    uint64 kmer32 = seq2int(data, start2, 32, valid);
    if(!valid)
        return;

    addRecord(key, kmer32);
}

void Duplicate::statPair(Read* r1, Read* r2) {
    if(r1->length() < 32 || r2->length() < 32)
        return;

    const char* data1 = r1->mSeq.mStr.c_str();
    const char* data2 = r2->mSeq.mStr.c_str();
    bool valid = true;

    uint64 ret = seq2int(data1, 0, mKeyLenInBase, valid);
    uint32 key = (uint32)ret;
    if(!valid)
        return;

    uint64 kmer32 = seq2int(data2, 0, 32, valid);
    if(!valid)
        return;

    addRecord(key, kmer32);
}

double Duplicate::statAll(vector<Duplicate*>& list, int* hist, int histSize) {
    long totalNum = 0;
    long dupNum = 0;
    for(int key=0; key<list[0]->mKeyLenInBit; key++) {
        bool consistent = true;
        for(int i=0; i<list.size()-1; i++) {
            if(list[i]->mDups[key] != list[i+1]->mDups[key] && list[i]->mCounts[key]>0 && list[i+1]->mCounts[key]>0) {
                consistent = false;
            }
        }
        if(consistent) {
            int count = 0;
            for(int i=0; i<list.size(); i++) {
                count += list[i]->mCounts[key];
            }

            if(count > 0) {
                totalNum += count;
                dupNum += count - 1;

                if(count >= histSize)
                    hist[histSize-1]++;
                else
                    hist[count]++;
            }
        }
    }

    if(totalNum == 0)
        return 0.0;
    else
        return (double)dupNum / (double)totalNum;
}